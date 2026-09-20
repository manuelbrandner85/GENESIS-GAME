// GENESIS: Der Kreislauf des Lebens

#include "GenesisSpermSwarm.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GenesisDebug.h"
#include "GenesisRandom.h"
#include "GenesisSpermSwimLogic.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	constexpr int32 MaterialDataCount = 4;
	constexpr uint64 CellSeedSalt = 0x5BE2Aull;
}

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
		Cells.Add(GenesisSpermSwimLogic::CreateCell(CellSeed, Vitality, Channel, Tuning));
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

	for (int32 StepIndex = 0; StepIndex < Steps; ++StepIndex)
	{
		for (FGenesisSpermCell& Cell : Cells)
		{
			GenesisSpermSwimLogic::Step(Cell, Channel, Tuning, StepSeconds);
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
		const FTransform Current = GenesisSpermSwimLogic::ComputeVisualTransform(Cell);
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
			}
			const double Count = Self->Cells.Num();
			OutLines.Add(FString::Printf(TEXT("Zellen %d | progressiv %d | hyperaktiviert %d | träge %d | Simulationszeit %.1f s (×%.2f)"),
				Self->Cells.Num(), Progressive, Hyper, Sluggish, Self->SimulationSeconds, Self->TimeScale));
			OutLines.Add(FString::Printf(TEXT("Ø Vortrieb %.1f µm/s | an der Wand (<%.0f µm) %.0f %% | Ø Ausrichtung gegen den Strom %+.2f | CPU %.3f ms"),
				SpeedSum / Count, Self->Tuning.WallAttractionDistanceUm, 100.0 * NearWall / Count, UpstreamSum / Count, Self->LastTickMs));
		}
	});
#endif
}
