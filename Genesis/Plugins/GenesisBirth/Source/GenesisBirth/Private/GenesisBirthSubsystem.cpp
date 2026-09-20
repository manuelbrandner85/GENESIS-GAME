// GENESIS: Der Kreislauf des Lebens

#include "GenesisBirthSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBirthLogic.h"
#include "GenesisBodySubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisDebug.h"
#include "GenesisGameplayTags.h"
#include "GenesisLog.h"
#include "GenesisSoulMusicSubsystem.h"
#include "GenesisWorldClockSubsystem.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	UGenesisBirthSubsystem* GetBirthSubsystem(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisBirthSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisBirthStartCommand(
		TEXT("genesis.Birth.Start"),
		TEXT("Entwickler: startet die Geburt des gezeugten Kindes (oder eines neuen, wenn es keines gibt)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBirthSubsystem* Birth = GetBirthSubsystem(World))
			{
				Birth->BeginLabor(FGuid());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBirthAdvanceCommand(
		TEXT("genesis.Birth.Advance"),
		TEXT("Entwickler: führt die Geburt um N Minuten weiter (Standard 30)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBirthSubsystem* Birth = GetBirthSubsystem(World))
			{
				Birth->AdvanceMinutes(Args.Num() > 0 ? FCString::Atod(*Args[0]) : 30.0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBirthSpeedCommand(
		TEXT("genesis.Birth.Speed"),
		TEXT("Entwickler: Simulationsminuten je Sekunde Echtzeit (0 = nur Weltuhr)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisBirthSubsystem* Birth = GetBirthSubsystem(World))
			{
				Birth->LaborTimeScale = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 5.0f;
				UE_LOG(LogGenesis, Display, TEXT("Geburt: %.1f Simulationsminuten je Sekunde"), Birth->LaborTimeScale);
			}
		}));
}
#endif

void UGenesisBirthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	if (UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>())
	{
		StepHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisBirthSubsystem::HandleSimulationStep);
	}

	// Die Geburt darf schneller laufen als die Weltuhr: Acht Stunden Wehen sind kein Spielinhalt,
	// die letzten Minuten schon.
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UGenesisBirthSubsystem::TickRealTime));

	RegisterDebugPage();
}

bool UGenesisBirthSubsystem::TickRealTime(float DeltaSeconds)
{
	if (HasLabor() && LaborTimeScale > 0.0f && !State.IsBorn())
	{
		AdvanceMinutes(static_cast<double>(DeltaSeconds) * LaborTimeScale);
	}
	return true;
}

void UGenesisBirthSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->OnSimulationStep.Remove(StepHandle);
		}
	}
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Birth"));
#endif
	Super::Deinitialize();
}

bool UGenesisBirthSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisBirthSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisBirthState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	return true;
}

void UGenesisBirthSubsystem::ResetState()
{
	State = FGenesisBirthState();
}

void UGenesisBirthSubsystem::BeginLabor(const FGuid& EntityId)
{
	UGameInstance* GameInstance = GetGameInstance();
	UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;

	// Ohne genannte Person: das erste ungeborene Kind der Welt – im Vertical Slice ist das der Spieler
	FGuid Child = EntityId;
	const FGenesisBodyState* BodyState = Body && Child.IsValid() ? Body->FindBody(Child) : nullptr;
	if (Body && !BodyState)
	{
		for (int32 Index = 0; Index < Body->GetBodyCount(); ++Index)
		{
			const FGenesisBodyState* Candidate = Body->GetBodyByIndex(Index);
			if (Candidate && Candidate->bAlive && !Candidate->bBorn)
			{
				Child = Candidate->EntityId;
				BodyState = Candidate;
				break;
			}
		}
	}

	if (!Child.IsValid())
	{
		UE_LOG(LogGenesis, Warning, TEXT("Geburt: Kein ungeborenes Kind gefunden – zuerst zeugen (genesis.Embryo.Start oder die Befruchtung)."));
		return;
	}

	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();

	// Reife und Lungenreife kommen aus der Körpersimulation – eine Frühgeburt verläuft anders
	float Weeks = 40.0f;
	float LungMaturity = 1.0f;
	if (BodyState)
	{
		Weeks = FMath::Max(20.0f, static_cast<float>(FGenesisTimestamp::YearsBetween(BodyState->ConceptionTime, Now) * 52.1775));
		LungMaturity = BodyState->Organ(EGenesisOrgan::Lungs).Development;
	}

	State = GenesisBirthLogic::BeginLabor(Child, GenesisHash::FromGuid(Child), Weeks, LungMaturity, Now, Tuning);
	UE_LOG(LogGenesis, Display, TEXT("Geburt: Wehen beginnen (Kind %s, %.1f Wochen, Lungenreife %.2f, Erschwernis: %s)."),
		*Child.ToString(EGuidFormats::Short), Weeks, LungMaturity, *GenesisBirthLogic::GetComplicationName(State.Complication));

	OnStageChanged.Broadcast(State, EGenesisLaborStage::NotStarted);
}

