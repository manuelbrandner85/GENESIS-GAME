// GENESIS: Der Kreislauf des Lebens

#include "GenesisEarlyLifeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBirthSubsystem.h"
#include "GenesisBirthTypes.h"
#include "GenesisDebug.h"
#include "GenesisEarlyLifeLogic.h"
#include "GenesisGameplayTags.h"
#include "GenesisLog.h"
#include "GenesisMemorySubsystem.h"
#include "GenesisRandom.h"
#include "GenesisSoulMusicSubsystem.h"
#include "GenesisSoulSubsystem.h"
#include "GenesisWorldClockSubsystem.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	UGenesisEarlyLifeSubsystem* GetEarlyLife(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornSkinCommand(
		TEXT("genesis.Newborn.SkinToSkin"),
		TEXT("Das Kind auf die Haut der Mutter legen (1) oder ablegen (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->SetSkinToSkin(Args.Num() == 0 || Args[0] != TEXT("0"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornVoiceCommand(
		TEXT("genesis.Newborn.Voice"),
		TEXT("Mit dem Kind sprechen (1) oder schweigen (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->SetMotherSpeaking(Args.Num() == 0 || Args[0] != TEXT("0"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornAdvanceCommand(
		TEXT("genesis.Newborn.Advance"),
		TEXT("Entwickler: führt die erste Stunde um N Minuten weiter (Standard 10)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->AdvanceMinutes(Args.Num() > 0 ? FCString::Atod(*Args[0]) : 10.0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornCryCommand(
		TEXT("genesis.Newborn.Cry"),
		TEXT("Das Kind ruft (0..1). Im Spiel liegt das auf der Leertaste."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->SetCryEffort(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.0f);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornRootCommand(
		TEXT("genesis.Newborn.Root"),
		TEXT("Das Kind sucht die Brust (0..1). Im Spiel liegt das auf E."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->SetRootingEffort(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1.0f);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisNewbornSpeedCommand(
		TEXT("genesis.Newborn.Speed"),
		TEXT("Simulationsminuten je Sekunde Echtzeit (0 = nur Weltuhr)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisEarlyLifeSubsystem* Early = GetEarlyLife(World))
			{
				Early->TimeScale = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 2.0f;
			}
		}));
}
#endif

void UGenesisEarlyLifeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	if (UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>())
	{
		StepHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisEarlyLifeSubsystem::HandleSimulationStep);
	}

	// Die erste Stunde beginnt mit der Geburt – ohne Umweg über Gameplay-Code
	if (UGenesisBirthSubsystem* Birth = Collection.InitializeDependency<UGenesisBirthSubsystem>())
	{
		BirthHandle = Birth->OnStageChanged.AddUObject(this, &UGenesisEarlyLifeSubsystem::HandleBirthStage);
	}

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UGenesisEarlyLifeSubsystem::TickRealTime));
	RegisterDebugPage();
}

void UGenesisEarlyLifeSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->OnSimulationStep.Remove(StepHandle);
		}
		if (UGenesisBirthSubsystem* Birth = GameInstance->GetSubsystem<UGenesisBirthSubsystem>())
		{
			Birth->OnStageChanged.Remove(BirthHandle);
		}
	}
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Newborn"));
#endif
	Super::Deinitialize();
}

bool UGenesisEarlyLifeSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisEarlyLifeSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisNewbornState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	return true;
}

void UGenesisEarlyLifeSubsystem::ResetState()
{
	State = FGenesisNewbornState();
}

void UGenesisEarlyLifeSubsystem::BeginNewborn(const FGuid& EntityId)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();

	// Die Mutter ist im Vertical Slice noch nicht als Person angelegt; die Bindung gilt trotzdem ihr
	State = GenesisEarlyLifeLogic::BeginNewborn(EntityId, FGuid(), GenesisHash::FromGuid(EntityId), Now);
	UE_LOG(LogGenesis, Display, TEXT("Erste Stunde: Kind %s ist da (%.1f °C)."),
		*EntityId.ToString(EGuidFormats::Short), State.BodyTemperature);

	OnStageChanged.Broadcast(State, EGenesisNewbornStage::NotBorn);
}

