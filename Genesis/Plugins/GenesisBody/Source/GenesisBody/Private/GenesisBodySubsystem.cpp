// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBodyGameplayTags.h"
#include "GenesisBodyLogic.h"
#include "GenesisDebug.h"
#include "GenesisGeneticsSubsystem.h"
#include "GenesisLifeSimulationEngine.h"
#include "GenesisLifeSimulationSubsystem.h"
#include "GenesisLog.h"
#include "GenesisWorldClockSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UGenesisBodySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* PersistenceRegistry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		PersistenceRegistry->RegisterSystem(this);
	}
	if (UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>())
	{
		ClockHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisBodySubsystem::HandleSimulationStep);
	}
	Collection.InitializeDependency<UGenesisGeneticsSubsystem>();
	Collection.InitializeDependency<UGenesisLifeSimulationSubsystem>();

	RegisterDebugPage();
}

void UGenesisBodySubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* PersistenceRegistry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			PersistenceRegistry->UnregisterSystem(this);
		}
		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->OnSimulationStep.Remove(ClockHandle);
		}
	}

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Body"));
#endif

	OnVitalFailure.Clear();
	Super::Deinitialize();
}

bool UGenesisBodySubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(Registry, OutPayload);
}

bool UGenesisBodySubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisBodyRegistry Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	Registry = MoveTemp(Loaded);
	RebuildIndex();
	return true;
}

void UGenesisBodySubsystem::ResetState()
{
	Registry = FGenesisBodyRegistry();
	Index.Reset();
}

void UGenesisBodySubsystem::RebuildIndex()
{
	Index.Reset();
	for (int32 BodyIndex = 0; BodyIndex < Registry.Bodies.Num(); ++BodyIndex)
	{
		Index.Add(Registry.Bodies[BodyIndex].EntityId, BodyIndex);
	}
}

FGenesisTimestamp UGenesisBodySubsystem::GetNow() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	return Clock ? Clock->GetNow() : FGenesisTimestamp();
}

float UGenesisBodySubsystem::GetPsychologicalStress(const FGuid& EntityId) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisLifeSimulationSubsystem* Life = GameInstance ? GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>() : nullptr;
	const FGenesisLifeProfile* Profile = Life && Life->GetEngine() ? Life->GetEngine()->GetState().FindProfile(EntityId) : nullptr;
	return Profile ? Profile->Stress : 0.0f;
}

const FGenesisBodyState* UGenesisBodySubsystem::FindBody(const FGuid& EntityId) const
{
	const int32* Found = Index.Find(EntityId);
	return Found ? &Registry.Bodies[*Found] : nullptr;
}

FGenesisBodyState* UGenesisBodySubsystem::FindBodyMutable(const FGuid& EntityId)
{
	const int32* Found = Index.Find(EntityId);
	return Found ? &Registry.Bodies[*Found] : nullptr;
}

void UGenesisBodySubsystem::CreateBodyAtConception(const FGuid& EntityId, const FGuid& GenomeId, const FGenesisConceptionVitality& Vitality, EGenesisSimulationLevel Level)
{
	if (!EntityId.IsValid() || FindBody(EntityId))
	{
		UE_LOG(LogGenesis, Warning, TEXT("Body: Körper für %s existiert bereits oder ungültige ID."), *EntityId.ToString(EGuidFormats::Short));
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisGeneticsSubsystem* Genetics = GameInstance ? GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
	const FGenesisBodyGenetics BodyGenetics = Genetics
		? GenesisBodyLogic::MakeGenetics(Genetics->FindGenome(GenomeId), Genetics->GetTraitDefinitions())
		: FGenesisBodyGenetics();

	const int32 NewIndex = Registry.Bodies.Add(GenesisBodyLogic::CreateAtConception(EntityId, GenomeId, BodyGenetics, Vitality, GetNow(), Level));
	Index.Add(EntityId, NewIndex);
}

void UGenesisBodySubsystem::Birth(const FGuid& EntityId)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		GenesisBodyLogic::Birth(*Body, GetNow(), GetDefault<UGenesisBodySettings>()->Tuning);
	}
}

void UGenesisBodySubsystem::SetActivity(const FGuid& EntityId, EGenesisActivity Activity)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		Body->Activity = Activity;
	}
}

void UGenesisBodySubsystem::ApplyInjury(const FGuid& EntityId, FGameplayTag Region, float Severity)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		GenesisBodyLogic::ApplyInjury(*Body, Region, Severity, GetNow());
	}
}

void UGenesisBodySubsystem::ApplyAcuteStressor(const FGuid& EntityId, float Intensity)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		GenesisBodyLogic::ApplyAcuteStressor(*Body, Intensity);
	}
}

void UGenesisBodySubsystem::ApplyBonding(const FGuid& EntityId, float Intensity)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		GenesisBodyLogic::ApplyBonding(*Body, Intensity);
	}
}

