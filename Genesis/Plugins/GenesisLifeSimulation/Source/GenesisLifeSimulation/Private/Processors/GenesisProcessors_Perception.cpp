// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationProcessors.h"
#include "GenesisGameplayTags.h"
#include "GenesisLifeAction.h"
#include "GenesisMath.h"

// ============================== Kulturelle Identität ==============================

FGameplayTag UGenesisProcessor_CulturalIdentity::GetSystemTag() const
{
	return GenesisTags::SimSystem_CulturalIdentity;
}

void UGenesisProcessor_CulturalIdentity::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);
	if (!Actor)
	{
		return;
	}

	const FGameplayTagContainer ActionTags = Definition.GetActionTags();
	FGenesisCulturalIdentity& ActorCulture = Actor->Culture;

	// Tabubruch in der eigenen Kultur erzeugt Scham und inneren Widerspruch – und lockert die Bindung
	Frame.TabooViolation = FMath::Min(1.0f, GenesisMath::CountMatchingTags(ActionTags, ActorCulture.TabooTags) * 0.5f) * ActorCulture.Adherence;
	if (Frame.TabooViolation > 0.0f)
	{
		Frame.StressLoad += 0.2f * Frame.TabooViolation;
		Frame.Dissonance += 0.3f * Frame.TabooViolation;
		ActorCulture.Adherence = FMath::Clamp(ActorCulture.Adherence - 0.02f * Frame.TabooViolation, 0.0f, 1.0f);
	}
	else if (GenesisMath::CountMatchingTags(Definition.ExpressesValues, ActorCulture.ValuedTags) > 0)
	{
		ActorCulture.Adherence = FMath::Clamp(ActorCulture.Adherence + 0.01f * Frame.Intensity, 0.0f, 1.0f);
	}

	// Beobachter: Wie fremd ist ihnen die Kultur des Handelnden, verletzt die Handlung ihre Tabus?
	for (FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		const FGenesisLifeProfile* Observer = Context.State.FindProfile(Perception.ObserverId);
		if (!Observer)
		{
			// Unbekannte Person: mittlere Distanz, keine bekannten Tabus
			Perception.CulturalDistance = 0.5f;
			continue;
		}

		float Distance = 1.0f - GenesisMath::TagSimilarity(ActorCulture.ValuedTags, Observer->Culture.ValuedTags);
		if (ActorCulture.Culture.IsValid() && ActorCulture.Culture == Observer->Culture.Culture)
		{
			Distance *= 0.5f;
		}
		Perception.CulturalDistance = Distance;
		Perception.TabooOffense = FMath::Min(1.0f, GenesisMath::CountMatchingTags(ActionTags, Observer->Culture.TabooTags) * 0.5f) * Observer->Culture.Adherence;
	}
}

// ================================= Missverständnis =================================

FGameplayTag UGenesisProcessor_Misunderstanding::GetSystemTag() const
{
	return GenesisTags::SimSystem_Misunderstanding;
}

void UGenesisProcessor_Misunderstanding::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;
	FGenesisRandomStream& Rng = Context.Rng();

	for (FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		const FGenesisLifeProfile* Observer = Context.State.FindProfile(Perception.ObserverId);
		const float Familiarity = Context.State.GetFamiliarity(Perception.ObserverId, Action.ActorId);
		const float ObserverStress = Observer ? Observer->Stress : 0.0f;
		const bool bIsTarget = Perception.Role == EGenesisMemoryPerspective::Target;

		// Bemerkt? Betroffene bemerken Handlungen gegen sich eher
		const float NoticeChance = FMath::Clamp(Definition.Visibility * (bIsTarget ? 1.6f : 1.0f), 0.0f, 1.0f);
		Perception.bNoticed = Rng.Bernoulli(NoticeChance);
		if (!Perception.bNoticed)
		{
			continue;
		}

		const float MisreadChance = FMath::Clamp(
			Tuning.BaseMisreadChance
			+ Definition.Ambiguity * Tuning.AmbiguityMisreadWeight
			+ Perception.CulturalDistance * Tuning.CulturalDistanceMisreadWeight
			+ ObserverStress * Tuning.StressMisreadWeight
			- Familiarity * Tuning.FamiliarityMisreadReduction,
			0.0f, Tuning.MaxMisreadChance);

		Perception.bMisread = Rng.Bernoulli(MisreadChance);
		Perception.PerceivedThemes = Definition.Themes;

		if (Perception.bMisread)
		{
			// Fehldeutung: Absicht wird oft misstrauischer gelesen, ein Aspekt geht verloren
			Perception.PerceivedIntent = FMath::Clamp(-0.5f * Frame.ActualIntent + Rng.Gaussian(0.0f, 0.35f) - 0.3f * Perception.TabooOffense, -1.0f, 1.0f);
			if (Perception.PerceivedThemes.Num() > 1)
			{
				TArray<FGameplayTag> Tags;
				Perception.PerceivedThemes.GetGameplayTagArray(Tags);
				Perception.PerceivedThemes.RemoveTag(Tags[Rng.RandRange(0, Tags.Num() - 1)]);
			}
		}
		else
		{
			Perception.PerceivedIntent = FMath::Clamp(Frame.ActualIntent + Rng.Gaussian(0.0f, 0.1f) - 0.3f * Perception.TabooOffense, -1.0f, 1.0f);
		}

		Perception.Arousal = FMath::Clamp(
			FMath::Abs(Definition.Valence) * 0.6f + Frame.Magnitude * 0.4f + Perception.TabooOffense * 0.3f + (bIsTarget ? 0.15f : 0.0f),
			0.0f, 1.0f);
	}
}
