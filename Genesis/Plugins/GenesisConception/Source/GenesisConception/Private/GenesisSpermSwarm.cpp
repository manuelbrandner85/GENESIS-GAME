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
	TAutoConsoleVariable<int32> CVarRaceAutoPilot(
		TEXT("genesis.Race.AutoPilot"),
		0,
		TEXT("1 = die eigene Zelle lenkt und schlägt von selbst wie ein guter Spieler (Prüfhilfe)."));

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

		// Die Zellen kommen von der Gebärmutter her, flussaufwärts. Im Rennen treffen sie nach und nach ein
		// (Wilcox 1995, Docs/38): Das Feld beginnt hinter der eigenen Zelle und reicht weit zurück.
		const float BandDistance = bRaceLayout ? RaceTuning.FieldDistanceUm : StartBandDistanceUm;
		const float BandSpread = bRaceLayout ? RaceTuning.FieldSpreadUm : StartBandSpreadUm;
		if (BandSpread > 0.0f)
		{
			const float BandCenter = Oocyte
				? static_cast<float>(GetActorTransform().InverseTransformPosition(Oocyte->GetActorLocation()).X
					/ GenesisMicroScale::UnitsPerMicrometer) - BandDistance
				: 0.5f * Channel.LengthUm - BandDistance;

			const float Spread = VitalityRandom.Gaussian(0.0f, BandSpread);
			const float Offset = bRaceLayout ? -FMath::Abs(Spread) : Spread;
			Cell.Position.X = FMath::Fmod(BandCenter + Offset + Channel.LengthUm, Channel.LengthUm);
			if (Cell.Position.X < 0.0)
			{
				Cell.Position.X += Channel.LengthUm;
			}
		}

		Cells.Add(MoveTemp(Cell));
	}

	// Die Zelle des Spielers: vorn, eine der stärksten, progressiv, Richtung Eizelle – und kapazitiert,
	// also jetzt bereit. Ohne diesen Zustand könnte sie gar nicht befruchten (Docs/38).
	PlayerCellIndex = INDEX_NONE;
	PlayerAtEggSeconds = -1.0;
	RaceOutcome = EGenesisRaceOutcome::None;
	PlayerVigor = 0.0f;
	PlayerPlace = 0;
	if (bRaceLayout && Oocyte && Cells.Num() > 0)
	{
		const FVector EggLocal = GetActorTransform().InverseTransformPosition(Oocyte->GetActorLocation()) / GenesisMicroScale::UnitsPerMicrometer;
		const uint64 PlayerSeed = GenesisHash::Combine(static_cast<uint64>(Seed), 0x91A7E5ull);
		FGenesisSpermCell Mine = GenesisSpermSwimLogic::CreateCell(PlayerSeed, RaceTuning.Vitality, Channel, Tuning);
		Mine.bCapacitated = true;
		GenesisSpermSwimLogic::ApplyMotility(Mine, EGenesisSpermMotility::Progressive, Tuning);
		Mine.Position = FVector(EggLocal.X - RaceTuning.StartDistanceUm, EggLocal.Y + 60.0, EggLocal.Z - 40.0);
		Mine.Heading = FVector::ForwardVector;
		PlayerCellIndex = 0;
		Cells[PlayerCellIndex] = MoveTemp(Mine);
		RaceOutcome = EGenesisRaceOutcome::Running;
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

void AGenesisSpermSwarm::StartRace(uint64 RunSeed)
{
	if (!Oocyte)
	{
		return;
	}
	bRaceLayout = true;
	// Jedes Rennen ein eigenes Feld – vorher stand in jedem Durchlauf dieselbe Siegerin am Start
	if (RunSeed != 0)
	{
		Seed = static_cast<int32>(RunSeed & 0x7FFFFFFF);
	}
	LastReportedPhase = 255;
	bReportedCumulus = false;
	bReportedHyper = false;
	// Die Eizelle unberührt: Vor dem Start darf nichts geschehen sein, was das Rennen vorwegnimmt
	FGenesisOocyteState& Egg = Oocyte->GetMutableState();
	Egg.FertilizedByCell = INDEX_NONE;
	Egg.bZonaHardened = false;
	Egg.CorticalReaction = 0.0f;
	Egg.SecondsSinceFusion = 0.0f;
	Egg.BoundCells = 0;
	Egg.PerivitellineCells = 0;
	FertilizationResult = FGenesisFertilizationResult();
	EffectiveTimeScale = -1.0f;
	RebuildSwarm();
	const FVector EggLocal = GetActorTransform().InverseTransformPosition(Oocyte->GetActorLocation()) / GenesisMicroScale::UnitsPerMicrometer;
	UE_LOG(LogGenesis, Display, TEXT("Conception: Das Rennen beginnt – %d Zellen (Seed %d), die eigene %.0f µm vor der Eizelle. Kanal: Lumen %.0f µm, Wandstrom %.0f µm/s (Mitte %.0f %%), Eizelle bei (%.0f, %.0f, %.0f), Cumulus %.0f µm, Schritt %.4f s."),
		Cells.Num(), Seed, RaceTuning.StartDistanceUm, Channel.LumenRadiusUm, Channel.WallFlowSpeedUm, 100.0f * Channel.CoreFlowFraction,
		EggLocal.X, EggLocal.Y, EggLocal.Z, Oocyte->GetState().CumulusRadiusUm, Tuning.FixedStepSeconds);
}

void AGenesisSpermSwarm::ReportPlayerProgress()
{
	if (!Cells.IsValidIndex(PlayerCellIndex) || !Oocyte)
	{
		return;
	}
	const FGenesisSpermCell& Mine = Cells[PlayerCellIndex];
	const double Distance = FVector::Dist(Mine.Position, Oocyte->GetState().Position);
	if (!bReportedCumulus && Distance < Oocyte->GetState().CumulusRadiusUm)
	{
		bReportedCumulus = true;
		UE_LOG(LogGenesis, Display, TEXT("Rennen: eigene Zelle im Cumulus nach %.1f s (Platz %d)."), SimulationSeconds, PlayerPlace);
	}
	if (!bReportedHyper && Mine.Motility == EGenesisSpermMotility::Hyperactivated)
	{
		bReportedHyper = true;
		UE_LOG(LogGenesis, Display, TEXT("Rennen: eigene Zelle hyperaktiviert nach %.1f s."), SimulationSeconds);
	}
	if (Mine.Phase != LastReportedPhase)
	{
		LastReportedPhase = Mine.Phase;
		if (PlayerAtEggSeconds < 0.0 && GenesisFertilizationLogic::IsAttached(Mine))
		{
			PlayerAtEggSeconds = SimulationSeconds;
		}
		UE_LOG(LogGenesis, Display, TEXT("Rennen: eigene Zelle %s nach %.1f s (Tiefe %.1f µm, Kraft %.2f)."),
			*StaticEnum<EGenesisSpermPhase>()->GetNameStringByValue(Mine.Phase), SimulationSeconds, Mine.PenetrationDepthUm, PlayerVigor);
	}
}

void AGenesisSpermSwarm::SetPlayerInput(const FVector2D& Steer, int32 StrokePresses)
{
	PlayerSteer = Steer;
	PendingStrokes += FMath::Max(0, StrokePresses);
}

EGenesisSpermPhase AGenesisSpermSwarm::GetPlayerPhase() const
{
	return Cells.IsValidIndex(PlayerCellIndex) ? GenesisFertilizationLogic::GetPhase(Cells[PlayerCellIndex]) : EGenesisSpermPhase::Swimming;
}

bool AGenesisSpermSwarm::IsPlayerHyperactivated() const
{
	return Cells.IsValidIndex(PlayerCellIndex) && Cells[PlayerCellIndex].Motility == EGenesisSpermMotility::Hyperactivated;
}

float AGenesisSpermSwarm::GetPlayerDistanceToZonaUm() const
{
	return Cells.IsValidIndex(PlayerCellIndex) && Oocyte
		? FMath::Max(0.0f, GenesisFertilizationLogic::DistanceToZona(Cells[PlayerCellIndex], Oocyte->GetState()))
		: 0.0f;
}

float AGenesisSpermSwarm::GetPlayerPenetrationUm() const
{
	return Cells.IsValidIndex(PlayerCellIndex) ? Cells[PlayerCellIndex].PenetrationDepthUm : 0.0f;
}

float AGenesisSpermSwarm::GetZonaThicknessUm() const
{
	return Oocyte ? Oocyte->GetState().ZonaOuterRadiusUm - Oocyte->GetState().ZonaInnerRadiusUm : 17.0f;
}

int32 AGenesisSpermSwarm::GetPerivitellineCells() const
{
	return Oocyte ? Oocyte->GetState().PerivitellineCells : 0;
}

bool AGenesisSpermSwarm::WantsTimeLapse() const
{
	const FGenesisOocyteState* Egg = Oocyte ? &Oocyte->GetState() : nullptr;
	if (!Egg)
	{
		return false;
	}
	// Nach der Verschmelzung: Die Cortikalreaktion dauert Minuten – gerafft, bis sie durch ist
	const bool bBlocking = Egg->IsFertilized() && Egg->CorticalReaction < 1.0f;
	if (IsRacing())
	{
		// Im Rennen erst, wenn die eigene Zelle angekommen ist: Wer noch lenkt, braucht die Zeitlupe
		return bBlocking || (Cells.IsValidIndex(PlayerCellIndex) && GenesisFertilizationLogic::IsAttached(Cells[PlayerCellIndex]));
	}
	return bBlocking || (!Egg->IsFertilized() && Egg->BoundCells + Egg->PerivitellineCells > 0);
}

void AGenesisSpermSwarm::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Kraft des Spielers: Tastendrücke heben sie, sie fällt in Echtzeit ab
	if (IsRacing())
	{
		PlayerVigor = GenesisSpermRace::UpdateVigor(PlayerVigor, PendingStrokes, DeltaSeconds, RaceTuning);
		PendingStrokes = 0;
		PlaceTimer -= DeltaSeconds;
		if (PlaceTimer <= 0.0f && Oocyte)
		{
			PlaceTimer = 0.25f;
			PlayerPlace = GenesisSpermRace::CountCellsAhead(Cells, PlayerCellIndex, Oocyte->GetState()) + 1;
		}
		ReportPlayerProgress();
	}
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisSpermSwarm_Tick);
	const double Start = FPlatformTime::Seconds();

	if (Instances && Instances->GetInstanceCount() != Cells.Num())
	{
		RebuildSwarm();
	}

	// An der Eizelle: sichtbarer Zeitraffer statt Zeitlupe. Die Rampe läuft gleichmäßig im Verhältnis
	// (logarithmisch) – linear sähe der Weg von 0,3 auf 45 aus wie ein Sprung am Ende.
	const float Target = FMath::Max(0.01f, WantsTimeLapse() ? TimeLapseScale : TimeScale);
	if (EffectiveTimeScale <= 0.0f)
	{
		EffectiveTimeScale = Target;
	}
	const float LogSpan = FMath::Abs(FMath::Loge(FMath::Max(TimeLapseScale, 1.0f) / FMath::Max(TimeScale, 0.01f)));
	const float LogRate = FMath::Max(LogSpan, 0.1f) / FMath::Max(0.1f, TimeScaleRampSeconds);
	EffectiveTimeScale = FMath::Exp(FMath::FInterpConstantTo(FMath::Loge(EffectiveTimeScale), FMath::Loge(Target), DeltaSeconds, LogRate));
	SimulateFor(DeltaSeconds * EffectiveTimeScale, IsTimeLapse() ? TimeLapseStepSeconds : Tuning.FixedStepSeconds);
	PushInstances(false);

	LastTickMs = static_cast<float>((FPlatformTime::Seconds() - Start) * 1000.0);
}

