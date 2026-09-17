// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationEngine.h"
#include "GenesisLifeSimulationProcessors.h"
#include "GenesisLog.h"
#include "GenesisMemoryWorld.h"
#include "GenesisWorldClockSubsystem.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UGenesisLifeSimulationEngine::Initialize(FGenesisMemoryWorld* InMemory, FGenesisGenomePool* InGenomes, const TArray<FGenesisTraitDefinition>* InTraits,
	const FGenesisLifeSimulationTuning& InTuning)
{
	Memory = InMemory;
	Genomes = InGenomes;
	Traits = InTraits;
	Tuning = InTuning;
}

void UGenesisLifeSimulationEngine::AddDefaultProcessors()
{
	const TArray<TSubclassOf<UGenesisLifeSimulationProcessor>> DefaultClasses = {
		UGenesisProcessor_CulturalIdentity::StaticClass(),
		UGenesisProcessor_Misunderstanding::StaticClass(),
		UGenesisProcessor_Indoctrination::StaticClass(),
		UGenesisProcessor_Belief::StaticClass(),
		UGenesisProcessor_Propaganda::StaticClass(),
		UGenesisProcessor_Zeitgeist::StaticClass(),
		UGenesisProcessor_GroupPressure::StaticClass(),
		UGenesisProcessor_EgoismAltruism::StaticClass(),
		UGenesisProcessor_DoubleStandard::StaticClass(),
		UGenesisProcessor_Trust::StaticClass(),
		UGenesisProcessor_ReputationRumors::StaticClass(),
		UGenesisProcessor_Karma::StaticClass(),
		UGenesisProcessor_Epigenetics::StaticClass(),
		UGenesisProcessor_ConsequenceNetwork::StaticClass()
	};

	for (const TSubclassOf<UGenesisLifeSimulationProcessor>& ProcessorClass : DefaultClasses)
	{
		AddProcessor(NewObject<UGenesisLifeSimulationProcessor>(this, ProcessorClass));
	}
}

void UGenesisLifeSimulationEngine::AddProcessor(UGenesisLifeSimulationProcessor* Processor)
{
	if (!Processor)
	{
		return;
	}

	const FGameplayTag SystemTag = Processor->GetSystemTag();
	const bool bDuplicate = Processors.ContainsByPredicate([&SystemTag](const TObjectPtr<UGenesisLifeSimulationProcessor>& Existing)
	{
		return Existing->GetSystemTag() == SystemTag;
	});
	if (bDuplicate)
	{
		UE_LOG(LogGenesis, Warning, TEXT("LifeSimulation: System %s ist bereits registriert."), *SystemTag.ToString());
		return;
	}

	Processors.Add(Processor);
	// Sortierung dereferenziert Zeiger-Elemente: Prädikat erhält Objekt-Referenzen
	Processors.StableSort([](const UGenesisLifeSimulationProcessor& A, const UGenesisLifeSimulationProcessor& B)
	{
		if (A.GetStage() != B.GetStage())
		{
			return static_cast<uint8>(A.GetStage()) < static_cast<uint8>(B.GetStage());
		}
		return A.GetOrder() < B.GetOrder();
	});
}

void UGenesisLifeSimulationEngine::ResetState(uint64 WorldSeed)
{
	State.Reset(WorldSeed);
}

void UGenesisLifeSimulationEngine::RegisterEntity(const FGenesisLifeProfile& Profile)
{
	if (!Profile.EntityId.IsValid())
	{
		UE_LOG(LogGenesis, Warning, TEXT("LifeSimulation: Person ohne EntityId wird nicht registriert."));
		return;
	}

	if (const FGenesisLifeProfile* Existing = State.FindProfile(Profile.EntityId))
	{
		// Stammdaten aktualisieren, verborgene Simulationswerte behalten
		FGenesisLifeProfile Merged = *Existing;
		Merged.SoulId = Profile.SoulId;
		Merged.GenomeId = Profile.GenomeId;
		Merged.SimulationLevel = Profile.SimulationLevel;
		Merged.Culture = Profile.Culture;
		Merged.Belief = Profile.Belief;
		Merged.Groups = Profile.Groups;
		State.AddOrUpdateProfile(Merged);
		return;
	}

	State.AddOrUpdateProfile(Profile);
}

FGenesisLifeSimulationContext UGenesisLifeSimulationEngine::MakeContext(const FGenesisTimestamp& Now)
{
	FGenesisLifeSimulationContext Context(State, Tuning);
	Context.Memory = Memory;
	Context.Genomes = Genomes;
	Context.Traits = Traits;
	Context.Now = Now;
	return Context;
}

