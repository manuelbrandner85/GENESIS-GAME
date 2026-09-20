// GENESIS: Der Kreislauf des Lebens

#include "GenesisSpermSwarm.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GenesisConceptionSubsystem.h"
#include "GenesisDebug.h"
#include "GenesisFertilizationLogic.h"
#include "GenesisLog.h"
#include "GenesisOocyte.h"
#include "GenesisRandom.h"
#include "GenesisSpermSwimLogic.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	constexpr int32 MaterialDataCount = 4;
	constexpr uint64 CellSeedSalt = 0x5BE2Aull;
}

#if !UE_BUILD_SHIPPING
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisTimeScaleCommand(
		TEXT("genesis.Conception.TimeScale"),
		TEXT("Zeitraffer des Schwarms: Simulationssekunden je Echtzeitsekunde (Standard 0,25)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				return;
			}
			const float Scale = FCString::Atof(*Args[0]);
			for (TActorIterator<AGenesisSpermSwarm> It(World); It; ++It)
			{
				It->TimeScale = Scale;
				UE_LOG(LogGenesis, Display, TEXT("Conception: Zeitraffer %.2f× (Simulationszeit je Sekunde)"), Scale);
			}
		}));
}
#endif

AGenesisSpermSwarm::AGenesisSpermSwarm()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cells"));
	SetRootComponent(Instances);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Instances->SetNumCustomDataFloats(MaterialDataCount);
	// Kleine, bewegte Objekte: keine Distanzfelder, aber Bewegungsunschärfe über vorherige Transformationen
	Instances->bAffectDistanceFieldLighting = false;
	Instances->SetCastShadow(true);
	// Sechstausend Zellen, deren Geißel im Shader schlägt: Ihre Strahlengeometrie müsste jedes Bild
	// neu gebaut werden und bliebe dauerhaft im Speicher – 87 MiB, die nichts zurückgeben. Eine
	// 5 µm große, fast durchsichtige Zelle spiegelt sich in nichts und erhellt nichts. Sie bleibt
	// deshalb aus der Strahlenszene heraus; gesehen, beleuchtet und beschattet wird sie weiterhin.
	Instances->SetVisibleInRayTracing(false);
}

void AGenesisSpermSwarm::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildSwarm();
}

void AGenesisSpermSwarm::BeginPlay()
{
	Super::BeginPlay();
	RebuildSwarm();
	RegisterDebugPage();
}

void AGenesisSpermSwarm::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING
	if (!DebugPageId.IsNone())
	{
		GenesisDebug::UnregisterPage(DebugPageId);
	}
#endif
	Super::EndPlay(EndPlayReason);
}

void AGenesisSpermSwarm::RebuildSwarm()
{
	if (!Instances)
	{
		return;
	}

	// Auch hier und nicht nur im Konstruktor: Ein bereits im Level abgelegter Schwarm trägt seine
	// eigenen gespeicherten Werte und würde eine Änderung am Konstruktor nie sehen.
	Instances->SetVisibleInRayTracing(false);

	Instances->SetStaticMesh(CellMesh);
	if (CellMaterial)
	{
		Instances->SetMaterial(0, CellMaterial);
	}
	Instances->ClearInstances();
	Instances->SetNumCustomDataFloats(MaterialDataCount);

	FGenesisRandomStream VitalityRandom(GenesisHash::Combine(static_cast<uint64>(Seed), CellSeedSalt));
	Cells.Reset(CellCount);
	for (int32 Index = 0; Index < CellCount; ++Index)
	{
		const float Vitality = FMath::Clamp(VitalityRandom.Gaussian(MeanVitality, 0.18f), 0.0f, 1.0f);
		const uint64 CellSeed = GenesisHash::Combine(GenesisHash::Combine(static_cast<uint64>(Seed), CellSeedSalt), static_cast<uint64>(Index));
		FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(CellSeed, Vitality, Channel, Tuning);

		// Die Zellen kommen nicht gleichmäßig über den ganzen Eileiter verteilt an, sondern als Pulk
		// von der Gebärmutter her: Der Zug, der es bis in die Ampulle geschafft hat, zieht gemeinsam
		// flussaufwärts. Gleichverteilung über drei Millimeter sieht dagegen nach Einzelgängern aus.
		if (StartBandSpreadUm > 0.0f)
		{
			const float BandCenter = Oocyte
				? static_cast<float>(GetActorTransform().InverseTransformPosition(Oocyte->GetActorLocation()).X
					/ GenesisMicroScale::UnitsPerMicrometer) - StartBandDistanceUm
				: 0.5f * Channel.LengthUm - StartBandDistanceUm;

			const float Offset = VitalityRandom.Gaussian(0.0f, StartBandSpreadUm);
			Cell.Position.X = FMath::Fmod(BandCenter + Offset + Channel.LengthUm, Channel.LengthUm);
			if (Cell.Position.X < 0.0)
			{
				Cell.Position.X += Channel.LengthUm;
			}
		}

		Cells.Add(MoveTemp(Cell));
	}

	SimulationSeconds = 0.0;
	StepAccumulator = 0.0f;

	TransformBuffer.SetNum(Cells.Num());
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		TransformBuffer[Index] = GenesisSpermSwimLogic::ComputeVisualTransform(Cells[Index]);
	}
	Instances->PreAllocateInstancesMemory(Cells.Num());
	Instances->AddInstances(TransformBuffer, false, false, false);
	Instances->SetHasPerInstancePrevTransforms(true);
	PushInstances(true);
}

