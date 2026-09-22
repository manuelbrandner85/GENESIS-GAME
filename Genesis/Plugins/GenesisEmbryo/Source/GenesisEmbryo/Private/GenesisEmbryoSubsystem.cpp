// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBodySubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisConceptionSubsystem.h"
#include "GenesisDebug.h"
#include "GenesisEmbryoLogic.h"
#include "GenesisEmbryogenesisLogic.h"
#include "GenesisGameplayTags.h"
#include "GenesisLog.h"
#include "GenesisSoulMusicSubsystem.h"
#include "GenesisWorldClockSubsystem.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	UGenesisEmbryoSubsystem* GetEmbryoSubsystem(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisEmbryoAdvanceCommand(
		TEXT("genesis.Embryo.Advance"),
		TEXT("Entwickler: führt den Keim um N Stunden weiter (Standard 24)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEmbryoSubsystem* Embryo = GetEmbryoSubsystem(World))
			{
				Embryo->AdvanceHours(Args.Num() > 0 ? FCString::Atod(*Args[0]) : 24.0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisEmbryoStartCommand(
		TEXT("genesis.Embryo.Start"),
		TEXT("Entwickler: legt einen Keim ohne Zeugung an (Lebenskraft optional, Standard 0,75)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEmbryoSubsystem* Embryo = GetEmbryoSubsystem(World))
			{
				const float Vitality = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.75f;
				Embryo->BeginEmbryo(FGuid::NewGuid(), FGuid::NewGuid(), Vitality, 0.6f);
			}
		}));
}
#endif

void UGenesisEmbryoSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	if (UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>())
	{
		StepHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisEmbryoSubsystem::HandleSimulationStep);
	}

	// Aus der Verschmelzung wird der Keim – ohne Umweg über Gameplay-Code
	if (UGenesisConceptionSubsystem* Conception = Collection.InitializeDependency<UGenesisConceptionSubsystem>())
	{
		ConceivedHandle = Conception->OnConceived.AddUObject(this, &UGenesisEmbryoSubsystem::HandleConceived);
		if (Conception->HasConceived() && !HasEmbryo())
		{
			HandleConceived(Conception->GetRecord());
		}
	}

	RegisterDebugPage();
}

void UGenesisEmbryoSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->OnSimulationStep.Remove(StepHandle);
		}
		if (UGenesisConceptionSubsystem* Conception = GameInstance->GetSubsystem<UGenesisConceptionSubsystem>())
		{
			Conception->OnConceived.Remove(ConceivedHandle);
		}
	}
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Embryo"));
#endif
	Super::Deinitialize();
}

bool UGenesisEmbryoSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisEmbryoSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisEmbryoState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	return true;
}

void UGenesisEmbryoSubsystem::ResetState()
{
	State = FGenesisEmbryoState();
}

void UGenesisEmbryoSubsystem::BeginEmbryo(const FGuid& EntityId, const FGuid& GenomeId, float Vitality, float Resilience)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();

	State = GenesisEmbryoLogic::CreateZygote(EntityId, GenomeId, Vitality, Resilience, Now, Tuning);
	UE_LOG(LogGenesis, Display, TEXT("Embryo: Zygote angelegt (Person %s, Lebenskraft %.2f, erste Teilung in %.1f h)."),
		*EntityId.ToString(EGuidFormats::Short), Vitality, State.Cells.Num() > 0 ? State.Cells[0].HoursToDivision : 0.0f);

	OnStageChanged.Broadcast(State, EGenesisEmbryoStage::Arrested);
}

