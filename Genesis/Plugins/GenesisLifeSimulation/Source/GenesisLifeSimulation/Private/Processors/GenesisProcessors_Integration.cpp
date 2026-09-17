// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationProcessors.h"
#include "GenesisCausalGraph.h"
#include "GenesisGameplayTags.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisGenomePool.h"
#include "GenesisLifeAction.h"
#include "GenesisMath.h"
#include "GenesisMemoryWorld.h"
#include "GenesisWorldClockSubsystem.h"

// ======================================= Karma =======================================

FGameplayTag UGenesisProcessor_Karma::GetSystemTag() const
{
	return GenesisTags::SimSystem_Karma;
}

void UGenesisProcessor_Karma::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	if (FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId))
	{
		// Der Impuls wurde von Gruppenzwang, Opferbereitschaft und Doppelmoral bereits moduliert
		Actor->Karma.ApplyImpulse(Frame.KarmaImpulse, Context.Tuning.Karma);
	}
}

// ============================== Genetischer Code & Epigenetik ==============================

FGameplayTag UGenesisProcessor_Epigenetics::GetSystemTag() const
{
	return GenesisTags::SimSystem_Epigenetics;
}

void UGenesisProcessor_Epigenetics::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);
	if (!Actor)
	{
		return;
	}

	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;

	// Veranlagung: dieselbe Situation belastet Menschen unterschiedlich stark
	float Sensitivity = 0.5f;
	if (Context.Genomes && Context.Traits && Actor->GenomeId.IsValid())
	{
		if (const FGenesisGenome* Genome = Context.Genomes->Find(Actor->GenomeId))
		{
			Sensitivity = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, *Context.Traits, GenesisGeneticsTags::Trait_Risk_AnxietySensitivity, 0.5f);
		}
	}

	const float Load = FMath::Clamp(Frame.StressLoad + 0.3f * Frame.Dissonance, 0.0f, 1.0f) * (0.5f + Sensitivity);

	Actor->Stress = FMath::Clamp(Actor->Stress + Tuning.StressGain * Load, 0.0f, 1.0f);
	Actor->Dissonance = GenesisMath::ApplySaturatingDelta(Actor->Dissonance, Tuning.DissonanceGain * FMath::Clamp(Frame.Dissonance, 0.0f, 1.0f), 0.0f, 1.0f, 1.0f);

	if (Load > 0.0f)
	{
		GenesisWeightedTags::AddWeight(Actor->EpigeneticDose, GenesisGeneticsTags::Epigenetic_StressResponse, Tuning.StressDoseScale * Load, -10.0f, 10.0f);
	}
	for (const FGenesisWeightedTag& Exposure : Frame.EpigeneticExposure)
	{
		GenesisWeightedTags::AddWeight(Actor->EpigeneticDose, Exposure.Tag, Exposure.Weight, -10.0f, 10.0f);
	}
}

void UGenesisProcessor_Epigenetics::AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const
{
	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;

	if (Context.bDailyTick)
	{
		// Erholung: Belastung und innerer Widerspruch klingen ohne neue Auslöser ab
		for (FGenesisLifeProfile& Profile : Context.State.Profiles)
		{
			Profile.Stress = GenesisMath::ApproachExponential(Profile.Stress, 0.0f, Tuning.StressRecoveryPerDay, Context.ElapsedDays);
			Profile.Dissonance = GenesisMath::ApproachExponential(Profile.Dissonance, 0.0f, Tuning.DissonanceRecoveryPerDay, Context.ElapsedDays);
		}
	}

	if (!Context.bYearlyTick || !Context.Genomes)
	{
		return;
	}

	// Jährlich: gesammelte Lebensstil-Dosis wird zur epigenetischen Markierung
	for (FGenesisLifeProfile& Profile : Context.State.Profiles)
	{
		FGenesisGenome* Genome = Profile.GenomeId.IsValid() ? Context.Genomes->FindMutable(Profile.GenomeId) : nullptr;
		if (!Genome)
		{
			Profile.EpigeneticDose.Reset();
			continue;
		}

		GenesisGeneticsLogic::RevertEpigeneticMarks(*Genome, Context.ElapsedYears, Tuning.EpigeneticReversionPerYear);
		for (const FGenesisWeightedTag& Dose : Profile.EpigeneticDose)
		{
			GenesisGeneticsLogic::ApplyEpigeneticExposure(*Genome, Dose.Tag, Dose.Weight, Tuning.EpigeneticPlasticity);
		}
		Profile.EpigeneticDose.Reset();
	}
}