void AGenesisSpermSwarm::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisSpermSwarm_Tick);
	const double Start = FPlatformTime::Seconds();

	if (Instances && Instances->GetInstanceCount() != Cells.Num())
	{
		RebuildSwarm();
	}

	SimulateFor(DeltaSeconds * TimeScale);
	PushInstances(false);

	LastTickMs = static_cast<float>((FPlatformTime::Seconds() - Start) * 1000.0);
}

void AGenesisSpermSwarm::SimulateFor(float SimulationDelta)
{
	const float StepSeconds = FMath::Max(0.001f, Tuning.FixedStepSeconds);
	StepAccumulator += FMath::Max(0.0f, SimulationDelta);

	// Begrenzung gegen Spiralen bei Rucklern (Hitch): höchstens 8 Schritte je Frame
	int32 Steps = FMath::Min(FMath::FloorToInt(StepAccumulator / StepSeconds), 8);
	if (Steps == 8)
	{
		StepAccumulator = 0.0f;
	}
	else
	{
		StepAccumulator -= static_cast<float>(Steps) * StepSeconds;
	}

	// Mit Eizelle: Der Schritt enthält Lockwirkung, Cumulus, Bindung, Akrosomreaktion, Durchdringung und den Polyspermie-Block
	if (Oocyte)
	{
		Oocyte->GetMutableState().Position = GetActorTransform().InverseTransformPosition(Oocyte->GetActorLocation()) / GenesisMicroScale::UnitsPerMicrometer;
	}

	for (int32 StepIndex = 0; StepIndex < Steps; ++StepIndex)
	{
		if (Oocyte)
		{
			if (GenesisFertilizationLogic::Step(Cells, Oocyte->GetMutableState(), Channel, Tuning, Oocyte->Tuning, StepSeconds, FertilizationResult))
			{
				UE_LOG(LogGenesis, Log, TEXT("Conception: Zelle %d verschmilzt mit der Eizelle nach %.1f s (Vitalität %.2f, %d Mitbewerber an der Zona)."),
					FertilizationResult.CellIndex, FertilizationResult.SecondsToFusion, FertilizationResult.Vitality, FertilizationResult.CompetingCells);
				OnFertilized.Broadcast(FertilizationResult);

				// Aus dem Mikrokosmos wird ein Mensch: Genom, erster Körper, Inkarnation, Leitmotiv
				if (bCreateLifeOnFertilization)
				{
					UGameInstance* GameInstance = GetGameInstance();
					if (UGenesisConceptionSubsystem* Conception = GameInstance ? GameInstance->GetSubsystem<UGenesisConceptionSubsystem>() : nullptr)
					{
						Conception->Conceive(FertilizationResult);
					}
				}
			}
		}
		else
		{
			for (FGenesisSpermCell& Cell : Cells)
			{
				GenesisSpermSwimLogic::Step(Cell, Channel, Tuning, StepSeconds);
			}
		}
		SimulationSeconds += StepSeconds;
	}
}

void AGenesisSpermSwarm::PushInstances(bool bTeleport)
{
	if (!Instances || Cells.Num() == 0 || Instances->GetInstanceCount() != Cells.Num())
	{
		return;
	}

	const bool bHasPrevious = !bTeleport && PreviousTransformBuffer.Num() == Cells.Num();
	TransformBuffer.SetNum(Cells.Num());
	PreviousTransformBuffer.SetNum(Cells.Num());
	CustomDataBuffer.SetNum(Cells.Num() * MaterialDataCount);
	const double JumpThreshold = 0.5 * Channel.LengthUm * GenesisMicroScale::UnitsPerMicrometer;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		const FGenesisSpermCell& Cell = Cells[Index];
		// Gebundene und bohrende Zellen stecken mit dem Kopf in der Zona – sie werden anders ausgerichtet als schwimmende
		const bool bAttached = Oocyte && GenesisFertilizationLogic::GetPhase(Cell) != EGenesisSpermPhase::Swimming;
		const FTransform Current = bAttached
			? GenesisFertilizationLogic::ComputeAttachedTransform(Cell, Oocyte->GetState())
			: GenesisSpermSwimLogic::ComputeVisualTransform(Cell);
		// Bewegungsunschärfe aus der echten Bewegung; beim Umlaufen am Abschnittsende kein Wisch durch den ganzen Kanal
		const bool bJumped = !bHasPrevious || FVector::Dist(TransformBuffer[Index].GetLocation(), Current.GetLocation()) > JumpThreshold;
		PreviousTransformBuffer[Index] = bJumped ? Current : TransformBuffer[Index];
		TransformBuffer[Index] = Current;
		GenesisSpermSwimLogic::ComputeMaterialData(Cell, &CustomDataBuffer[Index * MaterialDataCount]);
	}

	Instances->BatchUpdateInstancesTransforms(0, TransformBuffer, PreviousTransformBuffer, false, false, bTeleport);
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		Instances->SetCustomData(Index, TArrayView<const float>(&CustomDataBuffer[Index * MaterialDataCount], MaterialDataCount), false);
	}
	Instances->MarkRenderStateDirty();
}

