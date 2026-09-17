// GENESIS: Der Kreislauf des Lebens

#include "GenesisMemoryTrace.h"
#include "GenesisCausalGraph.h"
#include "GenesisGameplayTags.h"
#include "GenesisMath.h"
#include "GenesisRandom.h"

FGenesisMemoryTrace* FGenesisMemoryStore::FindTrace(const FGuid& TraceId)
{
	return Traces.FindByPredicate([&TraceId](const FGenesisMemoryTrace& Trace) { return Trace.TraceId == TraceId; });
}

const FGenesisMemoryTrace* FGenesisMemoryStore::FindTrace(const FGuid& TraceId) const
{
	return Traces.FindByPredicate([&TraceId](const FGenesisMemoryTrace& Trace) { return Trace.TraceId == TraceId; });
}

const FGenesisMemoryTrace* FGenesisMemoryStore::FindTraceForEvent(const FGuid& EventId) const
{
	return Traces.FindByPredicate([&EventId](const FGenesisMemoryTrace& Trace) { return Trace.EventId == EventId; });
}

namespace GenesisMemoryLogic
{
	namespace
	{
		float PerspectiveAccuracy(EGenesisMemoryPerspective Perspective)
		{
			switch (Perspective)
			{
			case EGenesisMemoryPerspective::Actor:       return 1.0f;
			case EGenesisMemoryPerspective::Target:      return 0.95f;
			case EGenesisMemoryPerspective::Participant: return 0.9f;
			case EGenesisMemoryPerspective::Witness:     return 0.85f;
			case EGenesisMemoryPerspective::Hearsay:     return 0.6f;
			case EGenesisMemoryPerspective::Inherited:   return 0.45f;
			default:                                     return 0.8f;
			}
		}

		float PerspectiveSalience(EGenesisMemoryPerspective Perspective)
		{
			switch (Perspective)
			{
			case EGenesisMemoryPerspective::Actor:
			case EGenesisMemoryPerspective::Target:      return 0.15f;
			case EGenesisMemoryPerspective::Participant: return 0.05f;
			case EGenesisMemoryPerspective::Hearsay:     return -0.15f;
			case EGenesisMemoryPerspective::Inherited:   return -0.25f;
			default:                                     return 0.0f;
			}
		}

		float SaturateTowardsOne(float Current, float Amount)
		{
			return FMath::Clamp(Current + FMath::Clamp(Amount, 0.0f, 1.0f) * (1.0f - Current), 0.0f, 1.0f);
		}
	}

