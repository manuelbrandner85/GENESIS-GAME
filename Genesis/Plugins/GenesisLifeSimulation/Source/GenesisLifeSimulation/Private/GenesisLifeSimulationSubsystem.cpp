// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisGameplayTags.h"
#include "GenesisGeneticsSubsystem.h"
#include "GenesisLifeSimulationEngine.h"
#include "GenesisLog.h"
#include "GenesisMemorySubsystem.h"
#include "GenesisWorldClockSubsystem.h"
#include "HAL/IConsoleManager.h"

namespace
{
	constexpr uint64 DefaultWorldSeed = 0x47454E45534953ull; // "GENESIS"
}

void UGenesisLifeSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>();
	UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>();
	UGenesisMemorySubsystem* Memory = Collection.InitializeDependency<UGenesisMemorySubsystem>();
	UGenesisGeneticsSubsystem* Genetics = Collection.InitializeDependency<UGenesisGeneticsSubsystem>();

	Engine = NewObject<UGenesisLifeSimulationEngine>(this);
	Engine->Initialize(
		Memory ? &Memory->GetMemoryWorld() : nullptr,
		Genetics ? &Genetics->GetPool() : nullptr,
		Genetics ? &Genetics->GetTraitDefinitions() : nullptr,
		GetDefault<UGenesisLifeSimulationSettings>()->Tuning);
	Engine->AddDefaultProcessors();
	Engine->OnConsequenceTriggered.AddUObject(this, &UGenesisLifeSimulationSubsystem::HandleConsequenceTriggered);

	if (Registry)
	{
		Registry->RegisterSystem(this);
	}
	if (Clock)
	{
		ClockHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisLifeSimulationSubsystem::HandleSimulationStep);
	}

	ResetState();
	RegisterDebugPage();
}

void UGenesisLifeSimulationSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* Registry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			Registry->UnregisterSystem(this);
		}
		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->OnSimulationStep.Remove(ClockHandle);
		}
	}

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Life"));
#endif

	Super::Deinitialize();
}

bool UGenesisLifeSimulationSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return Engine && GenesisPersistence::Write(Engine->GetState(), OutPayload);
}

bool UGenesisLifeSimulationSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisLifeSimulationState Loaded;
	if (!Engine || !GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	Engine->GetState() = MoveTemp(Loaded);
	Engine->GetState().RebuildIndices();
	return true;
}

void UGenesisLifeSimulationSubsystem::ResetState()
{
	if (Engine)
	{
		Engine->ResetState(DefaultWorldSeed);
	}
}

FGuid UGenesisLifeSimulationSubsystem::ReportAction(const FGenesisLifeAction& Action)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();

	const FGuid EventId = Engine ? Engine->ProcessAction(Action, Now) : FGuid();

	// Beobachter des Gedächtnisses (Story Director, Cinematics …) informieren
	if (UGenesisMemorySubsystem* Memory = GameInstance ? GameInstance->GetSubsystem<UGenesisMemorySubsystem>() : nullptr)
	{
		if (const FGenesisCausalEvent* Event = Memory->GetMemoryWorld().GetGraph().FindEvent(EventId))
		{
			Memory->NotifyEventRecorded(*Event);
		}
	}
	return EventId;
}

void UGenesisLifeSimulationSubsystem::RegisterEntity(const FGenesisLifeProfile& Profile)
{
	if (Engine)
	{
		Engine->RegisterEntity(Profile);
	}
}

void UGenesisLifeSimulationSubsystem::SetSimulationLevel(const FGuid& EntityId, EGenesisSimulationLevel Level)
{
	if (FGenesisLifeProfile* Profile = Engine ? Engine->GetState().FindProfile(EntityId) : nullptr)
	{
		Profile->SimulationLevel = Level;
	}
}

void UGenesisLifeSimulationSubsystem::SetZeitgeist(const TArray<FGenesisWeightedTag>& DominantValues)
{
	if (Engine)
	{
		Engine->GetState().Zeitgeist.DominantValues = DominantValues;
	}
}

void UGenesisLifeSimulationSubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	if (Engine)
	{
		Engine->AdvanceTime(Step);
	}
}