void UGenesisEarlyLifeSubsystem::AdvanceMinutes(double Minutes)
{
	if (!HasNewborn() || Minutes <= 0.0)
	{
		return;
	}

	const EGenesisNewbornStage Previous = State.Stage;
	if (GenesisEarlyLifeLogic::Advance(State, Tuning, Minutes))
	{
		HandleStageChange(Previous);
	}
}

FGenesisNewbornPerception UGenesisEarlyLifeSubsystem::GetPerception() const
{
	return GenesisEarlyLifeLogic::GetPerception(State, Tuning);
}

void UGenesisEarlyLifeSubsystem::SetSkinToSkin(bool bEnabled)
{
	if (!HasNewborn() || State.bSkinToSkin == bEnabled)
	{
		return;
	}
	State.bSkinToSkin = bEnabled;
	UE_LOG(LogGenesis, Display, TEXT("Erste Stunde: Kind %s auf der Haut der Mutter."), bEnabled ? TEXT("liegt") : TEXT("liegt nicht mehr"));
}

void UGenesisEarlyLifeSubsystem::SetMotherSpeaking(bool bEnabled)
{
	if (HasNewborn())
	{
		State.bMotherSpeaking = bEnabled;
	}
}

void UGenesisEarlyLifeSubsystem::SetEyeContact(bool bEnabled)
{
	if (HasNewborn() && State.bEyeContact != bEnabled)
	{
		State.bEyeContact = bEnabled;
		UE_LOG(LogGenesis, Display, TEXT("Erste Stunde: %s"), bEnabled ? TEXT("Blickkontakt mit der Mutter.") : TEXT("Blickkontakt gelöst."));
	}
}

void UGenesisEarlyLifeSubsystem::SetCryEffort(float Effort)
{
	if (HasNewborn())
	{
		State.CryEffort = FMath::Clamp(Effort, 0.0f, 1.0f);
	}
}

void UGenesisEarlyLifeSubsystem::SetRootingEffort(float Effort)
{
	if (HasNewborn())
	{
		// Suchen kann das Kind nur, wo etwas zu suchen ist
		State.RootingEffort = State.bSkinToSkin ? FMath::Clamp(Effort, 0.0f, 1.0f) : 0.0f;
	}
}

bool UGenesisEarlyLifeSubsystem::TickRealTime(float DeltaSeconds)
{
	if (HasNewborn() && TimeScale > 0.0f)
	{
		AdvanceMinutes(static_cast<double>(DeltaSeconds) * TimeScale);
	}
	return true;
}

void UGenesisEarlyLifeSubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	if (!HasNewborn())
	{
		return;
	}
	AdvanceMinutes(static_cast<double>(Step.GetDeltaSeconds()) / 60.0);
}

void UGenesisEarlyLifeSubsystem::HandleBirthStage(const FGenesisBirthState& BirthState, EGenesisLaborStage Previous)
{
	if (BirthState.IsBorn() && !HasNewborn())
	{
		BeginNewborn(BirthState.EntityId);
	}
}

void UGenesisEarlyLifeSubsystem::HandleStageChange(EGenesisNewbornStage Previous)
{
	UE_LOG(LogGenesis, Display, TEXT("Erste Stunde: %s → %s (%.0f min, %.1f °C, Ruhe %.2f, Bindung %.2f)"),
		*GenesisEarlyLifeLogic::GetStageName(Previous), *GenesisEarlyLifeLogic::GetStageName(State.Stage),
		State.MinutesSinceBirth, State.BodyTemperature, State.Calm, State.Bonding);

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	// Mit dem ersten Anlegen ist die Geburt abgeschlossen: Ab hier ist der Mensch ein Kind
	if (State.Stage == EGenesisNewbornStage::FirstFeed)
	{
		EncodeFirstMemory();

		if (UGenesisSoulMusicSubsystem* Music = GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>())
		{
			const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
			Music->NotifyLifePhase(State.EntityId, GenesisTags::LifePhase_Childhood, Clock ? Clock->GetNow() : FGenesisTimestamp());
		}
	}

	OnStageChanged.Broadcast(State, Previous);
}