int32 AGenesisSpermSwarm::FindAttachedCell() const
{
	// Die verschmolzene Zelle zuerst – sie ist der Moment, um den es geht
	int32 Bound = INDEX_NONE;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		switch (GenesisFertilizationLogic::GetPhase(Cells[Index]))
		{
		case EGenesisSpermPhase::Fused:
			return Index;
		case EGenesisSpermPhase::Penetrating:
			Bound = Index;
			break;
		case EGenesisSpermPhase::Bound:
			Bound = Bound == INDEX_NONE ? Index : Bound;
			break;
		default:
			break;
		}
	}
	return Bound;
}

FTransform AGenesisSpermSwarm::GetCellWorldTransform(int32 Index) const
{
	const FGenesisSpermCell* Cell = GetCell(Index);
	return Cell ? GenesisSpermSwimLogic::ComputeVisualTransform(*Cell) * GetActorTransform() : GetActorTransform();
}

FVector AGenesisSpermSwarm::GetCellHeadWorldPosition(int32 Index) const
{
	const FGenesisSpermCell* Cell = GetCell(Index);
	return Cell ? GetActorTransform().TransformPosition(GenesisSpermSwimLogic::ComputeHeadPosition(*Cell) * GenesisMicroScale::UnitsPerMicrometer) : GetActorLocation();
}

void AGenesisSpermSwarm::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	DebugPageId = TEXT("Conception");
	TWeakObjectPtr<AGenesisSpermSwarm> WeakThis(this);
	GenesisDebug::RegisterPage({
		DebugPageId,
		TEXT("Conception – Spermienschwarm"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const AGenesisSpermSwarm* Self = WeakThis.Get();
			if (!Self || Self->Cells.Num() == 0)
			{
				return;
			}

			int32 Progressive = 0;
			int32 Hyper = 0;
			int32 Sluggish = 0;
			int32 NearWall = 0;
			// Wer rückwärts schwimmt, treibt mit dem Strom zur Gebärmutter – für diesen Schwarm die falsche Richtung
			int32 Backwards = 0;
			double SpeedSum = 0.0;
			double UpstreamSum = 0.0;
			for (const FGenesisSpermCell& Cell : Self->Cells)
			{
				Progressive += Cell.Motility == EGenesisSpermMotility::Progressive ? 1 : 0;
				Hyper += Cell.Motility == EGenesisSpermMotility::Hyperactivated ? 1 : 0;
				Sluggish += Cell.Motility == EGenesisSpermMotility::Sluggish ? 1 : 0;
				NearWall += Self->Channel.LumenRadiusUm - FVector2D(Cell.Position.Y, Cell.Position.Z).Size() < Self->Tuning.WallAttractionDistanceUm ? 1 : 0;
				SpeedSum += Cell.Speed;
				UpstreamSum += Cell.Heading.X;
				Backwards += Cell.Heading.X < 0.0 ? 1 : 0;
			}
			const double Count = Self->Cells.Num();
			OutLines.Add(FString::Printf(TEXT("Zellen %d | progressiv %d | hyperaktiviert %d | träge %d | Simulationszeit %.1f s (×%.2f)"),
				Self->Cells.Num(), Progressive, Hyper, Sluggish, Self->SimulationSeconds, Self->TimeScale));
			if (Self->Oocyte)
			{
				const FGenesisOocyteState& Egg = Self->Oocyte->GetState();
				OutLines.Add(FString::Printf(TEXT("Eizelle: %s | an der Zona %d | Cortikalreaktion %.0f %%"),
					Egg.IsFertilized() ? *FString::Printf(TEXT("befruchtet von Zelle %d nach %.1f s"), Egg.FertilizedByCell, Self->FertilizationResult.SecondsToFusion) : TEXT("unbefruchtet"),
					Egg.BoundCells, 100.0f * Egg.CorticalReaction));
			}
			OutLines.Add(FString::Printf(TEXT("Ø Vortrieb %.1f µm/s | an der Wand (<%.0f µm) %.0f %% | Ø Ausrichtung gegen den Strom %+.2f | rückwärts %.0f %% | CPU %.3f ms"),
				SpeedSum / Count, Self->Tuning.WallAttractionDistanceUm, 100.0 * NearWall / Count, UpstreamSum / Count,
				100.0 * Backwards / Count, Self->LastTickMs));
		}
	});
#endif
}
