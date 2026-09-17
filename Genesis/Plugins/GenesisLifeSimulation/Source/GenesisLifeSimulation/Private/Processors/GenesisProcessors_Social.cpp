// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationProcessors.h"
#include "GenesisGameplayTags.h"
#include "GenesisLifeAction.h"
#include "GenesisMath.h"
#include "GenesisMemoryWorld.h"
#include "GenesisWorldClockSubsystem.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

// ===================================== Vertrauen =====================================

FGameplayTag UGenesisProcessor_Trust::GetSystemTag() const
{
	return GenesisTags::SimSystem_Trust;
}

void UGenesisProcessor_Trust::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;

	for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		if (!Perception.bNoticed || Perception.ObserverId == Action.ActorId || !Context.State.FindProfile(Perception.ObserverId))
		{
			continue;
		}

		const float RoleFactor = Perception.Role == EGenesisMemoryPerspective::Target ? 1.0f : Tuning.WitnessTrustFactor;

		// Vertrauen folgt der WAHRGENOMMENEN Absicht – nicht der tatsächlichen
		float Delta = Tuning.TrustScale * RoleFactor * (0.5f * Perception.PerceivedIntent + 0.5f * Definition.TrustImpact);
		Delta -= 0.2f * RoleFactor * Perception.HypocrisyPerceived;
		Delta -= 0.15f * Perception.TabooOffense;
		Delta += 0.05f * Perception.GroupStance;

		FGenesisTrustEdge& Edge = Context.State.FindOrAddTrust(Perception.ObserverId, Action.ActorId);
		if (Delta < 0.0f)
		{
			// Negatives wiegt schwerer – und Enttäuschung aus hohem Vertrauen am schwersten
			Delta *= Tuning.NegativityBias * (1.0f + FMath::Max(0.0f, Edge.Trust));
			if (Delta < Tuning.BetrayalThreshold && Edge.Trust > 0.3f)
			{
				++Edge.Betrayals;
			}
		}

		Edge.Trust = GenesisMath::ApplySaturatingDelta(Edge.Trust, Delta, -1.0f, 1.0f, 1.0f);
		Edge.Familiarity = FMath::Min(1.0f, Edge.Familiarity + 0.01f + 0.02f * Frame.Magnitude);
		Edge.LastInteraction = Frame.Time;
		Edge.LastEventId = Frame.EventId;
	}
}

// ================================= Ruf & Gerüchte =================================

FGameplayTag UGenesisProcessor_ReputationRumors::GetSystemTag() const
{
	return GenesisTags::SimSystem_ReputationRumors;
}

void UGenesisProcessor_ReputationRumors::ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
	FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
{
	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;

	TArray<FGenesisWeightedTag> Effects;
	for (const FGameplayTag& Value : Definition.ExpressesValues)
	{
		Effects.Emplace(Value, Tuning.ReputationScale);
	}
	for (const FGameplayTag& Value : Definition.ViolatesValues)
	{
		Effects.Emplace(Value, -Tuning.ReputationScale * (1.0f + Frame.Hypocrisy));
	}
	if (Effects.Num() == 0)
	{
		return;
	}

	TArray<FGuid> NoticingWitnesses;
	TArray<FGameplayTagContainer> WitnessThemes;
	float TruthSum = 0.0f;
	float MaxTaboo = 0.0f;

	for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		if (!Perception.bNoticed)
		{
			continue;
		}
		MaxTaboo = FMath::Max(MaxTaboo, Perception.TabooOffense);

		if (Perception.Role == EGenesisMemoryPerspective::Witness)
		{
			NoticingWitnesses.Add(Perception.ObserverId);
			WitnessThemes.Add(Perception.PerceivedThemes);
			TruthSum += Perception.bMisread ? 0.4f : 0.95f;
		}

		const FGenesisLifeProfile* Observer = Context.State.FindProfile(Perception.ObserverId);
		if (!Observer)
		{
			continue;
		}

		const float RoleFactor = Perception.Role == EGenesisMemoryPerspective::Target ? 1.0f : Tuning.WitnessReputationFactor;
		const float MisreadFactor = Perception.bMisread ? (Perception.PerceivedIntent >= 0.0f ? 0.5f : -0.5f) : 1.0f;
		// Wer dem Zeitgeist widerspricht, wird von angepassten Menschen strenger beurteilt
		const float ZeitgeistHarshness = Frame.ZeitgeistAlignment < 0.0f ? 1.0f + 0.5f * -Frame.ZeitgeistAlignment * Observer->Culture.Adherence : 1.0f;

		for (const FGenesisWeightedTag& Effect : Effects)
		{
			float Delta = Effect.Weight * RoleFactor * MisreadFactor;
			if (Delta < 0.0f)
			{
				Delta *= ZeitgeistHarshness;
			}
			FGenesisReputationEntry& Entry = Context.State.FindOrAddReputation(Action.ActorId, Perception.ObserverId, Effect.Tag);
			Entry.Score = GenesisMath::ApplySaturatingDelta(Entry.Score, Delta, -1.0f, 1.0f, 1.0f);
			Entry.Confidence = FMath::Min(1.0f, Entry.Confidence + 0.2f);
		}
	}

	// Gerücht entsteht, wenn es Zeugen gibt und die Geschichte "erzählenswert" ist
	const float Juiciness = FMath::Clamp(FMath::Abs(Definition.Valence) * 0.5f + MaxTaboo * 0.3f + Frame.Hypocrisy * 0.4f + Frame.Magnitude * 0.2f, 0.0f, 1.0f);
	if (NoticingWitnesses.Num() == 0 || Juiciness < Tuning.RumorMinJuiciness)
	{
		return;
	}

	FGenesisRumor& Rumor = Context.State.Rumors.AddDefaulted_GetRef();
	Rumor.RumorId = Context.Rng().NewGuid();
	Rumor.SourceEventId = Frame.EventId;
	Rumor.SubjectId = Action.ActorId;
	Rumor.ClaimThemes = WitnessThemes[0];
	Rumor.ReputationEffects = Effects;
	Rumor.Truth = TruthSum / NoticingWitnesses.Num();
	Rumor.Juiciness = Juiciness;
	Rumor.Holders = NoticingWitnesses;
	Rumor.CreatedAt = Frame.Time;
	Rumor.LastSpreadAt = Frame.Time;
	Frame.SpawnedRumorIds.Add(Rumor.RumorId);
}