FGuid UGenesisLifeSimulationEngine::ProcessAction(const FGenesisLifeAction& Action, const FGenesisTimestamp& Now, FGenesisLifeSimulationFrame* OutFrame)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisLife_ProcessAction);

	const UGenesisLifeActionDefinition* Definition = Action.Definition;
	if (!Definition)
	{
		UE_LOG(LogGenesis, Error, TEXT("LifeSimulation: Handlung ohne Definition."));
		return FGuid();
	}
	if (!State.FindProfile(Action.ActorId))
	{
		UE_LOG(LogGenesis, Warning, TEXT("LifeSimulation: Handelnder %s ist nicht registriert – verborgene Profilwerte werden nicht verändert."),
			*Action.ActorId.ToString(EGuidFormats::Short));
	}

	if (!State.bTimeInitialized)
	{
		State.LastDailyUpdate = Now;
		State.LastYearlyUpdate = Now;
		State.bTimeInitialized = true;
	}

	FGenesisLifeSimulationContext Context = MakeContext(Now);

	FGenesisLifeSimulationFrame Frame;
	Frame.EventId = Memory ? Memory->NewEventId() : State.Rng.NewGuid();
	Frame.Time = Now;
	Frame.Intensity = FMath::Clamp(Action.Intensity, 0.0f, 2.0f);
	Frame.Magnitude = FMath::Clamp(Definition->BaseMagnitude * Frame.Intensity, 0.0f, 1.0f);
	Frame.ActualIntent = FMath::Clamp(0.6f * Definition->OthersBenefit + 0.4f * Definition->TrustImpact, -1.0f, 1.0f);
	Frame.KarmaImpulse = Definition->KarmaImpulse * Frame.Intensity;
	Frame.StressLoad = Definition->StressLoad * Frame.Intensity;
	for (const FGenesisWeightedTag& Exposure : Definition->EpigeneticExposure)
	{
		Frame.EpigeneticExposure.Emplace(Exposure.Tag, Exposure.Weight * Frame.Intensity);
	}

	// Beteiligte: Ziele vor Zeugen, jede Person nur einmal, nie der Handelnde selbst
	auto AddPerception = [&Frame, &Action](const FGuid& ObserverId, EGenesisMemoryPerspective Role)
	{
		if (ObserverId.IsValid() && ObserverId != Action.ActorId && !Frame.FindPerception(ObserverId))
		{
			FGenesisObserverPerception& Perception = Frame.Perceptions.AddDefaulted_GetRef();
			Perception.ObserverId = ObserverId;
			Perception.Role = Role;
		}
	};
	for (const FGuid& Target : Action.TargetIds)
	{
		AddPerception(Target, EGenesisMemoryPerspective::Target);
	}
	for (const FGuid& Witness : Action.WitnessIds)
	{
		AddPerception(Witness, EGenesisMemoryPerspective::Witness);
	}

	for (const TObjectPtr<UGenesisLifeSimulationProcessor>& Processor : Processors)
	{
		Processor->ProcessAction(Action, *Definition, Frame, Context);
	}

	OnActionProcessed.Broadcast(Action, Frame);
	if (OutFrame)
	{
		*OutFrame = Frame;
	}
	return Frame.EventId;
}

void UGenesisLifeSimulationEngine::AdvanceTime(const FGenesisSimulationStep& Step)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisLife_AdvanceTime);

	if (!State.bTimeInitialized)
	{
		State.LastDailyUpdate = Step.From;
		State.LastYearlyUpdate = Step.From;
		State.bTimeInitialized = true;
	}

	FGenesisLifeSimulationContext Context = MakeContext(Step.To);

	Context.ElapsedDays = static_cast<double>(Step.To - State.LastDailyUpdate) / static_cast<double>(FGenesisTimestamp::SecondsPerDay);
	Context.bDailyTick = Context.ElapsedDays >= 1.0 || (Step.bIsTimeSkip && Context.ElapsedDays > 0.0);

	Context.ElapsedYears = FGenesisTimestamp::YearsBetween(State.LastYearlyUpdate, Step.To);
	Context.bYearlyTick = Context.ElapsedYears >= 1.0 || (Step.bIsTimeSkip && Context.ElapsedYears > 0.0);

	for (const TObjectPtr<UGenesisLifeSimulationProcessor>& Processor : Processors)
	{
		Processor->AdvanceTime(Step, Context);
	}

	if (Context.bDailyTick)
	{
		State.LastDailyUpdate = Step.To;
	}
	if (Context.bYearlyTick)
	{
		State.LastYearlyUpdate = Step.To;
	}

	for (const FGenesisFiredConsequence& Fired : Context.FiredConsequences)
	{
		OnConsequenceTriggered.Broadcast(Fired.Consequence, Fired.EventId);
	}
}