	FGenesisMemoryTrace& Encode(FGenesisMemoryStore& Store, const FGenesisCausalEvent& Event, const FGenesisEncodingContext& Context,
		const FGenesisTimestamp& Now, FGenesisRandomStream& Rng)
	{
		const float Stress = FMath::Clamp(Context.Stress, 0.0f, 1.0f);
		const float Attention = FMath::Clamp(Context.Attention, 0.0f, 1.0f);
		const float Arousal = FMath::Clamp(Context.Arousal, 0.0f, 1.0f);

		// Yerkes-Dodson: mittlerer Stress prägt stärker ein, extremer nicht mehr
		const float StressSharpening = 1.0f + 0.25f * (1.0f - FMath::Square(2.0f * Stress - 1.0f));
		const float NewIntensity = FMath::Clamp(
			(Event.Magnitude * 0.45f + Arousal * 0.4f + Attention * 0.15f + PerspectiveSalience(Context.Perspective)) * StressSharpening,
			0.02f, 1.0f);

		// Dasselbe Ereignis erneut erlebt (z. B. erst gehört, dann gesehen) → Spur verstärken
		for (FGenesisMemoryTrace& Existing : Store.Traces)
		{
			if (Existing.EventId == Event.EventId)
			{
				Existing.Intensity = SaturateTowardsOne(Existing.Intensity, NewIntensity * 0.5f);
				Existing.Arousal = FMath::Max(Existing.Arousal, Arousal);
				Existing.SensoryCues.AppendTags(Context.SensoryCues);
				return Existing;
			}
		}

		FGenesisMemoryTrace& Trace = Store.Traces.AddDefaulted_GetRef();
		Trace.TraceId = Rng.NewGuid();
		Trace.EventId = Event.EventId;
		Trace.EncodedAt = Now;
		Trace.LastRecalledAt = Now;
		Trace.Perspective = Context.Perspective;
		Trace.Intensity = NewIntensity;
		Trace.Arousal = Arousal;
		Trace.Accuracy = FMath::Clamp(
			0.97f * PerspectiveAccuracy(Context.Perspective) * (0.6f + 0.4f * Attention) * (1.0f - 0.45f * FMath::Square(Stress)),
			0.05f, 1.0f);
		Trace.Valence = FMath::Clamp(Event.Valence * 0.8f + Context.Mood * 0.2f, -1.0f, 1.0f);
		Trace.SensoryCues = Context.SensoryCues;
		Trace.Narrative = Context.Narrative;

		if (Context.bHasPerceptionOverride)
		{
			Trace.PerceivedThemes = Context.PerceivedThemesOverride;
			Trace.PerceivedActorId = Context.PerceivedActorOverride.IsValid() ? Context.PerceivedActorOverride : Event.ActorId;
		}
		else
		{
			Trace.PerceivedThemes = Event.Themes;
			Trace.PerceivedActorId = Event.ActorId;

			// Ungenaue Wahrnehmung: ein Thema geht verloren, oder die Handlung wird der falschen Person zugeschrieben
			if (Rng.Bernoulli(1.0f - Trace.Accuracy))
			{
				if (Trace.PerceivedThemes.Num() > 1)
				{
					TArray<FGameplayTag> Tags;
					Trace.PerceivedThemes.GetGameplayTagArray(Tags);
					Trace.PerceivedThemes.RemoveTag(Tags[Rng.RandRange(0, Tags.Num() - 1)]);
				}

				TArray<FGuid> Candidates;
				for (const FGuid& Id : Event.TargetIds)
				{
					if (Id != Store.OwnerId) { Candidates.AddUnique(Id); }
				}
				for (const FGuid& Id : Event.WitnessIds)
				{
					if (Id != Store.OwnerId) { Candidates.AddUnique(Id); }
				}
				Candidates.Remove(Event.ActorId);
				if (Candidates.Num() > 0 && Rng.Bernoulli(0.5f))
				{
					Trace.PerceivedActorId = Candidates[Rng.RandRange(0, Candidates.Num() - 1)];
				}
			}
		}

		Trace.bDistorted = Trace.PerceivedActorId != Event.ActorId || Trace.PerceivedThemes != Event.Themes;
		return Trace;
	}

	int32 Decay(FGenesisMemoryStore& Store, double ElapsedYears, const FGenesisMemoryDynamicsParams& Params)
	{
		if (ElapsedYears <= 0.0)
		{
			return 0;
		}

		for (FGenesisMemoryTrace& Trace : Store.Traces)
		{
			const double HalfLife = Params.BaseHalfLifeYears
				+ Params.EmotionalHalfLifeBonusYears * FMath::Square(static_cast<double>(Trace.Arousal))
				+ Params.RecallHalfLifeBonusYears * FMath::Log2(1.0 + static_cast<double>(Trace.RecallCount));

			Trace.Intensity = static_cast<float>(GenesisMath::ExponentialDecay(Trace.Intensity, ElapsedYears, HalfLife));
			Trace.Accuracy = FMath::Max(Params.MinimumAccuracy,
				Trace.Accuracy - Params.AccuracyDriftPerYear * static_cast<float>(ElapsedYears) * (1.0f - 0.5f * Trace.Arousal));

			// Sehr emotionale Spuren verschwinden nie ganz – sie ruhen und können durch Reize zurückkehren
			if (Trace.Intensity < Params.ForgetThreshold && Trace.Arousal >= Params.DormantArousalThreshold)
			{
				Trace.Intensity = Params.ForgetThreshold;
			}
		}

		return Store.Traces.RemoveAll([&Params](const FGenesisMemoryTrace& Trace)
		{
			return Trace.Intensity < Params.ForgetThreshold;
		});
	}