void UGenesisBodySubsystem::Eat(const FGuid& EntityId, float Quality)
{
	if (FGenesisBodyState* Body = FindBodyMutable(EntityId))
	{
		GenesisBodyLogic::Eat(*Body, Quality);
	}
}

TArray<FGenesisSymptom> UGenesisBodySubsystem::GetSymptoms(const FGuid& EntityId) const
{
	const FGenesisBodyState* Body = FindBody(EntityId);
	return Body ? GenesisBodyLogic::DeriveSymptoms(*Body, GetNow()) : TArray<FGenesisSymptom>();
}

EGenesisDevelopmentStage UGenesisBodySubsystem::GetDevelopmentStage(const FGuid& EntityId) const
{
	const FGenesisBodyState* Body = FindBody(EntityId);
	return Body ? GenesisBodyLogic::GetStage(*Body, GetNow()) : EGenesisDevelopmentStage::Deceased;
}

float UGenesisBodySubsystem::GetSenseAcuity(const FGuid& EntityId, EGenesisBodySense Sense) const
{
	const FGenesisBodyState* Body = FindBody(EntityId);
	return Body ? Body->Sense(Sense).Acuity : 0.0f;
}

void UGenesisBodySubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisBody_Step);

	const FGenesisBodyTuning& Tuning = GetDefault<UGenesisBodySettings>()->Tuning;
	const double Hours = static_cast<double>(Step.GetDeltaSeconds()) / static_cast<double>(FGenesisTimestamp::SecondsPerHour);

	for (FGenesisBodyState& Body : Registry.Bodies)
	{
		if (!Body.bAlive)
		{
			continue;
		}

		const float Stress = GetPsychologicalStress(Body.EntityId);

		// Level 1: stündliche Physiologie (nicht bei Zeitsprüngen – dort nur aggregierte Entwicklung)
		if (Body.SimulationLevel == EGenesisSimulationLevel::Full && !Step.bIsTimeSkip && Hours <= 48.0)
		{
			GenesisBodyLogic::AdvanceHours(Body, Step.To, Hours, Stress);
		}
		else
		{
			Body.HoursSinceDailyUpdate += static_cast<float>(Hours);
		}

		if (Body.HoursSinceDailyUpdate >= 24.0f || Step.bIsTimeSkip)
		{
			if (GenesisBodyLogic::AdvanceDays(Body, Step.To, Body.HoursSinceDailyUpdate / 24.0, Stress, Tuning))
			{
				UE_LOG(LogGenesis, Log, TEXT("Body: Vitalfunktionen von %s versagen (%s)."), *Body.EntityId.ToString(EGuidFormats::Short), *Body.DeathTime.ToString());
				OnVitalFailure.Broadcast(Body.EntityId, Body.DeathTime);
			}
		}

		// Nach einem Zeitsprung Vitalwerte am neuen Zeitpunkt einmal einschwingen – sonst zeigen Herz/Atem
		// bis zum nächsten regulären Stundenschritt den Stand von vor dem Sprung
		if (Step.bIsTimeSkip && Body.bAlive && Body.SimulationLevel == EGenesisSimulationLevel::Full)
		{
			GenesisBodyLogic::AdvanceHours(Body, Step.To, 1.0, Stress);
		}
	}
}

void UGenesisBodySubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisBodySubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Body"),
		TEXT("Body Simulation"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisBodySubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FGenesisBodyState* Body = Self->Registry.Bodies.FindByPredicate([](const FGenesisBodyState& Candidate)
			{
				return Candidate.SimulationLevel == EGenesisSimulationLevel::Full;
			});
			OutLines.Add(FString::Printf(TEXT("Körper: %d"), Self->Registry.Bodies.Num()));
			if (!Body)
			{
				return;
			}

			const FGenesisTimestamp Now = Self->GetNow();
			const UEnum* StageEnum = StaticEnum<EGenesisDevelopmentStage>();
			OutLines.Add(FString::Printf(TEXT("Stufe %s | SSW %.1f | Alter %.2f J | biologisch %.2f J | %.0f cm %.1f kg | Fitness %.2f | Schlafschuld %.1f h"),
				*StageEnum->GetNameStringByValue(static_cast<int64>(GenesisBodyLogic::GetStage(*Body, Now))),
				GenesisBodyLogic::GetGestationalWeeks(*Body, Now), GenesisBodyLogic::GetAgeYears(*Body, Now), Body->BiologicalAgeYears,
				Body->HeightCm, Body->WeightKg, Body->Fitness, Body->SleepDebtHours));

			const FGenesisVitalState& Vitals = Body->Vitals;
			OutLines.Add(FString::Printf(TEXT("Puls %.0f | Atmung %.0f | O2 %.0f%% | %.1f °C | Energie %.2f | Schlafdruck %.2f | Hunger %.2f | Wasser %.2f"),
				Vitals.HeartRate, Vitals.RespiratoryRate, Vitals.BloodOxygen, Vitals.BodyTemperature, Vitals.Energy, Vitals.SleepPressure, Vitals.Hunger, Vitals.Hydration));

			const FGenesisHormoneState& Hormones = Body->Hormones;
			OutLines.Add(FString::Printf(TEXT("Cortisol %.2f | Adrenalin %.2f | Oxytocin %.2f | Melatonin %.2f | Wachstum %.2f | Sexualhormone %.2f"),
				Hormones.Cortisol, Hormones.Adrenaline, Hormones.Oxytocin, Hormones.Melatonin, Hormones.GrowthHormone, Hormones.SexHormones));

			const UEnum* OrganEnum = StaticEnum<EGenesisOrgan>();
			FString Organs;
			for (int32 OrganIndex = 0; OrganIndex < Body->Organs.Num(); ++OrganIndex)
			{
				const FGenesisOrganState& Organ = Body->Organs[OrganIndex];
				Organs += FString::Printf(TEXT("%s %.2f/%.2f  "), *OrganEnum->GetNameStringByValue(OrganIndex), Organ.Development, Organ.GetHealth());
			}
			OutLines.Add(TEXT("Organe (Entwicklung/Gesundheit): ") + Organs);

			const UEnum* SenseEnum = StaticEnum<EGenesisBodySense>();
			FString Senses;
			for (int32 SenseIndex = 0; SenseIndex < Body->Senses.Num(); ++SenseIndex)
			{
				Senses += FString::Printf(TEXT("%s %.2f  "), *SenseEnum->GetNameStringByValue(SenseIndex), Body->Senses[SenseIndex].Acuity);
			}
			OutLines.Add(TEXT("Sinne (Schärfe): ") + Senses);

			FString Symptoms;
			for (const FGenesisSymptom& Symptom : GenesisBodyLogic::DeriveSymptoms(*Body, Now))
			{
				Symptoms += FString::Printf(TEXT("%s %.2f  "), *Symptom.Symptom.GetTagName().ToString().RightChop(16), Symptom.Intensity);
			}
			OutLines.Add(TEXT("Erlebte Symptome: ") + (Symptoms.IsEmpty() ? FString(TEXT("keine")) : Symptoms));

			for (const FGenesisBodyCondition& Condition : Body->Conditions)
			{
				OutLines.Add(FString::Printf(TEXT("  Zustand %s %.2f"), *Condition.Condition.ToString(), Condition.Severity));
			}
			if (Body->Scars.Num() > 0)
			{
				OutLines.Add(FString::Printf(TEXT("  Narben: %d"), Body->Scars.Num()));
			}
		}
	});
#endif
}

#if !UE_BUILD_SHIPPING
namespace
{
	UGenesisBodySubsystem* GetBodySubsystem(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	}

	/** Entwicklerkörper – bewusst feste ID, damit Befehle aufeinander aufbauen können. */
	const FGuid DevBodyId(0x6E, 0x5E5, 0xB0D1, 0x1);

	FAutoConsoleCommandWithWorldAndArgs GenesisBodyConceiveCommand(
		TEXT("genesis.Body.Conceive"),
		TEXT("Entwickler: Zwei Gründergenome, Befruchtung, neuer Körper (Level 1). Optional: Vitalität 0..1."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGenesisBodySubsystem* Body = GetBodySubsystem(World);
			UGenesisGeneticsSubsystem* Genetics = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
			if (!Body || !Genetics)
			{
				return;
			}
			const FGuid Child = Genetics->ConceiveChild(Genetics->CreateFounderGenome(), Genetics->CreateFounderGenome());
			FGenesisConceptionVitality Vitality;
			Vitality.Vitality = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.7f;
			Body->CreateBodyAtConception(DevBodyId, Child, Vitality, EGenesisSimulationLevel::Full);
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBodyBirthCommand(
		TEXT("genesis.Body.Birth"),
		TEXT("Entwickler: Geburt des Entwicklerkörpers."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBodySubsystem* Body = GetBodySubsystem(World))
			{
				Body->Birth(DevBodyId);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBodyActivityCommand(
		TEXT("genesis.Body.Activity"),
		TEXT("Entwickler: Aktivität des Entwicklerkörpers (0 Schlaf, 1 Ruhe, 2 leicht, 3 mittel, 4 intensiv)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBodySubsystem* Body = GetBodySubsystem(World))
			{
				const int32 Value = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 0, 4) : 1;
				Body->SetActivity(DevBodyId, static_cast<EGenesisActivity>(Value));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBodyInjureCommand(
		TEXT("genesis.Body.Injure"),
		TEXT("Entwickler: Verletzung am Arm des Entwicklerkörpers. Optional: Schwere 0..1."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBodySubsystem* Body = GetBodySubsystem(World))
			{
				Body->ApplyInjury(DevBodyId, GenesisBodyTags::Region_Arm, Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.6f);
			}
		}));
}
#endif