void UGenesisLifeSimulationSubsystem::HandleConsequenceTriggered(const FGenesisScheduledConsequence& Consequence, const FGuid& EventId)
{
	UE_LOG(LogGenesis, Log, TEXT("LifeSimulation: Konsequenz %s ausgelöst (Ursache %s)."),
		*Consequence.Spec.ConsequenceType.ToString(), *Consequence.SourceEventId.ToString(EGuidFormats::Short));

	const UGameInstance* GameInstance = GetGameInstance();
	if (UGenesisMemorySubsystem* Memory = GameInstance ? GameInstance->GetSubsystem<UGenesisMemorySubsystem>() : nullptr)
	{
		if (const FGenesisCausalEvent* Event = Memory->GetMemoryWorld().GetGraph().FindEvent(EventId))
		{
			Memory->NotifyEventRecorded(*Event);
		}
	}
}

void UGenesisLifeSimulationSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisLifeSimulationSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Life"),
		TEXT("Life Simulation (verborgene Werte)"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisLifeSimulationSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Engine)
			{
				return;
			}

			const FGenesisLifeSimulationState& State = Self->Engine->GetState();
			OutLines.Add(FString::Printf(TEXT("Personen %d | Vertrauenskanten %d | Rufeinträge %d | Gerüchte %d | geplante Folgen %d | Systeme %d"),
				State.Profiles.Num(), State.TrustEdges.Num(), State.Reputation.Num(), State.Rumors.Num(),
				State.PendingConsequences.Num(), Self->Engine->GetProcessors().Num()));

			for (int32 Index = 0; Index < FMath::Min(State.Profiles.Num(), 4); ++Index)
			{
				const FGenesisLifeProfile& Profile = State.Profiles[Index];
				const FGenesisKarmaVector& Karma = Profile.Karma.Values;
				OutLines.Add(FString::Printf(TEXT("  %s L%d | Karma Mit %+.1f Ehr %+.1f Mut %+.1f Groß %+.1f Weis %+.1f Lie %+.1f | Altruismus %+.2f Stress %.2f Dissonanz %.2f"),
					*Profile.EntityId.ToString(EGuidFormats::Short).Left(8), static_cast<int32>(Profile.SimulationLevel) + 1,
					Karma.Compassion, Karma.Honesty, Karma.Courage, Karma.Generosity, Karma.Wisdom, Karma.Love,
					Profile.Altruism, Profile.Stress, Profile.Dissonance));
			}

			for (int32 Index = 0; Index < FMath::Min(State.TrustEdges.Num(), 4); ++Index)
			{
				const FGenesisTrustEdge& Edge = State.TrustEdges[Index];
				OutLines.Add(FString::Printf(TEXT("  Vertrauen %s → %s: %+.2f (Vertrautheit %.2f, Verrat x%d)"),
					*Edge.FromId.ToString(EGuidFormats::Short).Left(8), *Edge.ToId.ToString(EGuidFormats::Short).Left(8),
					Edge.Trust, Edge.Familiarity, Edge.Betrayals));
			}

			for (const FGenesisRumor& Rumor : State.Rumors)
			{
				OutLines.Add(FString::Printf(TEXT("  Gerücht über %s: Mitwisser %d, Wahrheit %.2f, Reiz %.2f, Weitergaben %d"),
					*Rumor.SubjectId.ToString(EGuidFormats::Short).Left(8), Rumor.Holders.Num(), Rumor.Truth, Rumor.Juiciness, Rumor.Hops));
			}
		}
	});
#endif
}