// ================================= Konsequenz-Netzwerk =================================

namespace
{
	bool ShouldEncode(const FGenesisLifeProfile* Profile, float Magnitude, const FGenesisLifeSimulationTuning& Tuning)
	{
		if (!Profile)
		{
			return false;
		}
		switch (Profile->SimulationLevel)
		{
		case EGenesisSimulationLevel::Full:        return true;
		case EGenesisSimulationLevel::Reduced:     return Magnitude >= Tuning.ReducedEncodingMinMagnitude;
		case EGenesisSimulationLevel::Statistical:
		default:                                   return false;
		}
	}

	void AddFact(FGenesisCausalEvent& Event, const FGameplayTag& Tag, float Value)
	{
		if (!FMath::IsNearlyZero(Value))
		{
			Event.Facts.Emplace(Tag, Value);
		}
	}
}

FGameplayTag UGenesisProcessor_ConsequenceNetwork::GetSystemTag() const
{
	return GenesisTags::SimSystem_ConsequenceNetwork;
}

void UGenesisProcessor_ConsequenceNetwork::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;
	FGenesisRandomStream& Rng = Context.Rng();
	const FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);

	// --- 1) Ereignis mit allen Systemergebnissen in den Kausalgraph ---
	if (Context.Memory)
	{
		FGenesisCausalEvent Event;
		Event.EventId = Frame.EventId;
		Event.Time = Frame.Time;
		Event.EventType = Definition.ActionType;
		Event.Themes = Definition.Themes;
		Event.ExpressedValues = Definition.ExpressesValues;
		Event.ViolatedValues = Definition.ViolatesValues;
		Event.ActorId = Action.ActorId;
		Event.TargetIds = Action.TargetIds;
		Event.WitnessIds = Action.WitnessIds;
		Event.LocationId = Action.LocationId;
		Event.ActorSoulId = Actor ? Actor->SoulId : FGuid();
		Event.Magnitude = Frame.Magnitude;
		Event.Valence = Definition.Valence;
		Event.bResolved = !Definition.bOpensUnresolvedThread;

		// "Warum" für Lebensrückblick und Kosmische Bibliothek
		AddFact(Event, GenesisTags::SimSystem_CulturalIdentity, Frame.TabooViolation);
		AddFact(Event, GenesisTags::SimSystem_Indoctrination, Frame.IndoctrinationAlignment);
		AddFact(Event, GenesisTags::SimSystem_Belief, Frame.BeliefConsistency);
		AddFact(Event, GenesisTags::SimSystem_Zeitgeist, Frame.ZeitgeistAlignment);
		AddFact(Event, GenesisTags::SimSystem_GroupPressure, Frame.GroupPressure);
		AddFact(Event, GenesisTags::SimSystem_EgoismAltruism, Frame.AltruismSignal);
		AddFact(Event, GenesisTags::SimSystem_DoubleStandard, Frame.Hypocrisy);

		const FGenesisCausalEvent* Recorded = Context.Memory->RecordEvent(Event);
		if (Recorded)
		{
			// Direkte Ursache und prägende Einflüsse
			if (Action.CausedByEventId.IsValid())
			{
				Context.Memory->AddLink(Action.CausedByEventId, Frame.EventId, EGenesisCausalLinkType::Direct, 0.8f);
			}
			for (const FGuid& Influence : Frame.InfluenceEventIds)
			{
				Context.Memory->AddLink(Influence, Frame.EventId, EGenesisCausalLinkType::Contributing, 0.4f);
			}

			// Automatische Verknüpfung: thematisch verwandte, kürzlich erlebte Ereignisse prägen die Handlung
			// (z. B. wer belogen wurde, lügt eher selbst)
			if (Definition.Themes.Num() > 0 && Tuning.AutoLinkMaxLinks > 0)
			{
				const int64 WindowSeconds = FGenesisTimestamp::YearsToSeconds(Tuning.AutoLinkWindowYears);
				FGenesisEventQuery Query;
				Query.InvolvedEntityId = Action.ActorId;
				Query.AnyThemes = Definition.Themes;
				Query.bUseTimeRange = true;
				Query.From = Frame.Time - WindowSeconds;
				Query.To = Frame.Time;
				Query.MaxResults = 20;

				TArray<TPair<FGuid, float>> Candidates;
				for (const FGenesisCausalEvent* Past : Context.Memory->GetGraph().FindEvents(Query))
				{
					if (Past->EventId == Frame.EventId)
					{
						continue;
					}
					const float Age = static_cast<float>(Frame.Time - Past->Time) / static_cast<float>(FMath::Max<int64>(1, WindowSeconds));
					const float Score = GenesisMath::TagSimilarity(Past->Themes, Definition.Themes) * FMath::Clamp(1.0f - Age, 0.0f, 1.0f);
					if (Score > 0.05f)
					{
						Candidates.Emplace(Past->EventId, Score);
					}
				}
				Candidates.StableSort([](const TPair<FGuid, float>& A, const TPair<FGuid, float>& B) { return A.Value > B.Value; });
				for (int32 Index = 0; Index < FMath::Min(Candidates.Num(), Tuning.AutoLinkMaxLinks); ++Index)
				{
					Context.Memory->AddLink(Candidates[Index].Key, Frame.EventId, EGenesisCausalLinkType::Contributing, Tuning.AutoLinkStrength * Candidates[Index].Value);
				}
			}

			// --- 2) Subjektive Erinnerungen (Simulation LOD beachtet) ---
			if (ShouldEncode(Actor, Frame.Magnitude, Tuning))
			{
				FGenesisEncodingContext Encoding;
				Encoding.Perspective = EGenesisMemoryPerspective::Actor;
				Encoding.Arousal = FMath::Clamp(FMath::Abs(Definition.Valence) * 0.5f + Frame.Magnitude * 0.5f, 0.0f, 1.0f);
				Encoding.Stress = Actor->Stress;
				Encoding.Narrative = Action.Narrative;
				Context.Memory->EncodeMemory(Action.ActorId, Frame.EventId, Encoding, Frame.Time);
			}

			for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
			{
				const FGenesisLifeProfile* Observer = Context.State.FindProfile(Perception.ObserverId);
				if (!Perception.bNoticed || !ShouldEncode(Observer, Frame.Magnitude, Tuning))
				{
					continue;
				}

				FGenesisEncodingContext Encoding;
				Encoding.Perspective = Perception.Role;
				Encoding.Arousal = Perception.Arousal;
				Encoding.Stress = Observer->Stress;
				Encoding.bHasPerceptionOverride = Perception.bMisread;
				Encoding.PerceivedThemesOverride = Perception.PerceivedThemes;
				Encoding.PerceivedActorOverride = Action.ActorId;
				Context.Memory->EncodeMemory(Perception.ObserverId, Frame.EventId, Encoding, Frame.Time);
			}
		}
	}

	// --- 3) Verzögerte Folgen einplanen ---
	const int32 Noticed = Frame.CountNoticed();
	for (const FGenesisDelayedConsequenceSpec& Spec : Definition.DelayedConsequences)
	{
		if (!Spec.ConsequenceType.IsValid() || (Spec.bRequiresNoticed && Noticed == 0))
		{
			continue;
		}

		const float Probability = FMath::Clamp(Spec.Probability * Frame.Intensity, 0.0f, 1.0f);
		if (!Rng.Bernoulli(Probability))
		{
			continue;
		}

		FGenesisScheduledConsequence& Scheduled = Context.State.PendingConsequences.AddDefaulted_GetRef();
		Scheduled.ConsequenceId = Rng.NewGuid();
		Scheduled.SourceEventId = Frame.EventId;
		Scheduled.SourceActorId = Action.ActorId;
		Scheduled.Spec = Spec;
		Scheduled.DueTime = Frame.Time + FGenesisTimestamp::YearsToSeconds(Rng.FRandRange(Spec.MinDelayYears, FMath::Max(Spec.MinDelayYears, Spec.MaxDelayYears)));

		switch (Spec.Subject)
		{
		case EGenesisConsequenceSubject::Actor:
			Scheduled.SubjectIds.Add(Action.ActorId);
			break;
		case EGenesisConsequenceSubject::Targets:
			Scheduled.SubjectIds = Action.TargetIds;
			break;
		case EGenesisConsequenceSubject::Witnesses:
			Scheduled.SubjectIds = Action.WitnessIds;
			break;
		case EGenesisConsequenceSubject::ActorAndTargets:
			Scheduled.SubjectIds.Add(Action.ActorId);
			Scheduled.SubjectIds.Append(Action.TargetIds);
			break;
		}

		++Frame.ScheduledConsequences;
	}
}

