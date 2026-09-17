// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoulLogic.h"
#include "GenesisMath.h"
#include "GenesisRandom.h"

namespace GenesisSoulLogic
{
	namespace
	{
		/** Salt für den Zufallsstrom eines Lebensabschlusses. */
		constexpr uint64 ClosureSalt = 0x50C1C105Eull;

		constexpr int32 MinInterval = -7;
		constexpr int32 MaxInterval = 7;

		/** Sättigende Addition gegen 1. */
		float SaturateTowardsOne(float Current, float Amount)
		{
			return FMath::Clamp(Current + FMath::Clamp(Amount, 0.0f, 1.0f) * (1.0f - Current), 0.0f, 1.0f);
		}

		void GenerateMotif(FGenesisSoulMotif& Motif, FGenesisRandomStream& Rng)
		{
			const int32 NoteCount = Rng.RandRange(4, 7);
			Motif.ModeIndex = Rng.RandRange(0, 6);
			Motif.Tension = 0.2f;
			Motif.Warmth = 0.5f;
			Motif.Variation = 0;

			static const int32 DurationChoices[] = { 2, 4, 4, 6, 8 };

			Motif.Durations.Reset(NoteCount);
			for (int32 Index = 0; Index < NoteCount; ++Index)
			{
				Motif.Durations.Add(DurationChoices[Rng.RandRange(0, UE_ARRAY_COUNT(DurationChoices) - 1)]);
			}

			// Melodische Kontur: überwiegend Schritte, gelegentlich Sprünge – wirkt wie eine erinnerbare Phrase
			Motif.Intervals.Reset(NoteCount - 1);
			for (int32 Index = 0; Index < NoteCount - 1; ++Index)
			{
				const bool bLeap = Rng.Bernoulli(0.3f);
				const int32 Magnitude = bLeap ? Rng.RandRange(3, MaxInterval) : Rng.RandRange(1, 2);
				Motif.Intervals.Add(Rng.Bernoulli(0.5f) ? Magnitude : -Magnitude);
			}
		}

		void EvolveMotif(FGenesisSoulSeed& Soul, const FGenesisLifeClosure& Closure, FGenesisRandomStream& Rng)
		{
			FGenesisSoulMotif& Motif = Soul.Motif;
			++Motif.Variation;

			float OpenWeightSum = 0.0f;
			int32 OpenCount = 0;
			for (const FGenesisSoulEcho& Echo : Soul.Echoes)
			{
				if (Echo.State == EGenesisEchoState::Open)
				{
					OpenWeightSum += Echo.Weight;
					++OpenCount;
				}
			}
			const float OpenAverage = OpenCount > 0 ? OpenWeightSum / static_cast<float>(OpenCount) : 0.0f;

			Motif.Tension = GenesisMath::ApproachExponential(Motif.Tension, OpenAverage, 0.5f, 1.0);
			Motif.Warmth = GenesisMath::ApproachExponential(Motif.Warmth, FMath::Clamp(Closure.Warmth, 0.0f, 1.0f), 0.5f, 1.0);

			// Nur Intervalle nach dem Identitätskern verändern sich – die Seele bleibt wiedererkennbar
			if (Motif.Intervals.Num() > FGenesisSoulMotif::IdentityCoreLength)
			{
				const int32 Target = Rng.RandRange(FGenesisSoulMotif::IdentityCoreLength, Motif.Intervals.Num() - 1);
				const int32 Delta = Rng.Bernoulli(0.5f) ? 1 : -1;
				int32 NewInterval = FMath::Clamp(Motif.Intervals[Target] + Delta, MinInterval, MaxInterval);
				if (NewInterval == 0)
				{
					NewInterval = Delta;
				}
				Motif.Intervals[Target] = NewInterval;
			}

			// Modus folgt der emotionalen Grundfarbe, nicht einer Bewertung
			if (Motif.Tension > 0.6f && Rng.Bernoulli(0.3f))
			{
				Motif.ModeIndex = Rng.Bernoulli(0.5f) ? 5 : 2; // Äolisch / Phrygisch
			}
			else if (Motif.Warmth > 0.7f && Motif.Tension < 0.3f && Rng.Bernoulli(0.3f))
			{
				Motif.ModeIndex = Rng.Bernoulli(0.5f) ? 0 : 4; // Ionisch / Mixolydisch
			}
		}
	}