#if !UE_BUILD_SHIPPING
namespace
{
	UGenesisLifeActionDefinition* MakeScenarioAction(const FGameplayTag& Type, float Magnitude, float Valence)
	{
		UGenesisLifeActionDefinition* Definition = NewObject<UGenesisLifeActionDefinition>(GetTransientPackage());
		Definition->ActionType = Type;
		Definition->Themes.AddTag(Type);
		Definition->BaseMagnitude = Magnitude;
		Definition->Valence = Valence;
		Definition->Visibility = 1.0f;
		Definition->Ambiguity = 0.1f;
		return Definition;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisLifeScenarioCommand(
		TEXT("genesis.Life.SimulateScenario"),
		TEXT("Entwickler: Kind belügt Mutter vor Geschwister, predigt später Ehrlichkeit; 2 Jahre Zeitsprung (Konsequenz, Gerücht, Epigenetik)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UGenesisLifeSimulationSubsystem* Life = GameInstance ? GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>() : nullptr;
			UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
			UGenesisGeneticsSubsystem* Genetics = GameInstance ? GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
			if (!Life || !Clock || !Life->GetEngine())
			{
				return;
			}

			FGenesisRandomStream& Rng = Life->GetEngine()->GetState().Rng;
			const FGuid Child = Rng.NewGuid();
			const FGuid Mother = Rng.NewGuid();
			const FGuid Sibling = Rng.NewGuid();
			const FGuid Friend = Rng.NewGuid();

			auto Register = [Life, Genetics](const FGuid& Id, EGenesisSimulationLevel Level)
			{
				FGenesisLifeProfile Profile;
				Profile.EntityId = Id;
				Profile.SimulationLevel = Level;
				Profile.GenomeId = Genetics ? Genetics->CreateFounderGenome() : FGuid();
				Life->RegisterEntity(Profile);
			};
			Register(Child, EGenesisSimulationLevel::Full);
			Register(Mother, EGenesisSimulationLevel::Full);
			Register(Sibling, EGenesisSimulationLevel::Reduced);
			Register(Friend, EGenesisSimulationLevel::Reduced);

			// Gewachsene Beziehungen
			FGenesisLifeSimulationState& State = Life->GetEngine()->GetState();
			FGenesisTrustEdge& MotherTrust = State.FindOrAddTrust(Mother, Child);
			MotherTrust.Trust = 0.7f;
			MotherTrust.Familiarity = 0.9f;
			FGenesisTrustEdge& FriendTrust = State.FindOrAddTrust(Friend, Sibling);
			FriendTrust.Trust = 0.5f;
			FriendTrust.Familiarity = 0.8f;

			// 1) Lüge
			UGenesisLifeActionDefinition* Lie = MakeScenarioAction(GenesisTags::Theme_Deception, 0.5f, -0.5f);
			Lie->KarmaImpulse.Honesty = -8.0f;
			Lie->TrustImpact = -0.6f;
			Lie->SelfBenefit = 0.5f;
			Lie->OthersBenefit = -0.2f;
			Lie->StressLoad = 0.3f;
			Lie->ViolatesValues.AddTag(GenesisTags::Theme_Honesty);
			FGenesisDelayedConsequenceSpec Distrust;
			Distrust.ConsequenceType = GenesisTags::Theme_Trust;
			Distrust.Themes.AddTag(GenesisTags::Theme_Trust);
			Distrust.Probability = 1.0f;
			Distrust.MinDelayYears = 1.0f;
			Distrust.MaxDelayYears = 1.5f;
			Distrust.Valence = -0.4f;
			Distrust.bOpensThread = true;
			Lie->DelayedConsequences.Add(Distrust);

			FGenesisLifeAction LieAction;
			LieAction.Definition = Lie;
			LieAction.ActorId = Child;
			LieAction.TargetIds.Add(Mother);
			LieAction.WitnessIds.Add(Sibling);
			Life->ReportAction(LieAction);

			// 2) Öffentliches Eintreten für Ehrlichkeit (Doppelmoral)
			UGenesisLifeActionDefinition* Preach = MakeScenarioAction(GenesisTags::Theme_Honesty, 0.3f, 0.1f);
			Preach->ExpressesValues.AddTag(GenesisTags::Theme_Honesty);
			Preach->bIsAdvocacy = true;
			FGenesisLifeAction PreachAction;
			PreachAction.Definition = Preach;
			PreachAction.ActorId = Child;
			PreachAction.WitnessIds.Add(Mother);
			PreachAction.WitnessIds.Add(Sibling);
			Life->ReportAction(PreachAction);

			// 3) Zwei Jahre vergehen
			Clock->SkipTime(FGenesisTimestamp::YearsToSeconds(2.0));
			UE_LOG(LogGenesis, Display, TEXT("Szenario: Kind %s, Mutter %s, Geschwister %s, Freund %s"),
				*Child.ToString(EGuidFormats::Short).Left(8), *Mother.ToString(EGuidFormats::Short).Left(8),
				*Sibling.ToString(EGuidFormats::Short).Left(8), *Friend.ToString(EGuidFormats::Short).Left(8));
		}));
}
#endif