void UGenesisEarlyLifeSubsystem::EncodeFirstMemory()
{
	if (State.bFirstMemoryEncoded)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisMemorySubsystem* Memory = GameInstance ? GameInstance->GetSubsystem<UGenesisMemorySubsystem>() : nullptr;
	if (!Memory)
	{
		return;
	}

	const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();

	// Die erste Erinnerung eines Lebens ist kein Bild und kein Satz. Sie ist Wärme, ein Herzschlag
	// und eine Stimme, die das Kind schon kennt – abgelegt über Geruch, Tasten und Hören.
	FGenesisCausalEvent Event;
	Event.Time = Now;
	Event.EventType = GenesisTags::LifePhase_Birth;
	Event.Themes.AddTag(GenesisTags::Theme_Care);
	Event.Themes.AddTag(GenesisTags::Theme_Belonging);
	Event.ActorId = State.EntityId;
	Event.Magnitude = 0.85f;
	Event.Valence = FMath::Clamp(State.Bonding, 0.0f, 1.0f);

	const FGenesisCausalEvent* Recorded = Memory->RecordEvent(Event);
	if (!Recorded)
	{
		return;
	}

	FGenesisEncodingContext Context;
	// Das Kind erlebt diese Stunde nicht als Handelnder, sondern als der, dem sie widerfährt
	Context.Perspective = EGenesisMemoryPerspective::Target;
	// Ein Neugeborenes hat keine Worte, aber die Sinne sind offen – und diese Stunde prägt sich ein
	Context.Attention = FMath::Clamp(0.5f + 0.5f * State.Calm, 0.0f, 1.0f);
	Context.Stress = FMath::Clamp(1.0f - State.Calm, 0.0f, 1.0f);
	Context.Arousal = 0.7f;
	Context.Mood = FMath::Clamp(State.Bonding * 2.0f - 0.2f, -1.0f, 1.0f);
	Context.SensoryCues.AddTag(GenesisTags::Sense_Touch);
	Context.SensoryCues.AddTag(GenesisTags::Sense_Smell);
	Context.SensoryCues.AddTag(GenesisTags::Sense_Hearing);

	const FGuid TraceId = Memory->GetMemoryWorld().EncodeMemory(State.EntityId, Recorded->EventId, Context, Now);
	State.bFirstMemoryEncoded = TraceId.IsValid();

	// Die Seele nimmt mit, was hier geschehen ist: Geborgenheit als Muster, das ein Leben lang zieht
	if (UGenesisSoulSubsystem* Soul = GameInstance->GetSubsystem<UGenesisSoulSubsystem>())
	{
		if (Soul->HasPlayerSoul())
		{
			Soul->ReinforcePlayerResonance(GenesisTags::Theme_Care, State.Bonding);
		}
	}

	UE_LOG(LogGenesis, Display, TEXT("Erste Stunde: erste Erinnerung eines Lebens abgelegt (Bindung %.2f, Stimmung %+.2f)."),
		State.Bonding, Context.Mood);
}

void UGenesisEarlyLifeSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;

	TWeakObjectPtr<UGenesisEarlyLifeSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Newborn"),
		TEXT("Die ersten Minuten"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisEarlyLifeSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->HasNewborn())
			{
				OutLines.Add(TEXT("Noch kein Kind auf der Welt."));
				return;
			}

			const FGenesisNewbornState& State = Self->GetState();
			const FGenesisNewbornPerception Perception = Self->GetPerception();
			OutLines.Add(FString::Printf(TEXT("%s | %.0f min | %.1f °C | %s | %s"),
				*GenesisEarlyLifeLogic::GetStageName(State.Stage), State.MinutesSinceBirth, State.BodyTemperature,
				State.bSkinToSkin ? TEXT("auf der Haut") : TEXT("abgelegt"),
				State.bMotherSpeaking ? TEXT("jemand spricht") : TEXT("still")));
			OutLines.Add(FString::Printf(TEXT("Ruhe %.2f | Hunger %.2f | Bindung %.2f | geschrien %.0f min | %s"),
				State.Calm, State.Hunger, State.Bonding, State.CryingMinutes,
				State.bHasFed ? TEXT("hat getrunken") : TEXT("noch nicht getrunken")));
			OutLines.Add(FString::Printf(TEXT("Sicht: %.0f mm scharf, Schärfe %.3f | Blendung %.2f | Wärme %.2f | Stimme vertraut %.2f%s"),
				Perception.FocusDistanceMm, Perception.VisualAcuity, Perception.Glare, Perception.Warmth,
				Perception.VoiceFamiliarity, State.bFirstMemoryEncoded ? TEXT(" | erste Erinnerung abgelegt") : TEXT("")));
		}
	});
#endif
}
