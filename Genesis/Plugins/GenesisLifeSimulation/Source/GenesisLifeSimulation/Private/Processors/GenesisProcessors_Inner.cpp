// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationProcessors.h"
#include "GenesisCausalGraph.h"
#include "GenesisGameplayTags.h"
#include "GenesisLifeAction.h"
#include "GenesisMath.h"
#include "GenesisMemoryWorld.h"
#include "GenesisWorldClockSubsystem.h"

// ================================== Indoktrination ==================================

FGameplayTag UGenesisProcessor_Indoctrination::GetSystemTag() const
{
	return GenesisTags::SimSystem_Indoctrination;
}

void UGenesisProcessor_Indoctrination::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);
	if (!Actor)
	{
		return;
	}

	float Aligned = 0.0f;
	for (const FGameplayTag& Ideology : Definition.IdeologiesAligned)
	{
		const float Exposure = GenesisWeightedTags::GetWeight(Actor->IdeologyExposure, Ideology);
		if (Exposure > 0.0f)
		{
			Aligned += Exposure;
			// Danach handeln vertieft die Überzeugung
			GenesisWeightedTags::AddWeight(Actor->IdeologyExposure, Ideology, Context.Tuning.IndoctrinationHabituation * Frame.Intensity * (1.0f - Exposure), 0.0f, 1.0f);
		}
	}

	float Contradicted = 0.0f;
	for (const FGameplayTag& Ideology : Definition.IdeologiesContradicted)
	{
		const float Exposure = GenesisWeightedTags::GetWeight(Actor->IdeologyExposure, Ideology);
		if (Exposure > 0.0f)
		{
			Contradicted += Exposure;
			// Dagegen handeln erzeugt Dissonanz – und lockert schwächere Prägungen
			Frame.Dissonance += 0.5f * Exposure;
			GenesisWeightedTags::AddWeight(Actor->IdeologyExposure, Ideology, -Context.Tuning.IndoctrinationErosion * Frame.Intensity * (1.0f - Exposure), 0.0f, 1.0f);
		}
	}

	Frame.IndoctrinationAlignment = FMath::Clamp(Aligned - Contradicted, -1.0f, 1.0f);
}

// ======================================= Glaube =======================================

FGameplayTag UGenesisProcessor_Belief::GetSystemTag() const
{
	return GenesisTags::SimSystem_Belief;
}

void UGenesisProcessor_Belief::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);
	if (!Actor)
	{
		return;
	}

	FGenesisBeliefProfile& Belief = Actor->Belief;
	const float Rate = Context.Tuning.BeliefShiftRate * Frame.Intensity;
	Belief.Certainty = GenesisMath::ApplySaturatingDelta(Belief.Certainty, Definition.BeliefCertaintyImpulse * Rate, 0.0f, 1.0f, 1.0f);
	Belief.Openness = GenesisMath::ApplySaturatingDelta(Belief.Openness, Definition.BeliefOpennessImpulse * Rate, 0.0f, 1.0f, 1.0f);

	if (Belief.Tradition.IsValid())
	{
		if (Definition.IdeologiesAligned.HasTag(Belief.Tradition))
		{
			Frame.BeliefConsistency = 1.0f;
			Frame.Dissonance = FMath::Max(0.0f, Frame.Dissonance - 0.1f);
		}
		else if (Definition.IdeologiesContradicted.HasTag(Belief.Tradition))
		{
			Frame.BeliefConsistency = -1.0f;
			Frame.Dissonance += 0.3f * Belief.Certainty;
		}
	}

	// Verlorene Gewissheit öffnet die Haltung – ohne Richtung vorzugeben
	if (Belief.Certainty < 0.15f && (Belief.Stance == EGenesisBeliefStance::Religious || Belief.Stance == EGenesisBeliefStance::Atheist))
	{
		Belief.Stance = Belief.Openness > 0.6f ? EGenesisBeliefStance::Seeking : EGenesisBeliefStance::Agnostic;
	}
}

// ===================================== Propaganda =====================================

FGameplayTag UGenesisProcessor_Propaganda::GetSystemTag() const
{
	return GenesisTags::SimSystem_Propaganda;
}

