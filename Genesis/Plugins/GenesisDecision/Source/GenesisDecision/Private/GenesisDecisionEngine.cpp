// GENESIS: Der Kreislauf des Lebens

#include "GenesisDecisionEngine.h"
#include "GenesisDecisionConsiderations.h"
#include "GenesisLog.h"
#include "GenesisRandom.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

void UGenesisDecisionEngine::AddDefaultConsiderations()
{
	AddConsideration(NewObject<UGenesisConsideration_Character>(this));
	AddConsideration(NewObject<UGenesisConsideration_Motive>(this));
	AddConsideration(NewObject<UGenesisConsideration_Conviction>(this));
	AddConsideration(NewObject<UGenesisConsideration_Culture>(this));
	AddConsideration(NewObject<UGenesisConsideration_Social>(this));
	AddConsideration(NewObject<UGenesisConsideration_Relationship>(this));
	AddConsideration(NewObject<UGenesisConsideration_Experience>(this));
	AddConsideration(NewObject<UGenesisConsideration_Subconscious>(this));
}

void UGenesisDecisionEngine::AddConsideration(UGenesisDecisionConsideration* Consideration)
{
	if (!Consideration)
	{
		return;
	}
	const FName Name = Consideration->GetConsiderationName();
	if (Considerations.ContainsByPredicate([Name](const TObjectPtr<UGenesisDecisionConsideration>& Existing) { return Existing->GetConsiderationName() == Name; }))
	{
		UE_LOG(LogGenesis, Warning, TEXT("Decision: Consideration %s ist bereits registriert."), *Name.ToString());
		return;
	}
	Considerations.Add(Consideration);
}

FGenesisDecisionResult UGenesisDecisionEngine::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionInputs& Inputs) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisDecision_Evaluate);

	FGenesisDecisionResult Result;
	const FGenesisLifeProfile* Decider = Inputs.State ? Inputs.State->FindProfile(Situation.DeciderId) : nullptr;
	if (!Decider)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Decision: Entscheider %s ist nicht registriert."), *Situation.DeciderId.ToString(EGuidFormats::Short));
		return Result;
	}

	// Zeitdruck und Stress verdrängen die Überlegung zugunsten des Unterbewusstseins
	const float Deliberation = FMath::Clamp(
		1.0f - Tuning.TimePressureDeliberationLoss * Situation.TimePressure - Tuning.StressDeliberationLoss * Decider->Stress,
		Tuning.MinDeliberation, 1.0f);

	for (int32 OptionIndex = 0; OptionIndex < Situation.Options.Num(); ++OptionIndex)
	{
		const FGenesisDecisionOption& Option = Situation.Options[OptionIndex];
		FGenesisOptionEvaluation& Evaluation = Result.Evaluations.AddDefaulted_GetRef();
		Evaluation.OptionIndex = OptionIndex;
		Evaluation.bValid = Option.Action != nullptr;
		if (!Evaluation.bValid)
		{
			continue;
		}

		float ConsciousSum = 0.0f;
		float ConsciousWeight = 0.0f;
		float SubconsciousSum = 0.0f;
		float SubconsciousWeight = 0.0f;
		float CharacterScore = 0.0f;
		TArray<TPair<FGuid, float>> Influences;

		for (const TObjectPtr<UGenesisDecisionConsideration>& Consideration : Considerations)
		{
			const float Score = FMath::Clamp(Consideration->Evaluate(Situation, Option, *Decider, Inputs, Influences), -1.0f, 1.0f);
			const float Weight = Consideration->Weight;

			FGenesisConsiderationScore& Entry = Evaluation.Breakdown.AddDefaulted_GetRef();
			Entry.Consideration = Consideration->GetConsiderationName();
			Entry.Score = Score;
			Entry.Weight = Weight;
			Entry.bSubconscious = Consideration->IsSubconscious();

			if (Entry.bSubconscious)
			{
				SubconsciousSum += Score * Weight;
				SubconsciousWeight += Weight;
			}
			else
			{
				ConsciousSum += Score * Weight;
				ConsciousWeight += Weight;
				if (Entry.Consideration == TEXT("Character"))
				{
					CharacterScore = Score;
				}
			}
		}

		Evaluation.Conscious = ConsciousWeight > 0.0f ? ConsciousSum / ConsciousWeight : 0.0f;
		Evaluation.Subconscious = SubconsciousWeight > 0.0f ? SubconsciousSum / SubconsciousWeight : 0.0f;
		Evaluation.Impulse = FMath::Lerp(Evaluation.Subconscious, CharacterScore, Tuning.CharacterInImpulse);
		Evaluation.Total = Deliberation * Evaluation.Conscious + (1.0f - Deliberation) * Evaluation.Impulse + Tuning.SubconsciousFloor * Evaluation.Subconscious;

		// Stärkste prägende Erinnerungen (pro Ereignis nur einmal)
		Influences.StableSort([](const TPair<FGuid, float>& A, const TPair<FGuid, float>& B) { return A.Value > B.Value; });
		for (const TPair<FGuid, float>& Influence : Influences)
		{
			if (Evaluation.InfluenceEventIds.Num() >= Tuning.MaxInfluenceEvents)
			{
				break;
			}
			Evaluation.InfluenceEventIds.AddUnique(Influence.Key);
		}
	}

	// Wahrscheinlichkeiten (Softmax): Stress und geringe Tragweite machen die Wahl weniger vorhersagbar
	const float Temperature = FMath::Max(0.01f, Tuning.BaseTemperature + Tuning.StressTemperature * Decider->Stress + Tuning.LowStakesTemperature * (1.0f - Situation.Stakes));

	float BestTotal = -FLT_MAX;
	float BestConscious = -FLT_MAX;
	float BestImpulse = -FLT_MAX;
	for (const FGenesisOptionEvaluation& Evaluation : Result.Evaluations)
	{
		if (!Evaluation.bValid)
		{
			continue;
		}
		BestTotal = FMath::Max(BestTotal, Evaluation.Total);
		if (Evaluation.Conscious > BestConscious)
		{
			BestConscious = Evaluation.Conscious;
			Result.ConsciousBestIndex = Evaluation.OptionIndex;
		}
		if (Evaluation.Impulse > BestImpulse)
		{
			BestImpulse = Evaluation.Impulse;
			Result.ImpulseIndex = Evaluation.OptionIndex;
		}
	}
	if (Result.ImpulseIndex == INDEX_NONE)
	{
		return Result;
	}

	double Normalizer = 0.0;
	for (const FGenesisOptionEvaluation& Evaluation : Result.Evaluations)
	{
		if (Evaluation.bValid)
		{
			Normalizer += FMath::Exp((Evaluation.Total - BestTotal) / Temperature);
		}
	}

	float Highest = 0.0f;
	float SecondHighest = 0.0f;
	for (FGenesisOptionEvaluation& Evaluation : Result.Evaluations)
	{
		Evaluation.Probability = Evaluation.bValid ? static_cast<float>(FMath::Exp((Evaluation.Total - BestTotal) / Temperature) / Normalizer) : 0.0f;
		if (Evaluation.Probability > Highest)
		{
			SecondHighest = Highest;
			Highest = Evaluation.Probability;
		}
		else if (Evaluation.Probability > SecondHighest)
		{
			SecondHighest = Evaluation.Probability;
		}
	}

	// Zögern: je knapper die beiden besten Möglichkeiten, desto stärker
	Result.Hesitation = FMath::Clamp(1.0f - (Highest - SecondHighest), 0.0f, 1.0f);

	// Innerer Konflikt: Kopf und Bauch zeigen auf unterschiedliche Optionen
	if (Result.ConsciousBestIndex != Result.ImpulseIndex)
	{
		const FGenesisOptionEvaluation& Head = Result.Evaluations[Result.ConsciousBestIndex];
		const FGenesisOptionEvaluation& Gut = Result.Evaluations[Result.ImpulseIndex];
		const float HeadMargin = Head.Conscious - Gut.Conscious;
		const float GutMargin = Gut.Impulse - Head.Impulse;
		Result.InnerConflict = FMath::Clamp(0.5f * (HeadMargin + GutMargin) + 0.25f, 0.0f, 1.0f);
	}

	return Result;
}