void UGenesisEmbryoSubsystem::AdvanceHours(double Hours)
{
	if (!HasEmbryo() || Hours <= 0.0)
	{
		return;
	}

	const EGenesisEmbryoStage Previous = State.Stage;
	const EGenesisImplantationPhase PreviousPhase = State.Nidation.Phase;
	const EGenesisEmbryogenesisStage PreviousBodyPlan = State.Embryogenesis.Stage;
	const bool bStageChanged = GenesisEmbryoLogic::Advance(State, Tuning, Hours);

	// Ab der Einnistung baut sich der Körper: Keimblätter, Neuralrohr, Herz
	if (State.Stage == EGenesisEmbryoStage::Implanted)
	{
		const float Day = static_cast<float>(State.HoursSinceFusion) / 24.0f;
		if (GenesisEmbryogenesisLogic::Advance(State.Embryogenesis, EmbryogenesisTuning, Day, MotherNutrition, State.bPlayerEmbryo, State.Seed))
		{
			UE_LOG(LogGenesis, Display, TEXT("Embryo: %s (Tag %.1f, %.1f mm, %d Somiten%s)"),
				*GenesisEmbryogenesisLogic::GetStageName(State.Embryogenesis.Stage), Day, State.Embryogenesis.LengthMm,
				State.Embryogenesis.Somites,
				State.Embryogenesis.bHeartBeating ? *FString::Printf(TEXT(", Herz %.0f/min"), State.Embryogenesis.HeartRateBpm) : TEXT(""));
			if (State.Embryogenesis.Defect != EGenesisNeuralTubeDefect::None && PreviousBodyPlan != State.Embryogenesis.Stage)
			{
				UE_LOG(LogGenesis, Warning, TEXT("Embryo: Neuralrohr – %s (Risiko %.2f %%)"),
					*GenesisEmbryogenesisLogic::GetDefectName(State.Embryogenesis.Defect), 100.0f * State.Embryogenesis.DefectRisk);
			}
		}
	}
	if (State.Nidation.Phase != PreviousPhase)
	{
		UE_LOG(LogGenesis, Display, TEXT("Einnistung: %s (Tag %.1f, Tiefe %.0f µm, Keim %.0f µm, hCG %.1f mIU/ml)"),
			*GenesisEmbryoLogic::GetImplantationPhaseName(State.Nidation.Phase), State.HoursSinceFusion / 24.0,
			State.Nidation.DepthUm, State.Nidation.ConceptusDiameterUm, State.Nidation.HcgMilliIU);
	}
	if (bStageChanged)
	{
		HandleStageChange(Previous);
	}
}

void UGenesisEmbryoSubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	// Nach der Einnistung geht es weiter: dritte und vierte Woche (GENESIS-041), bis der Embryo steht
	if (!HasEmbryo() || State.Stage == EGenesisEmbryoStage::Arrested || IsEmbryogenesisComplete())
	{
		return;
	}

	const double Hours = static_cast<double>(Step.GetDeltaSeconds()) / static_cast<double>(FGenesisTimestamp::SecondsPerHour);
	AdvanceHours(Hours);
}

void UGenesisEmbryoSubsystem::HandleConceived(const FGenesisConceptionRecord& Record)
{
	if (HasEmbryo())
	{
		return;
	}
	BeginEmbryo(Record.EntityId, Record.GenomeId, Record.Vitality, Record.Resilience);
}

void UGenesisEmbryoSubsystem::HandleStageChange(EGenesisEmbryoStage Previous)
{
	UE_LOG(LogGenesis, Display, TEXT("Embryo: %s → %s (%.0f h, %d Zellen, Qualität %.2f)"),
		*GenesisEmbryoLogic::GetStageName(Previous), *GenesisEmbryoLogic::GetStageName(State.Stage),
		State.HoursSinceFusion, State.GetCellCount(), State.Quality);

	UGameInstance* GameInstance = GetGameInstance();

	if (State.Stage == EGenesisEmbryoStage::Implanted && GameInstance)
	{
		// Übergabe an die Körpersimulation: Die Qualität der ersten Woche prägt die Organanlagen.
		// Ein Keim, der sich schlecht geteilt hat, startet mit schlechteren Voraussetzungen ins Leben.
		const float Quality = GenesisEmbryoLogic::GetDevelopmentQuality(State);
		if (UGenesisBodySubsystem* Body = GameInstance->GetSubsystem<UGenesisBodySubsystem>())
		{
			if (FGenesisBodyState* BodyState = Body->FindBodyMutable(State.EntityId))
			{
				for (FGenesisOrganState& Organ : BodyState->Organs)
				{
					Organ.DevelopmentQuality = FMath::Clamp(Organ.DevelopmentQuality * (0.55f + 0.45f * Quality), 0.0f, 1.0f);
				}
				UE_LOG(LogGenesis, Display, TEXT("Embryo: Entwicklungsqualität %.2f an die Organanlagen übergeben."), Quality);
			}
		}

		if (UGenesisSoulMusicSubsystem* Music = GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>())
		{
			const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
			Music->NotifyLifePhase(State.EntityId, GenesisTags::LifePhase_Embryo, Clock ? Clock->GetNow() : FGenesisTimestamp());
		}
	}

	OnStageChanged.Broadcast(State, Previous);
}

void UGenesisEmbryoSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;

	TWeakObjectPtr<UGenesisEmbryoSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Embryo"),
		TEXT("Embryo – die ersten vier Wochen"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisEmbryoSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->HasEmbryo())
			{
				OutLines.Add(TEXT("Noch kein Keim."));
				return;
			}

			const FGenesisEmbryoState& State = Self->GetState();
			OutLines.Add(FString::Printf(TEXT("%s | Tag %.1f (%.0f h) | %d Zellen | Radius %.1f µm"),
				*GenesisEmbryoLogic::GetStageName(State.Stage), State.HoursSinceFusion / 24.0, State.HoursSinceFusion,
				State.GetCellCount(), State.Cells.Num() > 0 ? State.Cells[0].RadiusUm : 0.0f));
			OutLines.Add(FString::Printf(TEXT("Kompaktierung %.0f %% | Hohlraum %.0f %% | Zona %.1f µm | Einnistung %.0f %%"),
				100.0f * State.Compaction, 100.0f * State.Cavity, State.ZonaThicknessUm, 100.0f * State.Implantation));
			const FGenesisEmbryogenesisState& Plan = State.Embryogenesis;
			if (Plan.Stage != EGenesisEmbryogenesisStage::None)
			{
				OutLines.Add(FString::Printf(TEXT("Körperbau: %s | %.1f mm | %d Somiten | Neuralrohr %.0f %% (vorn offen %.0f %%, hinten %.0f %%)"),
					*GenesisEmbryogenesisLogic::GetStageName(Plan.Stage), Plan.LengthMm, Plan.Somites,
					100.0f * Plan.TubeClosure, 100.0f * Plan.RostralNeuropore, 100.0f * Plan.CaudalNeuropore));
				OutLines.Add(FString::Printf(TEXT("Herz %s | Kiemenbögen %d | Augen %.0f %% | Knospen %.0f %% | Defektrisiko %.2f %% (%s)"),
					Plan.bHeartBeating ? *FString::Printf(TEXT("%.0f/min"), Plan.HeartRateBpm) : TEXT("noch still"),
					Plan.PharyngealArches, 100.0f * Plan.OpticVesicles, 100.0f * Plan.LimbBuds,
					100.0f * Plan.DefectRisk, *GenesisEmbryogenesisLogic::GetDefectName(Plan.Defect)));
			}
			OutLines.Add(FString::Printf(TEXT("Qualität %.2f | Fragmentierung %.0f %% | Embryoblast %d Zellen%s"),
				State.Quality, 100.0f * State.Fragmentation, GenesisEmbryoLogic::CountInnerCellMass(State),
				State.Stage == EGenesisEmbryoStage::Arrested
					? *FString::Printf(TEXT(" | Stillstand: %s"), *GenesisEmbryoLogic::GetArrestReasonName(State.ArrestReason))
					: TEXT("")));
			const FGenesisImplantationState& Nid = State.Nidation;
			if (Nid.AppositionAtHours > 0.0f)
			{
				OutLines.Add(FString::Printf(TEXT("Einnistung: %s | Tiefe %.0f µm | Keim %.0f µm | Synzytium %.0f µm | Lakunen %d (Blut %.0f %%)"),
					*GenesisEmbryoLogic::GetImplantationPhaseName(Nid.Phase), Nid.DepthUm, Nid.ConceptusDiameterUm,
					Nid.SyncytiumThicknessUm, Nid.Lacunae, 100.0f * Nid.LacunarBlood));
				OutLines.Add(FString::Printf(TEXT("Keimscheibe %.0f µm: Epiblast %d, Hypoblast %d | Amnion %.0f %% | Dottersack %.0f/%.0f %% | Zotten %d | hCG %.1f mIU/ml | Risiko %.0f %% (Tag %.1f n. ES)"),
					Nid.DiscDiameterUm, Nid.EpiblastCells, Nid.HypoblastCells, 100.0f * Nid.AmnioticCavity,
					100.0f * Nid.PrimaryYolkSac, 100.0f * Nid.SecondaryYolkSac, Nid.PrimaryVilli, Nid.HcgMilliIU,
					100.0f * Nid.EarlyLossRisk, Nid.ImplantationDayPostOvulation));
			}
		}
	});
#endif
}