	FGenesisSoulMotif GenerateMotifFromSeed(uint64 Seed)
	{
		FGenesisRandomStream Rng(Seed);
		FGenesisSoulMotif Motif;
		GenerateMotif(Motif, Rng);
		return Motif;
	}

	FGenesisSoulSeed CreateSoulSeed(uint64 OriginSeed, const TArray<FGameplayTag>& InnatePatternPool)
	{
		FGenesisRandomStream Rng(OriginSeed);

		FGenesisSoulSeed Soul;
		Soul.OriginSeed = OriginSeed;
		Soul.SoulId = Rng.NewGuid();

		GenerateMotif(Soul.Motif, Rng);

		TArray<FGameplayTag> Pool;
		for (const FGameplayTag& Tag : InnatePatternPool)
		{
			if (Tag.IsValid())
			{
				Pool.AddUnique(Tag);
			}
		}

		if (Pool.Num() > 0)
		{
			const int32 Count = Rng.RandRange(1, FMath::Min(2, Pool.Num()));
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const int32 Pick = Rng.RandRange(0, Pool.Num() - 1);
				FGenesisSoulResonance& Resonance = Soul.Resonances.AddDefaulted_GetRef();
				Resonance.Pattern = Pool[Pick];
				Resonance.Intensity = Rng.FRandRange(0.15f, 0.4f);
				Resonance.OriginIncarnation = 0;
				Resonance.LastReinforcedIncarnation = INDEX_NONE;
				Pool.RemoveAtSwap(Pick);
			}
		}