void UGenesisProcessor_Propaganda::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	// Einflüsse, die diese Entscheidung geformt haben, werden später als Ursachen verknüpft
	Frame.InfluenceEventIds = Action.InfluenceEventIds;

	if (!Definition.PropagatedIdeology.IsValid() || Definition.ManipulationStrength <= 0.0f)
	{
		return;
	}

	for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		if (!Perception.bNoticed)
		{
			continue;
		}

		FGenesisLifeProfile* Observer = Context.State.FindProfile(Perception.ObserverId);
		if (!Observer)
		{
			continue;
		}

		// Empfänglichkeit: Vertrauen in die Quelle und Belastung erhöhen, Weisheit senkt
		const float TrustInSource = Context.State.GetTrust(Perception.ObserverId, Action.ActorId);
		const float Wisdom = Observer->Karma.Values.Wisdom / 100.0f;
		float Susceptibility = FMath::Clamp(0.5f + 0.4f * TrustInSource - 0.3f * Wisdom + 0.2f * Observer->Stress, 0.05f, 1.0f);
		if (Perception.bMisread)
		{
			Susceptibility *= 0.3f;
		}

		const float Delta = Definition.ManipulationStrength * Frame.Intensity * Context.Tuning.PropagandaGain * Susceptibility;
		GenesisWeightedTags::AddWeight(Observer->IdeologyExposure, Definition.PropagatedIdeology, Delta, 0.0f, 1.0f);
	}
}

// ===================================== Zeitgeist =====================================

FGameplayTag UGenesisProcessor_Zeitgeist::GetSystemTag() const
{
	return GenesisTags::SimSystem_Zeitgeist;
}

void UGenesisProcessor_Zeitgeist::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisZeitgeistState& Zeitgeist = Context.State.Zeitgeist;

	float Alignment = 0.0f;
	for (const FGenesisWeightedTag& Dominant : Zeitgeist.DominantValues)
	{
		if (Definition.ExpressesValues.HasTag(Dominant.Tag))
		{
			Alignment += Dominant.Weight;
		}
		if (Definition.ViolatesValues.HasTag(Dominant.Tag))
		{
			Alignment -= Dominant.Weight;
		}
	}
	Frame.ZeitgeistAlignment = FMath::Clamp(Alignment, -1.0f, 1.0f);

	if (Frame.ZeitgeistAlignment < 0.0f)
	{
		Frame.StressLoad += Context.Tuning.ZeitgeistResistanceStress * -Frame.ZeitgeistAlignment;
	}

	// Nur wahrgenommene Handlungen verändern die Gesellschaft
	if (Frame.CountNoticed() == 0)
	{
		return;
	}

	const float Pressure = Context.Tuning.ZeitgeistPressureGain * Frame.Magnitude;
	for (const FGameplayTag& Value : Definition.ExpressesValues)
	{
		GenesisWeightedTags::AddWeight(Zeitgeist.PendingPressure, Value, Pressure, -1.0f, 1.0f);
	}
	for (const FGameplayTag& Value : Definition.ViolatesValues)
	{
		GenesisWeightedTags::AddWeight(Zeitgeist.PendingPressure, Value, -Pressure, -1.0f, 1.0f);
	}
}

void UGenesisProcessor_Zeitgeist::AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const
{
	if (!Context.bYearlyTick)
	{
		return;
	}

	// Gesellschaftlicher Druck wird einmal jährlich in die dominanten Werte eingearbeitet
	FGenesisZeitgeistState& Zeitgeist = Context.State.Zeitgeist;
	for (const FGenesisWeightedTag& Pressure : Zeitgeist.PendingPressure)
	{
		GenesisWeightedTags::AddWeight(Zeitgeist.DominantValues, Pressure.Tag, Pressure.Weight, 0.0f, 1.0f);
	}
	Zeitgeist.PendingPressure.Reset();
	GenesisWeightedTags::RemoveNegligible(Zeitgeist.DominantValues, 0.001f);
}

// =================================== Gruppenzwang ===================================

FGameplayTag UGenesisProcessor_GroupPressure::GetSystemTag() const
{
	return GenesisTags::SimSystem_GroupPressure;
}