void UGenesisProcessor_ConsequenceNetwork::AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const
{
	TArray<FGenesisScheduledConsequence>& Pending = Context.State.PendingConsequences;
	if (Pending.Num() == 0)
	{
		return;
	}

	TArray<FGenesisScheduledConsequence> Due;
	for (int32 Index = Pending.Num() - 1; Index >= 0; --Index)
	{
		if (Pending[Index].DueTime <= Context.Now)
		{
			Due.Add(Pending[Index]);
			Pending.RemoveAt(Index);
		}
	}
	Due.StableSort([](const FGenesisScheduledConsequence& A, const FGenesisScheduledConsequence& B) { return A.DueTime < B.DueTime; });

	for (const FGenesisScheduledConsequence& Consequence : Due)
	{
		FGuid EventId;
		if (Context.Memory)
		{
			FGenesisCausalEvent Event;
			Event.EventId = Context.Rng().NewGuid();
			Event.Time = Consequence.DueTime;
			Event.EventType = Consequence.Spec.ConsequenceType;
			Event.Themes = Consequence.Spec.Themes;
			Event.ActorId = Consequence.SubjectIds.Num() > 0 ? Consequence.SubjectIds[0] : Consequence.SourceActorId;
			for (int32 Index = 1; Index < Consequence.SubjectIds.Num(); ++Index)
			{
				Event.TargetIds.Add(Consequence.SubjectIds[Index]);
			}
			if (const FGenesisLifeProfile* Subject = Context.State.FindProfile(Event.ActorId))
			{
				Event.ActorSoulId = Subject->SoulId;
			}
			Event.Magnitude = Consequence.Spec.Magnitude;
			Event.Valence = Consequence.Spec.Valence;
			Event.bResolved = !Consequence.Spec.bOpensThread;

			if (const FGenesisCausalEvent* Recorded = Context.Memory->RecordEvent(Event))
			{
				EventId = Recorded->EventId;
				// Die Ursache kann durch Verdichtung bereits entfernt sein – dann bleibt die Folge ohne Kante
				Context.Memory->AddLink(Consequence.SourceEventId, EventId, EGenesisCausalLinkType::Direct, Consequence.Spec.LinkStrength);
			}
		}

		Context.FiredConsequences.Add({ Consequence, EventId });
	}
}
