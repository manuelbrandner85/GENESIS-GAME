// GENESIS: Der Kreislauf des Lebens

#include "GenesisMemorySubsystem.h"
#include "Engine/GameInstance.h"
#include "GenesisDebug.h"
#include "GenesisLog.h"
#include "GenesisWorldClockSubsystem.h"

#include "Engine/World.h"
#include "GenesisGameplayTags.h"
#include "HAL/IConsoleManager.h"

namespace
{
	constexpr uint64 DefaultWorldSeed = 0x47454E45534953ull; // "GENESIS"
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisMemoryLieChainCommand(
		TEXT("genesis.Memory.SimulateLieChain"),
		TEXT("Entwickler: erzeugt die Beispielkette 'Kind lügt → … → Familienkonflikt Jahrzehnte später' inkl. subjektiver Erinnerungen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UGenesisMemorySubsystem* Memory = GameInstance ? GameInstance->GetSubsystem<UGenesisMemorySubsystem>() : nullptr;
			if (!Memory)
			{
				return;
			}

			FGenesisMemoryWorld& MemoryWorld = Memory->GetMemoryWorld();
			const FGuid Child = MemoryWorld.NewEventId();
			const FGuid Parent = MemoryWorld.NewEventId();
			const FGuid Grandchild = MemoryWorld.NewEventId();

			auto Record = [Memory](int64 Year, const FGuid& Actor, const FGameplayTag& Type, const FGameplayTag& Theme, float Magnitude, float Valence, bool bResolved)
			{
				FGenesisCausalEvent Event;
				Event.Time = FGenesisTimestamp::FromCalendar(Year);
				Event.ActorId = Actor;
				Event.EventType = Type;
				Event.Themes.AddTag(Theme);
				Event.Magnitude = Magnitude;
				Event.Valence = Valence;
				Event.bResolved = bResolved;
				const FGenesisCausalEvent* Recorded = Memory->RecordEvent(Event);
				return Recorded ? Recorded->EventId : FGuid();
			};

			const FGuid Lie = Record(1408, Child, GenesisTags::Theme_Deception, GenesisTags::Theme_Deception, 0.4f, -0.3f, true);
			const FGuid Trust = Record(1408, Parent, GenesisTags::Theme_Trust, GenesisTags::Theme_Trust, 0.5f, -0.5f, false);
			const FGuid Fear = Record(1416, Child, GenesisTags::Theme_Fear, GenesisTags::Theme_Fear, 0.6f, -0.4f, false);
			const FGuid Inherit = Record(1440, Grandchild, GenesisTags::Theme_Deception, GenesisTags::Theme_Deception, 0.5f, -0.3f, true);
			const FGuid Conflict = Record(1470, Grandchild, GenesisTags::Theme_Betrayal, GenesisTags::Theme_Betrayal, 0.9f, -0.9f, false);

			MemoryWorld.AddLink(Lie, Trust, EGenesisCausalLinkType::Direct, 0.9f);
			MemoryWorld.AddLink(Trust, Fear, EGenesisCausalLinkType::Contributing, 0.6f);
			MemoryWorld.AddLink(Fear, Inherit, EGenesisCausalLinkType::Inherited, 0.7f);
			MemoryWorld.AddLink(Inherit, Conflict, EGenesisCausalLinkType::Contributing, 0.9f);

			FGenesisEncodingContext ChildView;
			ChildView.Perspective = EGenesisMemoryPerspective::Actor;
			ChildView.Arousal = 0.7f;
			ChildView.SensoryCues.AddTag(GenesisTags::Sense_Smell);
			MemoryWorld.EncodeMemory(Child, Lie, ChildView, FGenesisTimestamp::FromCalendar(1408));

			FGenesisEncodingContext ParentView;
			ParentView.Perspective = EGenesisMemoryPerspective::Target;
			ParentView.Arousal = 0.8f;
			MemoryWorld.EncodeMemory(Parent, Lie, ParentView, FGenesisTimestamp::FromCalendar(1408));

			const TArray<FGenesisCausalTraceEntry> Chain = MemoryWorld.GetGraph().TraceConsequences(Lie, 10, 0.01f);
			for (const FGenesisCausalTraceEntry& Entry : Chain)
			{
				const FGenesisCausalEvent* Event = MemoryWorld.GetGraph().FindEvent(Entry.EventId);
				UE_LOG(LogGenesis, Display, TEXT("Folge Tiefe %d (Stärke %.2f): %s im %s"), Entry.Depth, Entry.PathStrength,
					Event ? *Event->EventType.ToString() : TEXT("?"), Event ? *Event->Time.ToString() : TEXT("?"));
			}
		}));
}
#endif

void UGenesisMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}
	if (UGenesisWorldClockSubsystem* Clock = Collection.InitializeDependency<UGenesisWorldClockSubsystem>())
	{
		ClockHandle = Clock->OnSimulationStep.AddUObject(this, &UGenesisMemorySubsystem::HandleSimulationStep);
	}

	ResetState();

#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisMemorySubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Memory"),
		TEXT("Causal Memory Graph"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisMemorySubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FGenesisCausalGraph& Graph = Self->MemoryWorld.GetGraph();
			int32 TraceCount = 0;
			for (const FGenesisMemoryStore& Store : Self->MemoryWorld.GetStores())
			{
				TraceCount += Store.Traces.Num();
			}
			OutLines.Add(FString::Printf(TEXT("Ereignisse %d | Kanten %d | Gedächtnisse %d | Erinnerungsspuren %d"),
				Graph.NumEvents(), Graph.NumLinks(), Self->MemoryWorld.GetStores().Num(), TraceCount));

			const TArray<FGenesisCausalEvent>& Events = Graph.GetEvents();
			for (int32 Index = Events.Num() - 1; Index >= FMath::Max(0, Events.Num() - 6); --Index)
			{
				const FGenesisCausalEvent& Event = Events[Index];
				OutLines.Add(FString::Printf(TEXT("  #%lld %s %s Mag %.2f Val %+.2f%s"),
					Event.Sequence, *Event.Time.ToString(), *Event.EventType.ToString(), Event.Magnitude, Event.Valence,
					Event.bResolved ? TEXT("") : TEXT(" [offen]")));
			}
		}
	});
#endif
}

void UGenesisMemorySubsystem::Deinitialize()
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
	GenesisDebug::UnregisterPage(TEXT("Memory"));
#endif

	OnEventRecorded.Clear();
	Super::Deinitialize();
}

bool UGenesisMemorySubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(MemoryWorld, OutPayload);
}

bool UGenesisMemorySubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisMemoryWorld Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	MemoryWorld = MoveTemp(Loaded);
	MemoryWorld.RebuildIndices();
	return true;
}

void UGenesisMemorySubsystem::ResetState()
{
	MemoryWorld.Reset(DefaultWorldSeed);
}

void UGenesisMemorySubsystem::ResetWorld(uint64 WorldSeed)
{
	MemoryWorld.Reset(WorldSeed);
}

const FGenesisCausalEvent* UGenesisMemorySubsystem::RecordEvent(const FGenesisCausalEvent& Event)
{
	const FGenesisCausalEvent* Recorded = MemoryWorld.RecordEvent(Event);
	if (Recorded)
	{
		OnEventRecorded.Broadcast(*Recorded);
	}
	return Recorded;
}

int32 UGenesisMemorySubsystem::CompactGraph()
{
	const UGenesisMemorySettings* Settings = GetDefault<UGenesisMemorySettings>();
	const int32 Removed = MemoryWorld.Compact(Settings->CompactionMaxMagnitude, Settings->CompactionMinBridgedStrength);
	UE_LOG(LogGenesis, Log, TEXT("Memory: Verdichtung entfernte %d Ereignisse (verbleibend %d)."), Removed, MemoryWorld.GetGraph().NumEvents());
	return Removed;
}

void UGenesisMemorySubsystem::HandleSimulationStep(const FGenesisSimulationStep& Step)
{
	const UGenesisMemorySettings* Settings = GetDefault<UGenesisMemorySettings>();
	const int64 Interval = FGenesisTimestamp::DaysToSeconds(Settings->DecayIntervalDays);

	// Erster Schritt nach Weltbeginn: Referenzzeit setzen
	if (MemoryWorld.LastDecayTime.Seconds == 0 && Step.From.Seconds != 0)
	{
		MemoryWorld.LastDecayTime = Step.From;
	}

	const int64 Elapsed = Step.To - MemoryWorld.LastDecayTime;
	if (Elapsed < Interval && !Step.bIsTimeSkip)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisMemory_Decay);
	MemoryWorld.DecayAll(static_cast<double>(Elapsed) / static_cast<double>(FGenesisTimestamp::SecondsPerYear), Settings->Dynamics);
	MemoryWorld.LastDecayTime = Step.To;
}