void UGenesisProcessor_ReputationRumors::AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const
{
	if (!Context.bDailyTick || Context.State.Rumors.Num() == 0)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisLife_RumorSpread);

	const FGenesisLifeSimulationTuning& Tuning = Context.Tuning;
	FGenesisLifeSimulationState& State = Context.State;
	FGenesisRandomStream& Rng = Context.Rng();

	// Soziales Netz: Wer kennt wen (in beide Richtungen), mit welcher Vertrautheit?
	TMap<FGuid, TArray<TPair<FGuid, float>>> Neighbors;
	for (const FGenesisTrustEdge& Edge : State.TrustEdges)
	{
		if (Edge.Familiarity >= 0.1f)
		{
			Neighbors.FindOrAdd(Edge.FromId).Emplace(Edge.ToId, Edge.Familiarity);
			Neighbors.FindOrAdd(Edge.ToId).Emplace(Edge.FromId, Edge.Familiarity);
		}
	}

	for (FGenesisRumor& Rumor : State.Rumors)
	{
		const double Days = static_cast<double>(Context.Now - Rumor.LastSpreadAt) / static_cast<double>(FGenesisTimestamp::SecondsPerDay);
		if (Days <= 0.0)
		{
			continue;
		}

		TArray<FGuid> NewHolders;
		const int32 TellerCount = FMath::Min(Rumor.Holders.Num(), Tuning.RumorMaxHoldersPerStep);
		for (int32 TellerIndex = 0; TellerIndex < TellerCount; ++TellerIndex)
		{
			const FGuid Teller = Rumor.Holders[TellerIndex];
			const TArray<TPair<FGuid, float>>* Contacts = Neighbors.Find(Teller);
			if (!Contacts)
			{
				continue;
			}

			for (const TPair<FGuid, float>& Contact : *Contacts)
			{
				const FGuid& Listener = Contact.Key;
				if (Listener == Rumor.SubjectId || Rumor.Holders.Contains(Listener) || NewHolders.Contains(Listener))
				{
					continue;
				}

				const double SpreadChance = 1.0 - FMath::Exp(-Tuning.RumorSpreadRatePerDay * Rumor.Juiciness * Contact.Value * Days);
				if (!Rng.Bernoulli(static_cast<float>(SpreadChance)))
				{
					continue;
				}

				NewHolders.Add(Listener);

				// Stille Post: Beim Weitererzählen verzerrt sich die Geschichte
				if (Rng.Bernoulli(Tuning.RumorDistortionPerHop))
				{
					Rumor.Truth *= 0.85f;
					if (Rumor.ClaimThemes.Num() > 1)
					{
						TArray<FGameplayTag> Tags;
						Rumor.ClaimThemes.GetGameplayTagArray(Tags);
						Rumor.ClaimThemes.RemoveTag(Tags[Rng.RandRange(0, Tags.Num() - 1)]);
					}
				}

				const FGenesisLifeProfile* ListenerProfile = State.FindProfile(Listener);
				if (!ListenerProfile)
				{
					continue;
				}

				// Geglaubt wird nach Vertrauen in den Erzähler – nicht nach Wahrheit
				const float Credibility = 0.5f * (State.GetTrust(Listener, Teller) + 1.0f) * 0.6f;
				for (const FGenesisWeightedTag& Effect : Rumor.ReputationEffects)
				{
					FGenesisReputationEntry& Entry = State.FindOrAddReputation(Rumor.SubjectId, Listener, Effect.Tag);
					Entry.Score = GenesisMath::ApplySaturatingDelta(Entry.Score, Effect.Weight * Credibility, -1.0f, 1.0f, 1.0f);
					Entry.Confidence = FMath::Min(1.0f, Entry.Confidence + 0.1f);
				}

				// Hörensagen wird Teil des Gedächtnisses – mit den verzerrten Themen
				if (Context.Memory && ListenerProfile->SimulationLevel != EGenesisSimulationLevel::Statistical)
				{
					FGenesisEncodingContext Encoding;
					Encoding.Perspective = EGenesisMemoryPerspective::Hearsay;
					Encoding.Arousal = 0.3f * Rumor.Juiciness;
					Encoding.bHasPerceptionOverride = true;
					Encoding.PerceivedThemesOverride = Rumor.ClaimThemes;
					Encoding.PerceivedActorOverride = Rumor.SubjectId;
					Context.Memory->EncodeMemory(Listener, Rumor.SourceEventId, Encoding, Context.Now);
				}
			}
		}

		if (NewHolders.Num() > 0)
		{
			Rumor.Holders.Append(NewHolders);
			++Rumor.Hops;
		}

		Rumor.Juiciness *= static_cast<float>(FMath::Pow(0.5, Days / FMath::Max(1.0f, Tuning.RumorHalfLifeDays)));
		Rumor.LastSpreadAt = Context.Now;
	}

	// Ausgeschwiegene Gerüchte verlassen den aktiven Zustand (das Ereignis bleibt im Kausalgraph)
	State.Rumors.RemoveAll([](const FGenesisRumor& Rumor) { return Rumor.Juiciness < 0.01f; });
}