		return Soul;
	}

	FGenesisIncarnationRecord* BeginIncarnation(FGenesisSoulSeed& Soul, const FGenesisIncarnationRecord& Template)
	{
		if (!Soul.IsValid() || Soul.IsIncarnated())
		{
			return nullptr;
		}

		FGenesisIncarnationRecord& Record = Soul.Incarnations.Add_GetRef(Template);
		Record.Index = Soul.Incarnations.Num() - 1;
		Record.bCompleted = false;
		return &Record;
	}

	void ReinforceResonance(FGenesisSoulSeed& Soul, const FGameplayTag& Pattern, float Amount, const FGenesisSoulCarryOverParams& Params)
	{
		if (!Pattern.IsValid() || Amount <= 0.0f)
		{
			return;
		}

		const int32 CurrentIncarnation = FMath::Max(0, Soul.GetCurrentIncarnationIndex());
		const float Gain = FMath::Clamp(Amount, 0.0f, 1.0f) * Params.ResonanceReinforcementGain;

		for (FGenesisSoulResonance& Resonance : Soul.Resonances)
		{
			if (Resonance.Pattern == Pattern)
			{
				Resonance.Intensity = SaturateTowardsOne(Resonance.Intensity, Gain);
				Resonance.LastReinforcedIncarnation = CurrentIncarnation;
				++Resonance.ReinforcementCount;
				return;
			}
		}

		FGenesisSoulResonance& NewResonance = Soul.Resonances.AddDefaulted_GetRef();
		NewResonance.Pattern = Pattern;
		NewResonance.Intensity = Gain;
		NewResonance.OriginIncarnation = CurrentIncarnation;
		NewResonance.LastReinforcedIncarnation = CurrentIncarnation;
		NewResonance.ReinforcementCount = 1;
	}

	bool CloseIncarnation(FGenesisSoulSeed& Soul, const FGenesisLifeClosure& Closure, const FGenesisSoulCarryOverParams& Params)
	{
		if (!Soul.IsIncarnated())
		{
			return false;
		}

		FGenesisIncarnationRecord& Current = Soul.Incarnations.Last();
		Current.DeathTime = Closure.DeathTime;
		Current.DeathKind = Closure.DeathKind;
		Current.AfterlifeRealm = Closure.AfterlifeRealm;
		Current.bCompleted = true;

		const int32 LifeIndex = Current.Index;
		FGenesisRandomStream Rng = FGenesisRandomStream(Soul.OriginSeed).Derive(GenesisHash::Combine(ClosureSalt, static_cast<uint64>(LifeIndex)));

		// --- 1) Resonanzen: Erlebtes verstärkt, Nicht-Erlebtes verblasst ---
		for (const FGenesisWeightedTag& Experienced : Closure.ExperiencedPatterns)
		{
			ReinforceResonance(Soul, Experienced.Tag, Experienced.Weight, Params);
		}

		for (FGenesisSoulResonance& Resonance : Soul.Resonances)
		{
			if (Resonance.LastReinforcedIncarnation != LifeIndex)
			{
				Resonance.Intensity *= Params.ResonancePersistence;
			}
		}
		Soul.Resonances.RemoveAll([&Params](const FGenesisSoulResonance& Resonance)
		{
			return Resonance.Intensity < Params.ResonanceRemovalThreshold;
		});
		Soul.Resonances.StableSort([](const FGenesisSoulResonance& A, const FGenesisSoulResonance& B)
		{
			return A.Intensity > B.Intensity;
		});
		if (Soul.Resonances.Num() > Params.MaxResonances)
		{
			Soul.Resonances.SetNum(Params.MaxResonances);
		}

		// --- 2) Karma-Gericht: Themen werden zu Echos (Fundament oder Narbe) ---
		for (const FGenesisLifeClosureTheme& CourtTheme : Closure.CourtThemes)
		{
			if (!CourtTheme.Theme.IsValid())
			{
				continue;
			}

			FGenesisSoulEcho* Echo = Soul.Echoes.FindByPredicate([&CourtTheme](const FGenesisSoulEcho& Existing)
			{
				return Existing.Theme == CourtTheme.Theme;
			});

			if (Echo)
			{
				// Das Thema kehrte in diesem Leben wieder
				++Echo->ManifestationCount;
				Echo->Weight = SaturateTowardsOne(Echo->Weight, CourtTheme.Intensity * Params.EchoGain);
				Echo->State = CourtTheme.bResolved ? EGenesisEchoState::Integrated : EGenesisEchoState::Open;
			}
			else
			{
				Echo = &Soul.Echoes.AddDefaulted_GetRef();
				Echo->EchoId = Rng.NewGuid();
				Echo->Theme = CourtTheme.Theme;
				Echo->Weight = FMath::Clamp(CourtTheme.Intensity, 0.0f, 1.0f);
				Echo->State = CourtTheme.bResolved ? EGenesisEchoState::Integrated : EGenesisEchoState::Open;
				Echo->SourceIncarnation = LifeIndex;
			}

			Echo->SourceEventId = CourtTheme.SourceEventId;
			Echo->LastTouchedIncarnation = LifeIndex;
			for (const FGuid& Related : CourtTheme.RelatedSoulIds)
			{
				Echo->RelatedSoulIds.AddUnique(Related);
			}
		}

		for (FGenesisSoulEcho& Echo : Soul.Echoes)
		{
			if (Echo.LastTouchedIncarnation != LifeIndex)
			{
				Echo.Weight *= Echo.State == EGenesisEchoState::Open ? Params.OpenEchoPersistence : Params.IntegratedEchoPersistence;
			}
		}
		Soul.Echoes.RemoveAll([&Params](const FGenesisSoulEcho& Echo)
		{
			return Echo.Weight < Params.EchoRemovalThreshold;
		});
		Soul.Echoes.StableSort([](const FGenesisSoulEcho& A, const FGenesisSoulEcho& B)
		{
			return A.Weight > B.Weight;
		});
		if (Soul.Echoes.Num() > Params.MaxEchoes)
		{
			Soul.Echoes.SetNum(Params.MaxEchoes);
		}

		// --- 3) Seelenbindungen ---
		for (const FGenesisBondExperience& Experience : Closure.Bonds)
		{
			if (!Experience.OtherSoulId.IsValid() || Experience.OtherSoulId == Soul.SoulId)
			{
				continue;
			}

			FGenesisSoulBond* Bond = Soul.Bonds.FindByPredicate([&Experience](const FGenesisSoulBond& Existing)
			{
				return Existing.OtherSoulId == Experience.OtherSoulId;
			});

			if (!Bond)
			{
				Bond = &Soul.Bonds.AddDefaulted_GetRef();
				Bond->OtherSoulId = Experience.OtherSoulId;
				Bond->FirstIncarnation = LifeIndex;
			}

			Bond->Strength = SaturateTowardsOne(Bond->Strength, Experience.Intensity * Params.BondGain);
			Bond->SharedThemes.AppendTags(Experience.Themes);
			Bond->LastIncarnation = LifeIndex;
			++Bond->SharedLifetimes;
		}

		for (FGenesisSoulBond& Bond : Soul.Bonds)
		{
			if (Bond.LastIncarnation != LifeIndex)
			{
				Bond.Strength *= Params.BondPersistence;
			}
		}
		Soul.Bonds.RemoveAll([&Params](const FGenesisSoulBond& Bond)
		{
			return Bond.Strength < Params.BondRemovalThreshold;
		});
		Soul.Bonds.StableSort([](const FGenesisSoulBond& A, const FGenesisSoulBond& B)
		{
			return A.Strength > B.Strength;
		});
		if (Soul.Bonds.Num() > Params.MaxBonds)
		{
			Soul.Bonds.SetNum(Params.MaxBonds);
		}

		// --- 4) Motiv entwickelt sich ---
		EvolveMotif(Soul, Closure, Rng);

		// --- 5) Loslassen ---
		Soul.Detachment = SaturateTowardsOne(Soul.Detachment, Closure.LettingGo * Params.DetachmentGain);

		return true;
	}

	float GetResonanceIntensity(const FGenesisSoulSeed& Soul, const FGameplayTag& PatternQuery)
	{
		float Highest = 0.0f;
		for (const FGenesisSoulResonance& Resonance : Soul.Resonances)
		{
			if (Resonance.Pattern.MatchesTag(PatternQuery))
			{
				Highest = FMath::Max(Highest, Resonance.Intensity);
			}
		}
		return Highest;
	}

	float ComputeEchoPull(const FGenesisSoulSeed& Soul, const FGameplayTagContainer& SituationThemes)
	{
		// Wahrscheinlichkeits-ODER: mehrere passende Echos verstärken sich, ohne 1 zu überschreiten
		float Remaining = 1.0f;
		for (const FGenesisSoulEcho& Echo : Soul.Echoes)
		{
			if (SituationThemes.HasTag(Echo.Theme))
			{
				const float StateFactor = Echo.State == EGenesisEchoState::Open ? 1.0f : 0.4f;
				Remaining *= 1.0f - FMath::Clamp(Echo.Weight * StateFactor, 0.0f, 1.0f);
			}
		}
		return 1.0f - Remaining;
	}

	float ComputeRecognition(const FGenesisSoulSeed& Soul, const FGuid& OtherSoulId, const FGameplayTagContainer& ContextThemes)
	{
		float Recognition = 0.0f;

		if (const FGenesisSoulBond* Bond = FindBond(Soul, OtherSoulId))
		{
			float ThemeFactor = 0.6f;
			if (ContextThemes.Num() > 0 && Bond->SharedThemes.Num() > 0)
			{
				int32 Matching = 0;
				for (const FGameplayTag& Tag : ContextThemes)
				{
					if (Bond->SharedThemes.HasTag(Tag))
					{
						++Matching;
					}
				}
				ThemeFactor += 0.4f * static_cast<float>(Matching) / static_cast<float>(ContextThemes.Num());
			}
			Recognition = Bond->Strength * ThemeFactor;
		}

		// Gemeinsame offene Themen verstärken das Gefühl von Vertrautheit
		for (const FGenesisSoulEcho& Echo : Soul.Echoes)
		{
			if (Echo.RelatedSoulIds.Contains(OtherSoulId))
			{
				Recognition += 0.1f * Echo.Weight;
			}
		}

		return FMath::Clamp(Recognition, 0.0f, 1.0f);
	}

	const FGenesisSoulEcho* FindEcho(const FGenesisSoulSeed& Soul, const FGameplayTag& Theme)
	{
		return Soul.Echoes.FindByPredicate([&Theme](const FGenesisSoulEcho& Echo) { return Echo.Theme == Theme; });
	}

	const FGenesisSoulBond* FindBond(const FGenesisSoulSeed& Soul, const FGuid& OtherSoulId)
	{
		return Soul.Bonds.FindByPredicate([&OtherSoulId](const FGenesisSoulBond& Bond) { return Bond.OtherSoulId == OtherSoulId; });
	}
}