void UGenesisBirthSubsystem::AdvanceMinutes(double Minutes)
{
	if (!HasLabor() || Minutes <= 0.0)
	{
		return;
	}

	const EGenesisLaborStage Previous = State.Stage;
	if (GenesisBirthLogic::Advance(State, Tuning, Minutes))
	{
		HandleStageChange(Previous);
	}
}

FGenesisBirthPerception UGenesisBirthSubsystem::GetPerception() const
{
	return GenesisBirthLogic::GetPerception(State);
}

void UGenesisBirthSubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	if (!HasLabor())
	{
		return;
	}

	const double Minutes = static_cast<double>(Step.GetDeltaSeconds()) / 60.0;
	AdvanceMinutes(Minutes);
}

void UGenesisBirthSubsystem::HandleStageChange(EGenesisLaborStage Previous)
{
	UE_LOG(LogGenesis, Display, TEXT("Geburt: %s → %s (%.1f h, Muttermund %.1f cm, Sauerstoff %.2f, Herz %.0f/min)"),
		*GenesisBirthLogic::GetStageName(Previous), *GenesisBirthLogic::GetStageName(State.Stage),
		State.MinutesInLabor / 60.0, State.DilationCm, State.Oxygen, State.HeartRateBpm);

	UGameInstance* GameInstance = GetGameInstance();
	if (State.Stage == EGenesisLaborStage::Delivered && GameInstance)
	{
		// Der Körper wird geboren. Damit hört das Kind ab sofort in Luft statt in Fruchtwasser –
		// das Audio-System leitet das aus dem Körperzustand ab, ohne dass es hier gesagt werden muss.
		if (UGenesisBodySubsystem* Body = GameInstance->GetSubsystem<UGenesisBodySubsystem>())
		{
			Body->Birth(State.EntityId);
		}

		if (UGenesisSoulMusicSubsystem* Music = GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>())
		{
			const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
			Music->NotifyLifePhase(State.EntityId, GenesisTags::LifePhase_Birth, Clock ? Clock->GetNow() : FGenesisTimestamp());
		}

		UE_LOG(LogGenesis, Display, TEXT("Geburt: Kind geboren nach %.1f h, %d Wehen, %.0f min Sauerstoffmangel, erstes Zustandsbild %d/10."),
			State.MinutesInLabor / 60.0, State.ContractionCount, State.HypoxiaMinutes, State.ApgarScore);
	}

	OnStageChanged.Broadcast(State, Previous);
}

void UGenesisBirthSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;

	TWeakObjectPtr<UGenesisBirthSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Birth"),
		TEXT("Geburt – aus der Sicht des Kindes"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisBirthSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->HasLabor())
			{
				OutLines.Add(TEXT("Keine Geburt im Gange (genesis.Birth.Start)."));
				return;
			}

			const FGenesisBirthState& State = Self->GetState();
			const FGenesisBirthPerception Perception = Self->GetPerception();
			OutLines.Add(FString::Printf(TEXT("%s | %.1f h | Muttermund %.1f cm | Tiefertreten %.0f %% | %d Wehen"),
				*GenesisBirthLogic::GetStageName(State.Stage), State.MinutesInLabor / 60.0, State.DilationCm,
				100.0f * State.Descent, State.ContractionCount));
			OutLines.Add(FString::Printf(TEXT("Druck %.2f | Sauerstoff %.2f | Herz %.0f/min | Sauerstoffmangel %.0f min | Belastung %.2f"),
				Perception.Pressure, Perception.Oxygen, Perception.HeartRateBpm, State.HypoxiaMinutes, State.Stress));
			OutLines.Add(FString::Printf(TEXT("Licht %.2f | Dumpfheit %.2f | Enge %.2f | Kälte %.2f%s"),
				Perception.Light, Perception.SoundMuffling, Perception.Tightness, Perception.Cold,
				State.Complication != EGenesisBirthComplication::None
					? *FString::Printf(TEXT(" | %s"), *GenesisBirthLogic::GetComplicationName(State.Complication))
					: TEXT("")));
			if (State.IsBorn())
			{
				OutLines.Add(FString::Printf(TEXT("Geboren | erster Atemzug %s | Zustandsbild %d/10"),
					State.bFirstBreath ? TEXT("getan") : TEXT("steht aus"), State.ApgarScore));
			}
		}
	});
#endif
}