void UGenesisDecisionEngine::ChooseForNpc(FGenesisDecisionResult& Result, FGenesisRandomStream& Rng) const
{
	const double Roll = Rng.NextDouble();
	double Cumulative = 0.0;
	int32 LastValid = INDEX_NONE;

	for (const FGenesisOptionEvaluation& Evaluation : Result.Evaluations)
	{
		if (!Evaluation.bValid)
		{
			continue;
		}
		LastValid = Evaluation.OptionIndex;
		Cumulative += Evaluation.Probability;
		if (Roll < Cumulative)
		{
			Result.ChosenIndex = Evaluation.OptionIndex;
			break;
		}
	}

	if (Result.ChosenIndex == INDEX_NONE)
	{
		// Rundungsrest: letzte gültige Option
		Result.ChosenIndex = LastValid;
	}
	Result.bFollowedImpulse = Result.ChosenIndex != INDEX_NONE && Result.ChosenIndex == Result.ImpulseIndex;
}

void UGenesisDecisionEngine::ApplyPlayerChoice(FGenesisDecisionResult& Result, int32 ChosenIndex)
{
	const bool bValidChoice = Result.Evaluations.IsValidIndex(ChosenIndex) && Result.Evaluations[ChosenIndex].bValid;
	Result.ChosenIndex = bValidChoice ? ChosenIndex : Result.ImpulseIndex;
	Result.bFollowedImpulse = Result.ChosenIndex != INDEX_NONE && Result.ChosenIndex == Result.ImpulseIndex;
}

FGenesisLifeAction UGenesisDecisionEngine::BuildAction(const FGenesisDecisionSituation& Situation, const FGenesisDecisionResult& Result)
{
	FGenesisLifeAction Action;
	if (!Situation.Options.IsValidIndex(Result.ChosenIndex))
	{
		return Action;
	}

	const FGenesisDecisionOption& Option = Situation.Options[Result.ChosenIndex];
	Action.Definition = Option.Action;
	Action.ActorId = Situation.DeciderId;
	Action.TargetIds = Option.TargetIds;
	Action.WitnessIds = Situation.WitnessIds;
	Action.Groups = Situation.Groups;
	Action.Intensity = Option.Intensity;
	Action.CausedByEventId = Situation.CausedByEventId;

	if (Result.Evaluations.IsValidIndex(Result.ChosenIndex))
	{
		// Erinnerungen, die die Wahl geprägt haben, werden im Kausalgraph zu Ursachen
		Action.InfluenceEventIds = Result.Evaluations[Result.ChosenIndex].InfluenceEventIds;
	}
	return Action;
}