	bool Recall(FGenesisMemoryStore& Store, const FGuid& TraceId, const FGenesisTimestamp& Now, float CurrentMood, const FGenesisMemoryDynamicsParams& Params)
	{
		FGenesisMemoryTrace* Trace = Store.FindTrace(TraceId);
		if (!Trace)
		{
			return false;
		}

		Trace->Intensity = SaturateTowardsOne(Trace->Intensity, Params.RecallIntensityGain * (1.0f - Trace->Repression));
		Trace->Accuracy = FMath::Max(Params.MinimumAccuracy, Trace->Accuracy - Params.RecallAccuracyCost);
		Trace->Valence = FMath::Lerp(Trace->Valence, FMath::Clamp(CurrentMood, -1.0f, 1.0f), Params.RecallMoodPull);
		Trace->LastRecalledAt = Now;
		++Trace->RecallCount;
		return true;
	}

	TArray<FGenesisRecallCandidate> FindByCues(const FGenesisMemoryStore& Store, const FGameplayTagContainer& SensoryCues,
		const FGameplayTagContainer& Themes, int32 MaxResults, float MinScore, const FGenesisMemoryDynamicsParams& Params)
	{
		TArray<FGenesisRecallCandidate> Candidates;
		const FGameplayTag SmellRoot = GenesisTags::Sense_Smell;

		for (const FGenesisMemoryTrace& Trace : Store.Traces)
		{
			float SensoryMatch = 0.0f;
			for (const FGameplayTag& TraceCue : Trace.SensoryCues)
			{
				if (SensoryCues.HasTag(TraceCue) || TraceCue.MatchesAny(SensoryCues))
				{
					SensoryMatch += TraceCue.MatchesTag(SmellRoot) ? Params.SmellCueWeight : 1.0f;
				}
			}

			float ThemeMatch = 0.0f;
			for (const FGameplayTag& Theme : Themes)
			{
				if (Trace.PerceivedThemes.HasTag(Theme))
				{
					ThemeMatch += 0.5f;
				}
			}

			if (SensoryMatch <= 0.0f && ThemeMatch <= 0.0f)
			{
				continue;
			}

			// Verdrängte Erinnerungen bleiben schwer zugänglich
			const float Accessibility = Trace.Intensity * (1.0f - 0.8f * Trace.Repression);
			const float Score = Accessibility * (SensoryMatch + ThemeMatch);
			if (Score < MinScore)
			{
				continue;
			}

			FGenesisRecallCandidate& Candidate = Candidates.AddDefaulted_GetRef();
			Candidate.TraceId = Trace.TraceId;
			Candidate.EventId = Trace.EventId;
			Candidate.Score = Score;
			Candidate.bFlashback = SensoryMatch > 0.0f && Score >= 0.6f && Trace.Arousal >= 0.5f;
		}

		Candidates.StableSort([](const FGenesisRecallCandidate& A, const FGenesisRecallCandidate& B)
		{
			return A.Score > B.Score;
		});
		if (MaxResults > 0 && Candidates.Num() > MaxResults)
		{
			Candidates.SetNum(MaxResults);
		}
		return Candidates;
	}

	bool AdjustRepression(FGenesisMemoryStore& Store, const FGuid& TraceId, float Delta)
	{
		if (FGenesisMemoryTrace* Trace = Store.FindTrace(TraceId))
		{
			Trace->Repression = FMath::Clamp(Trace->Repression + Delta, 0.0f, 1.0f);
			return true;
		}
		return false;
	}

	float CompareWithTruth(const FGenesisMemoryTrace& Trace, const FGenesisCausalEvent& Event)
	{
		const float ThemeFidelity = GenesisMath::TagSimilarity(Trace.PerceivedThemes, Event.Themes);
		const float ActorFidelity = Trace.PerceivedActorId == Event.ActorId ? 1.0f : 0.0f;
		return 0.6f * ThemeFidelity + 0.4f * ActorFidelity;
	}
}