void UGenesisProcessor_GroupPressure::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	float TotalPressure = 0.0f;

	for (const FGenesisGroupContext& Group : Action.Groups)
	{
		const int32 Present = Group.PresentMemberIds.Num();
		if (Present == 0)
		{
			continue;
		}

		const float Weight = FMath::Clamp(Group.Cohesion, 0.0f, 1.0f) * FMath::Min(1.0f, Present / 3.0f);
		const int32 Support = GenesisMath::CountMatchingTags(Definition.ExpressesValues, Group.ExpectedValues)
			+ GenesisMath::CountMatchingTags(Definition.ViolatesValues, Group.RejectedValues)
			- GenesisMath::CountMatchingTags(Definition.ViolatesValues, Group.ExpectedValues)
			- GenesisMath::CountMatchingTags(Definition.ExpressesValues, Group.RejectedValues);
		const float Stance = FMath::Clamp(Support * 0.5f, -1.0f, 1.0f);

		TotalPressure += Weight * Stance;

		for (const FGuid& Member : Group.PresentMemberIds)
		{
			if (FGenesisObserverPerception* Perception = Frame.FindPerception(Member))
			{
				Perception->GroupStance = Stance;
			}
		}
	}

	Frame.GroupPressure = FMath::Clamp(TotalPressure, -1.0f, 1.0f);

	if (Frame.GroupPressure < -0.2f)
	{
		// Gegen die Gruppe handeln erfordert Mut und kostet Kraft
		Frame.KarmaImpulse.Courage += Context.Tuning.GroupDefianceCourage * -Frame.GroupPressure;
		Frame.StressLoad += Context.Tuning.GroupPressureStress * -Frame.GroupPressure;
	}
	else if (Frame.GroupPressure > 0.2f && Frame.Dissonance > 0.2f)
	{
		// Mitmachen gegen die eigene Überzeugung
		Frame.bConformedAgainstConviction = true;
		Frame.KarmaImpulse.Courage -= 0.5f * Context.Tuning.GroupDefianceCourage * Frame.GroupPressure;
		Frame.Dissonance += 0.1f;
	}
}

// ================================ Egoismus / Altruismus ================================

FGameplayTag UGenesisProcessor_EgoismAltruism::GetSystemTag() const
{
	return GenesisTags::SimSystem_EgoismAltruism;
}

void UGenesisProcessor_EgoismAltruism::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	float Signal = Definition.OthersBenefit - 0.5f * Definition.SelfBenefit;
	if (Definition.OthersBenefit > 0.0f)
	{
		// Ein Opfer wiegt schwerer als Hilfe ohne eigene Kosten
		Signal *= 1.0f + Definition.CostToSelf;
	}
	Frame.AltruismSignal = FMath::Clamp(Signal, -1.0f, 1.0f);

	if (Frame.KarmaImpulse.Generosity > 0.0f)
	{
		Frame.KarmaImpulse.Generosity *= 1.0f + 0.5f * Definition.CostToSelf;
	}

	if (FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId))
	{
		Actor->Altruism = GenesisMath::ApplySaturatingDelta(Actor->Altruism, Frame.AltruismSignal * Context.Tuning.AltruismShift * Frame.Intensity, -1.0f, 1.0f, 1.0f);
	}
}

// ==================================== Doppelmoral ====================================

FGameplayTag UGenesisProcessor_DoubleStandard::GetSystemTag() const
{
	return GenesisTags::SimSystem_DoubleStandard;
}

void UGenesisProcessor_DoubleStandard::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	FGenesisLifeProfile* Actor = Context.State.FindProfile(Action.ActorId);
	if (!Actor)
	{
		return;
	}

	float Hypocrisy = 0.0f;

	// a) Die Handlung verletzt Werte, die die Person öffentlich vertritt
	for (const FGameplayTag& Violated : Definition.ViolatesValues)
	{
		Hypocrisy += GenesisWeightedTags::GetWeight(Actor->StatedValues, Violated);
	}

	// b) Öffentliches Eintreten für Werte, die man selbst verletzt hat
	if (Definition.bIsAdvocacy && Definition.ExpressesValues.Num() > 0)
	{
		if (Context.Memory)
		{
			FGenesisEventQuery Query;
			Query.InvolvedEntityId = Action.ActorId;
			Query.bInvolvedAsActorOnly = true;
			Query.AnyViolatedValues = Definition.ExpressesValues;
			Query.MaxResults = 8;
			const int32 PastViolations = Context.Memory->GetGraph().FindEvents(Query).Num();
			Hypocrisy += FMath::Min(1.0f, PastViolations * 0.25f);
		}

		for (const FGameplayTag& Value : Definition.ExpressesValues)
		{
			GenesisWeightedTags::AddWeight(Actor->StatedValues, Value, Context.Tuning.AdvocacyGain * Frame.Intensity, 0.0f, 1.0f);
		}
	}

	Frame.Hypocrisy = FMath::Clamp(Hypocrisy, 0.0f, 1.0f);
	if (Frame.Hypocrisy <= 0.0f)
	{
		return;
	}

	Frame.Dissonance += 0.3f * Frame.Hypocrisy;
	Frame.KarmaImpulse.Honesty -= 3.0f * Frame.Hypocrisy;

	// Wer die Person gut kennt, erkennt den Widerspruch eher
	for (FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		if (Perception.bNoticed)
		{
			const float Familiarity = Context.State.GetFamiliarity(Perception.ObserverId, Action.ActorId);
			Perception.HypocrisyPerceived = Frame.Hypocrisy * (0.4f + 0.6f * Familiarity);
		}
	}
}
