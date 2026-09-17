// GENESIS: Der Kreislauf des Lebens

#include "GenesisDecisionConsiderations.h"
#include "GenesisCausalGraph.h"
#include "GenesisMath.h"
#include "GenesisMemoryWorld.h"
#include "GenesisSoulLogic.h"

namespace
{
	FGameplayTagContainer OptionThemes(const FGenesisDecisionSituation& Situation, const UGenesisLifeActionDefinition& Action)
	{
		FGameplayTagContainer Themes = Action.Themes;
		Themes.AppendTags(Situation.SituationThemes);
		return Themes;
	}

	/** Vermittelt eine Summe gewichteter Werte ohne Division durch null. */
	float WeightedMean(float WeightedSum, float WeightTotal)
	{
		return WeightTotal > KINDA_SMALL_NUMBER ? WeightedSum / WeightTotal : 0.0f;
	}
}

// ===================================== Charakter =====================================

float UGenesisConsideration_Character::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const FGenesisKarmaVector& Impulse = Option.Action->KarmaImpulse;
	const FGenesisKarmaVector& Values = Decider.Karma.Values;

	float Alignment = 0.0f;
	float ImpulseMagnitude = 0.0f;
	for (int32 Index = 0; Index < GenesisKarmaDimensionCount; ++Index)
	{
		const EGenesisKarmaDimension Dimension = static_cast<EGenesisKarmaDimension>(Index);
		const float ImpulseValue = Impulse.Get(Dimension);
		// Gewohnheit zählt mit: Wer zuletzt oft so gehandelt hat, dem fällt es leichter
		const float Tendency = FMath::Clamp(Values.Get(Dimension) / 100.0f + 0.3f * Decider.Karma.Habit.Get(Dimension), -1.0f, 1.0f);
		Alignment += Tendency * ImpulseValue;
		ImpulseMagnitude += FMath::Abs(ImpulseValue);
	}
	return FMath::Clamp(WeightedMean(Alignment, ImpulseMagnitude), -1.0f, 1.0f);
}

// ======================================= Motiv =======================================

float UGenesisConsideration_Motive::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const UGenesisLifeActionDefinition& Action = *Option.Action;
	const float Altruism = Decider.Altruism;

	// Altruistische Menschen gewichten den Nutzen anderer, egoistische den eigenen; Kosten schrecken Egoisten stärker ab
	const float Score = Action.OthersBenefit * (0.5f + 0.5f * Altruism)
		+ Action.SelfBenefit * (0.5f - 0.5f * Altruism)
		- Action.CostToSelf * (0.5f - 0.3f * Altruism);
	return FMath::Clamp(Score, -1.0f, 1.0f);
}

// ===================================== Überzeugung =====================================

float UGenesisConsideration_Conviction::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const UGenesisLifeActionDefinition& Action = *Option.Action;

	float Score = 0.0f;
	for (const FGameplayTag& Ideology : Action.IdeologiesAligned)
	{
		Score += GenesisWeightedTags::GetWeight(Decider.IdeologyExposure, Ideology);
	}
	for (const FGameplayTag& Ideology : Action.IdeologiesContradicted)
	{
		Score -= GenesisWeightedTags::GetWeight(Decider.IdeologyExposure, Ideology);
	}

	const FGameplayTag& Tradition = Decider.Belief.Tradition;
	if (Tradition.IsValid())
	{
		if (Action.IdeologiesAligned.HasTag(Tradition))
		{
			Score += Decider.Belief.Certainty;
		}
		if (Action.IdeologiesContradicted.HasTag(Tradition))
		{
			Score -= Decider.Belief.Certainty;
		}
	}
	return FMath::Clamp(Score, -1.0f, 1.0f);
}

// ======================================= Kultur =======================================

float UGenesisConsideration_Culture::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const UGenesisLifeActionDefinition& Action = *Option.Action;
	const FGenesisCulturalIdentity& Culture = Decider.Culture;

	const float Taboo = FMath::Min(1.0f, GenesisMath::CountMatchingTags(Action.GetActionTags(), Culture.TabooTags) * 0.5f);
	const float Valued = FMath::Min(1.0f, GenesisMath::CountMatchingTags(Action.ExpressesValues, Culture.ValuedTags) * 0.5f);
	return FMath::Clamp((Valued - Taboo) * Culture.Adherence, -1.0f, 1.0f);
}

// ======================================== Umfeld ========================================