void AGenesisSpermSwarm::SimulateFor(float SimulationDelta, float RequestedStepSeconds)
{
	const float StepSeconds = FMath::Max(0.001f, RequestedStepSeconds);
	StepAccumulator += FMath::Max(0.0f, SimulationDelta);

	// Begrenzung gegen Spiralen bei Rucklern (Hitch). Im Zeitraffer sind es bei 30 Bildern je Sekunde
	// gut 45 Schritte – mit 150 Zellen günstig; 6.000 hätten das nicht erlaubt.
	constexpr int32 MaxSteps = 64;
	int32 Steps = FMath::Min(FMath::FloorToInt(StepAccumulator / StepSeconds), MaxSteps);
	if (Steps == MaxSteps)
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
		// Die Hand des Spielers an seiner Zelle: Lenken beim Schwimmen, Kraft beim Bohren
		if (Cells.IsValidIndex(PlayerCellIndex))
		{
			FGenesisSpermCell& Mine = Cells[PlayerCellIndex];
			const bool bSwimming = GenesisFertilizationLogic::GetPhase(Mine) == EGenesisSpermPhase::Swimming;
			Mine.SteerDirection = bSwimming ? GenesisSpermRace::SteerFromInput(Mine.Heading, PlayerSteer, RaceTuning) : FVector::ZeroVector;
			Mine.Vigor = PlayerVigor;
#if !UE_BUILD_SHIPPING
			// Prüfhilfe: lenkt und schlägt wie ein guter Spieler (wie der Testfahrer in Genesis.Conception.Race.SkillDecides)
			if (CVarRaceAutoPilot.GetValueOnGameThread() > 0 && Oocyte)
			{
				Mine.SteerDirection = bSwimming ? (Oocyte->GetState().Position - Mine.Position).GetSafeNormal() : FVector::ZeroVector;
				Mine.Vigor = 1.0f;
				PlayerVigor = 1.0f;
			}
#endif
		}

		if (Oocyte)
		{
			if (GenesisFertilizationLogic::Step(Cells, Oocyte->GetMutableState(), Channel, Tuning, Oocyte->Tuning, StepSeconds, FertilizationResult))
			{
				UE_LOG(LogGenesis, Log, TEXT("Conception: Zelle %d verschmilzt mit der Eizelle nach %.1f s (Vitalität %.2f, %d Mitbewerber an der Zona)."),
					FertilizationResult.CellIndex, FertilizationResult.SecondsToFusion, FertilizationResult.Vitality, FertilizationResult.CompetingCells);
				OnFertilized.Broadcast(FertilizationResult);

				if (IsRacing())
				{
					RaceOutcome = GenesisSpermRace::OutcomeAfterFusion(PlayerCellIndex, FertilizationResult.CellIndex);
					UE_LOG(LogGenesis, Display, TEXT("Conception: %s"), RaceOutcome == EGenesisRaceOutcome::Won
						? TEXT("Die eigene Zelle ist verschmolzen – dieses Leben beginnt.")
						: TEXT("Eine andere Zelle ist verschmolzen."));
				}

				// Aus dem Mikrokosmos wird ein Mensch: Genom, erster Körper, Inkarnation, Leitmotiv.
				// Im Rennen nur, wenn es die eigene Zelle war – sonst beginnt dieses Leben nicht.
				if (bCreateLifeOnFertilization && RaceOutcome != EGenesisRaceOutcome::Lost)
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

	// Im Zeitraffer ist jedes Bild eine eigene kurze Belichtung, wie bei einer echten Zeitrafferaufnahme:
	// keine Bewegungsunschärfe über den Weg, den eine Zelle zwischen zwei Bildern zurücklegt (bis 30 µm)
	const bool bHasPrevious = !bTeleport && !IsTimeLapse() && PreviousTransformBuffer.Num() == Cells.Num();
	TransformBuffer.SetNum(Cells.Num());
	PreviousTransformBuffer.SetNum(Cells.Num());
	CustomDataBuffer.SetNum(Cells.Num() * MaterialDataCount);
	const double JumpThreshold = 0.5 * Channel.LengthUm * GenesisMicroScale::UnitsPerMicrometer;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		const FGenesisSpermCell& Cell = Cells[Index];
		// Anhaftende Zellen stecken mit dem Kopf an oder in der Zona – sie werden anders ausgerichtet als schwimmende
		const bool bAttached = Oocyte && GenesisFertilizationLogic::IsAttached(Cell);
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
	// Die verschmolzene Zelle zuerst – sie ist der Moment, um den es geht. Sonst die führende: im Spalt
	// unter der Zona, sonst die am tiefsten in der Zona steckt; bei Gleichstand (alle noch gebunden) die erste.
	int32 Leader = INDEX_NONE;
	float LeaderDepth = -1.0f;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		const EGenesisSpermPhase Phase = GenesisFertilizationLogic::GetPhase(Cells[Index]);
		if (Phase == EGenesisSpermPhase::Fused)
		{
			return Index;
		}
		if (Phase == EGenesisSpermPhase::Bound || Phase == EGenesisSpermPhase::Penetrating || Phase == EGenesisSpermPhase::Perivitelline)
		{
			// Im Spalt: wer am kürzesten vor der Verschmelzung steht, führt
			const float Depth = Phase == EGenesisSpermPhase::Perivitelline ? 1000.0f - Cells[Index].FusionTimer / 60.0f
				: (Phase == EGenesisSpermPhase::Penetrating ? Cells[Index].PenetrationDepthUm : 0.0f);
			if (Depth > LeaderDepth)
			{
				LeaderDepth = Depth;
				Leader = Index;
			}
		}
	}
	return Leader;
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
			int32 Capacitated = 0;
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
				Capacitated += Cell.bCapacitated ? 1 : 0;
				NearWall += Self->Channel.LumenRadiusUm - FVector2D(Cell.Position.Y, Cell.Position.Z).Size() < Self->Tuning.WallAttractionDistanceUm ? 1 : 0;
				SpeedSum += Cell.Speed;
				UpstreamSum += Cell.Heading.X;
				Backwards += Cell.Heading.X < 0.0 ? 1 : 0;
			}
			const double Count = Self->Cells.Num();
			OutLines.Add(FString::Printf(TEXT("Zellen %d | kapazitiert %d | progressiv %d | hyperaktiviert %d | träge %d | Simulationszeit %.1f s (×%.2f)"),
				Self->Cells.Num(), Capacitated, Progressive, Hyper, Sluggish, Self->SimulationSeconds, Self->GetEffectiveTimeScale()));
			if (Self->Oocyte)
			{
				const FGenesisOocyteState& Egg = Self->Oocyte->GetState();
				OutLines.Add(FString::Printf(TEXT("Eizelle: %s | an der Zona %d | im Spalt %d | Cortikalreaktion %.0f %%"),
					Egg.IsFertilized() ? *FString::Printf(TEXT("befruchtet von Zelle %d nach %.1f s"), Egg.FertilizedByCell, Self->FertilizationResult.SecondsToFusion) : TEXT("unbefruchtet"),
					Egg.BoundCells, Egg.PerivitellineCells, 100.0f * Egg.CorticalReaction));
			}
			OutLines.Add(FString::Printf(TEXT("Ø Vortrieb %.1f µm/s | an der Wand (<%.0f µm) %.0f %% | Ø Ausrichtung gegen den Strom %+.2f | rückwärts %.0f %% | CPU %.3f ms"),
				SpeedSum / Count, Self->Tuning.WallAttractionDistanceUm, 100.0 * NearWall / Count, UpstreamSum / Count,
				100.0 * Backwards / Count, Self->LastTickMs));
		}
	});
#endif
}