float UGenesisConsideration_Social::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const UGenesisLifeActionDefinition& Action = *Option.Action;
	float Score = 0.0f;

	// Erwartungen anwesender Gruppen
	for (const FGenesisGroupContext& Group : Situation.Groups)
	{
		if (Group.PresentMemberIds.Num() == 0)
		{
			continue;
		}
		const float Presence = Group.Cohesion * FMath::Min(1.0f, Group.PresentMemberIds.Num() / 3.0f);
		const int32 Support = GenesisMath::CountMatchingTags(Action.ExpressesValues, Group.ExpectedValues)
			+ GenesisMath::CountMatchingTags(Action.ViolatesValues, Group.RejectedValues)
			- GenesisMath::CountMatchingTags(Action.ViolatesValues, Group.ExpectedValues)
			- GenesisMath::CountMatchingTags(Action.ExpressesValues, Group.RejectedValues);
		Score += Presence * FMath::Clamp(Support * 0.5f, -1.0f, 1.0f);
	}

	// Zeitgeist: Mitschwimmen ist leichter
	if (Inputs.State)
	{
		for (const FGenesisWeightedTag& Dominant : Inputs.State->Zeitgeist.DominantValues)
		{
			if (Action.ExpressesValues.HasTag(Dominant.Tag))
			{
				Score += 0.5f * Dominant.Weight;
			}
			if (Action.ViolatesValues.HasTag(Dominant.Tag))
			{
				Score -= 0.5f * Dominant.Weight;
			}
		}
	}

	// Sorge um den eigenen Ruf, wenn jemand zusieht
	if (Situation.WitnessIds.Num() > 0 || Option.TargetIds.Num() > 0)
	{
		const float Exposure = Action.Visibility * FMath::Min(1.0f, (Situation.WitnessIds.Num() + Option.TargetIds.Num()) / 3.0f);
		Score -= 0.3f * Exposure * Action.ViolatesValues.Num();
		Score += 0.15f * Exposure * Action.ExpressesValues.Num();
	}

	return FMath::Clamp(Score, -1.0f, 1.0f);
}

// ====================================== Beziehung ======================================

float UGenesisConsideration_Relationship::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	if (!Inputs.State || Option.TargetIds.Num() == 0)
	{
		return 0.0f;
	}

	float TrustSum = 0.0f;
	for (const FGuid& Target : Option.TargetIds)
	{
		TrustSum += Inputs.State->GetTrust(Decider.EntityId, Target);
	}
	const float AverageTrust = TrustSum / Option.TargetIds.Num();

	// Menschen, denen man vertraut, tut man eher Gutes – Misstrauen macht verletzende Handlungen leichter
	const UGenesisLifeActionDefinition& Action = *Option.Action;
	const float Effect = FMath::Clamp(0.5f * (Action.TrustImpact + Action.OthersBenefit), -1.0f, 1.0f);
	return FMath::Clamp(Effect * AverageTrust, -1.0f, 1.0f);
}

// ====================================== Erfahrung ======================================

float UGenesisConsideration_Experience::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const FGenesisMemoryStore* Store = Inputs.Memory ? Inputs.Memory->FindStore(Decider.EntityId) : nullptr;
	if (!Store)
	{
		return 0.0f;
	}

	const FGameplayTagContainer& ActionThemes = Option.Action->Themes;
	if (ActionThemes.Num() == 0)
	{
		return 0.0f;
	}

	float WeightedValence = 0.0f;
	float WeightTotal = 0.0f;
	for (const FGenesisMemoryTrace& Trace : Store->Traces)
	{
		if (!Trace.PerceivedThemes.HasAny(ActionThemes))
		{
			continue;
		}

		// Nur bewusst zugängliche Erinnerungen – Verdrängtes wirkt im Unterbewusstsein
		const float Accessibility = Trace.Intensity * (1.0f - Trace.Repression);
		if (Accessibility <= 0.01f)
		{
			continue;
		}

		// Eigene Taten sagen "wie war es, so zu handeln", Erlittenes sagt "wie fühlt es sich an"
		const bool bWasActor = Trace.Perspective == EGenesisMemoryPerspective::Actor;
		WeightedValence += Accessibility * Trace.Valence * (bWasActor ? 1.0f : 0.8f);
		WeightTotal += Accessibility;
		OutInfluences.Emplace(Trace.EventId, Accessibility);
	}

	// Blasse oder weitgehend verdrängte Erfahrungen prägen die bewusste Abwägung nur schwach
	const float TotalAccessibility = FMath::Min(1.0f, WeightTotal);
	return FMath::Clamp(WeightedMean(WeightedValence, WeightTotal) * TotalAccessibility, -1.0f, 1.0f);
}

// =================================== Unterbewusstsein ===================================

float UGenesisConsideration_Subconscious::Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option,
	const FGenesisLifeProfile& Decider, const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const
{
	const UGenesisLifeActionDefinition& Action = *Option.Action;
	const FGameplayTagContainer Themes = OptionThemes(Situation, Action);
	float Score = 0.0f;

	// 1) Verdrängte, schmerzhafte Erinnerungen erzeugen Vermeidung der Themen
	if (const FGenesisMemoryStore* Store = Inputs.Memory ? Inputs.Memory->FindStore(Decider.EntityId) : nullptr)
	{
		float Avoidance = 0.0f;
		for (const FGenesisMemoryTrace& Trace : Store->Traces)
		{
			if (Trace.Repression <= 0.0f || Trace.Valence >= 0.0f || !Trace.PerceivedThemes.HasAny(Action.Themes))
			{
				continue;
			}
			const float Pull = Trace.Intensity * Trace.Repression * -Trace.Valence;
			Avoidance += Pull;
			OutInfluences.Emplace(Trace.EventId, Pull);
		}
		Score -= FMath::Min(1.0f, Avoidance);
	}

	// 2) Belastung drängt zu Selbstschutz
	Score += Decider.Stress * (Action.SelfBenefit - Action.CostToSelf);

	// 3) Seelen-Echos: Offene Lebensthemen ziehen an – ein Echo, keine Strafe
	if (Inputs.DeciderSoul)
	{
		Score += 0.6f * GenesisSoulLogic::ComputeEchoPull(*Inputs.DeciderSoul, Themes);
	}

	return FMath::Clamp(Score, -1.0f, 1.0f);
}
