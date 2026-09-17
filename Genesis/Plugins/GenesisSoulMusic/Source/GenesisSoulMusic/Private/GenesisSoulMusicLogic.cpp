// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoulMusicLogic.h"
#include "GenesisGameplayTags.h"
#include "GenesisMemoryTrace.h"
#include "GenesisRandom.h"

namespace GenesisSoulMusicLogic
{
	namespace
	{
		constexpr int32 MinInterval = -7;
		constexpr int32 MaxInterval = 7;
		constexpr int32 DefaultDurationSixteenths = 4;
		constexpr int32 LowestMidiNote = 21;
		constexpr int32 HighestMidiNote = 108;

		/** Tonvorrat der sieben Modi in Halbtönen über der Tonika. */
		constexpr int32 ModeSteps[7][7] = {
			{ 0, 2, 4, 5, 7, 9, 11 }, // Ionisch
			{ 0, 2, 3, 5, 7, 9, 10 }, // Dorisch
			{ 0, 1, 3, 5, 7, 8, 10 }, // Phrygisch
			{ 0, 2, 4, 6, 7, 9, 11 }, // Lydisch
			{ 0, 2, 4, 5, 7, 9, 10 }, // Mixolydisch
			{ 0, 2, 3, 5, 7, 8, 10 }, // Äolisch
			{ 0, 1, 3, 5, 6, 8, 10 }  // Lokrisch
		};

		constexpr int32 AeolianMode = 5;
		constexpr int32 IonianMode = 0;

		bool IsBrightMode(int32 ModeIndex) { return ModeIndex == 0 || ModeIndex == 3 || ModeIndex == 4; }
		bool IsDarkMode(int32 ModeIndex) { return ModeIndex == 2 || ModeIndex == 5 || ModeIndex == 6; }

		FGenesisInstrumentLayer Layer(EGenesisInstrument Instrument, float Gain, int32 OctaveOffset)
		{
			FGenesisInstrumentLayer Result;
			Result.Instrument = Instrument;
			Result.Gain = Gain;
			Result.OctaveOffset = OctaveOffset;
			return Result;
		}

		FGenesisPhaseArrangement MakeArrangement(const FGameplayTag& Phase, TArray<FGenesisInstrumentLayer>&& Layers, float TempoBpm, float NoteDensity, int32 Tonic, float Presence, float Space)
		{
			FGenesisPhaseArrangement Result;
			Result.LifePhase = Phase;
			Result.Layers = MoveTemp(Layers);
			Result.TempoBpm = TempoBpm;
			Result.NoteDensity = NoteDensity;
			Result.TonicMidiNote = Tonic;
			Result.Presence = Presence;
			Result.Space = Space;
			return Result;
		}

		int32 NoteCountOf(const FGenesisSoulMotif& Motif)
		{
			return FMath::Max(Motif.Durations.Num(), Motif.Intervals.Num() > 0 ? Motif.Intervals.Num() + 1 : 0);
		}

		int32 DurationAt(const FGenesisSoulMotif& Motif, int32 NoteIndex)
		{
			return Motif.Durations.Num() > 0 ? FMath::Max(1, Motif.Durations[NoteIndex % Motif.Durations.Num()]) : DefaultDurationSixteenths;
		}

		int32 ClampInterval(int32 Value, int32 SignIfZero)
		{
			const int32 Clamped = FMath::Clamp(Value, MinInterval, MaxInterval);
			return Clamped != 0 ? Clamped : (SignIfZero >= 0 ? 1 : -1);
		}

		float IntervalSimilarity(const TArray<int32>& A, const TArray<int32>& B)
		{
			const int32 Longest = FMath::Max(A.Num(), B.Num());
			if (Longest == 0)
			{
				return 1.0f;
			}
			float Sum = 0.0f;
			const int32 Shared = FMath::Min(A.Num(), B.Num());
			for (int32 Index = 0; Index < Shared; ++Index)
			{
				const float Distance = static_cast<float>(FMath::Min(FMath::Abs(A[Index] - B[Index]), MaxInterval)) / static_cast<float>(MaxInterval);
				const float ContourPenalty = FMath::Sign(A[Index]) != FMath::Sign(B[Index]) ? 0.4f : 0.0f;
				Sum += FMath::Max(0.0f, 1.0f - 0.6f * Distance - ContourPenalty);
			}
			return Sum / static_cast<float>(Longest);
		}

		float RhythmSimilarity(const TArray<int32>& A, const TArray<int32>& B)
		{
			const int32 Longest = FMath::Max(A.Num(), B.Num());
			if (Longest == 0)
			{
				return 1.0f;
			}
			float Sum = 0.0f;
			const int32 Shared = FMath::Min(A.Num(), B.Num());
			for (int32 Index = 0; Index < Shared; ++Index)
			{
				Sum += 1.0f - static_cast<float>(FMath::Min(FMath::Abs(A[Index] - B[Index]), 8)) / 8.0f;
			}
			return Sum / static_cast<float>(Longest);
		}

		bool IsInMode(int32 MidiPitch, int32 TonicMidiNote, int32 ModeIndex)
		{
			const int32 PitchClass = ((MidiPitch - TonicMidiNote) % 12 + 12) % 12;
			for (int32 Step : ModeSteps[FMath::Clamp(ModeIndex, 0, 6)])
			{
				if (Step == PitchClass)
				{
					return true;
				}
			}
			return false;
		}

		/** Nächster Ton des Modus oberhalb (Direction > 0) bzw. unterhalb von MidiPitch. */
		int32 StepInMode(int32 MidiPitch, int32 TonicMidiNote, int32 ModeIndex, int32 Direction)
		{
			const int32 Step = Direction >= 0 ? 1 : -1;
			int32 Candidate = MidiPitch + Step;
			while (!IsInMode(Candidate, TonicMidiNote, ModeIndex))
			{
				Candidate += Step;
			}
			return Candidate;
		}

		FGuid GuidFromHash(uint64 Hash)
		{
			const uint64 Second = GenesisHash::Mix64(Hash);
			return FGuid(static_cast<uint32>(Hash >> 32), static_cast<uint32>(Hash), static_cast<uint32>(Second >> 32), static_cast<uint32>(Second));
		}
	}

	FGenesisPhaseArrangement GetDefaultArrangement(const FGameplayTag& LifePhase)
	{
		using EI = EGenesisInstrument;

		// Spieluhr → Klavier/Gitarre → Streicher → reduziertes Klavier → Orchester → Chor + kosmisch → kaum hörbare Spieluhr
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Conception))
		{
			return MakeArrangement(LifePhase, { Layer(EI::MusicBox, 0.6f, 1) }, 56.0f, 0.5f, 72, 0.08f, 0.9f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Embryo))
		{
			return MakeArrangement(LifePhase, { Layer(EI::MusicBox, 0.7f, 1) }, 60.0f, 0.6f, 72, 0.15f, 0.85f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Birth))
		{
			return MakeArrangement(LifePhase, { Layer(EI::MusicBox, 0.6f, 1), Layer(EI::Piano, 0.5f, 0) }, 64.0f, 1.0f, 72, 0.35f, 0.6f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Childhood))
		{
			return MakeArrangement(LifePhase, { Layer(EI::MusicBox, 0.7f, 1), Layer(EI::Piano, 0.6f, 0) }, 84.0f, 1.0f, 72, 0.5f, 0.45f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Youth))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Piano, 0.8f, 0), Layer(EI::Guitar, 0.7f, -1) }, 96.0f, 1.0f, 64, 0.6f, 0.35f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Elder))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Piano, 0.7f, 0) }, 58.0f, 0.6f, 60, 0.45f, 0.55f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Death))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Orchestra, 1.0f, -1), Layer(EI::Strings, 0.7f, 0), Layer(EI::Choir, 0.3f, 0) }, 52.0f, 1.0f, 55, 0.9f, 0.7f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Ghost))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Choir, 0.6f, 0), Layer(EI::CosmicPad, 0.5f, -1) }, 46.0f, 0.7f, 67, 0.4f, 1.0f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Afterlife))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Choir, 0.8f, 0), Layer(EI::CosmicPad, 0.8f, -1) }, 44.0f, 1.0f, 64, 0.6f, 1.0f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Rebirth))
		{
			return MakeArrangement(LifePhase, { Layer(EI::MusicBox, 0.4f, 1) }, 56.0f, 0.5f, 72, 0.05f, 0.9f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_CosmicConsciousness))
		{
			return MakeArrangement(LifePhase, { Layer(EI::CosmicPad, 1.0f, -1), Layer(EI::Choir, 0.5f, 0) }, 40.0f, 0.8f, 60, 0.5f, 1.0f);
		}
		if (LifePhase.MatchesTagExact(GenesisTags::LifePhase_Creation))
		{
			return MakeArrangement(LifePhase, { Layer(EI::Orchestra, 0.7f, -1), Layer(EI::Choir, 0.7f, 0), Layer(EI::CosmicPad, 0.6f, -1) }, 60.0f, 1.0f, 60, 0.8f, 0.9f);
		}

		// Erwachsenenleben und unbekannte Phasen
		return MakeArrangement(LifePhase, { Layer(EI::Strings, 0.8f, 0), Layer(EI::Piano, 0.5f, 0) }, 80.0f, 1.0f, 60, 0.65f, 0.45f);
	}

	FGenesisPhaseArrangement ResolveArrangement(const FGameplayTag& LifePhase, const FGenesisSoulMusicTuning& Tuning)
	{
		const FGenesisPhaseArrangement* Override = Tuning.ArrangementOverrides.FindByPredicate([&LifePhase](const FGenesisPhaseArrangement& Candidate)
		{
			return Candidate.LifePhase.IsValid() && Candidate.LifePhase.MatchesTagExact(LifePhase);
		});
		return Override ? *Override : GetDefaultArrangement(LifePhase);
	}

	int32 QuantizeToMode(int32 MidiPitch, int32 TonicMidiNote, int32 ModeIndex)
	{
		const int32 Mode = FMath::Clamp(ModeIndex, 0, 6);
		const int32 PitchClass = ((MidiPitch - TonicMidiNote) % 12 + 12) % 12;

		int32 BestOffset = 0;
		int32 BestDistance = MAX_int32;
		for (int32 Step : ModeSteps[Mode])
		{
			for (int32 Wrap = -12; Wrap <= 12; Wrap += 12)
			{
				const int32 Candidate = Step + Wrap;
				const int32 Distance = FMath::Abs(Candidate - PitchClass);
				// Gleichstand: tiefere Alternative
				if (Distance < BestDistance || (Distance == BestDistance && Candidate < BestOffset))
				{
					BestDistance = Distance;
					BestOffset = Candidate;
				}
			}
		}
		return MidiPitch - PitchClass + BestOffset;
	}

	TArray<int32> ComputePitches(const FGenesisSoulMotif& Motif, int32 TonicMidiNote)
	{
		const int32 NoteCount = NoteCountOf(Motif);
		TArray<int32> Pitches;
		Pitches.Reserve(NoteCount);

		// Rohe Tonhöhe läuft ungerastet weiter, damit sich Rundungen nicht aufsummieren
		int32 RawPitch = TonicMidiNote;
		for (int32 Index = 0; Index < NoteCount; ++Index)
		{
			const int32 Interval = Index > 0 && Motif.Intervals.IsValidIndex(Index - 1) ? Motif.Intervals[Index - 1] : 0;
			RawPitch += Interval;
			int32 Pitch = QuantizeToMode(RawPitch, TonicMidiNote, Motif.ModeIndex);

			// Die Kontur ist die Identität des Motivs: Ein Halbtonschritt darf durch das Einrasten weder verschwinden
			// noch die Richtung wechseln – dann zum nächsten Modus-Ton in Schrittrichtung
			if (Index > 0 && Interval != 0 && FMath::Sign(Pitch - Pitches.Last()) != FMath::Sign(Interval))
			{
				Pitch = StepInMode(Pitches.Last(), TonicMidiNote, Motif.ModeIndex, FMath::Sign(Interval));
			}
			Pitches.Add(Pitch);
		}
		return Pitches;
	}

	FGenesisMusicPhrase RenderPhrase(const FGenesisSoulMotif& Motif, const FGenesisPhaseArrangement& Arrangement)
	{
		FGenesisMusicPhrase Phrase;
		Phrase.TempoBpm = Arrangement.TempoBpm;
		Phrase.Presence = Arrangement.Presence;
		Phrase.Space = Arrangement.Space;

		const TArray<int32> Pitches = ComputePitches(Motif, Arrangement.TonicMidiNote);
		const int32 NoteCount = Pitches.Num();
		if (NoteCount == 0)
		{
			return Phrase;
		}

		// Welche Noten erklingen: Identitätskern und Schlussnote immer, dazwischen gleichmäßig verteilt
		const int32 CoreCount = FMath::Min(NoteCount, FGenesisSoulMotif::IdentityCoreLength + 1);
		const int32 KeepCount = FMath::Max(FMath::Clamp(FMath::RoundToInt(static_cast<float>(NoteCount) * Arrangement.NoteDensity), 1, NoteCount), FMath::Min(NoteCount, CoreCount + 1));

		TArray<bool> Kept;
		Kept.Init(false, NoteCount);
		for (int32 Index = 0; Index < CoreCount; ++Index)
		{
			Kept[Index] = true;
		}
		Kept[NoteCount - 1] = true;

		TArray<int32> Middle;
		for (int32 Index = CoreCount; Index < NoteCount - 1; ++Index)
		{
			Middle.Add(Index);
		}
		int32 AlreadyKept = 0;
		for (bool bKept : Kept)
		{
			AlreadyKept += bKept ? 1 : 0;
		}
		const int32 Remaining = FMath::Min(KeepCount - AlreadyKept, Middle.Num());
		for (int32 Slot = 0; Slot < Remaining; ++Slot)
		{
			const int32 MiddleIndex = FMath::Clamp(FMath::FloorToInt((static_cast<float>(Slot) + 0.5f) * static_cast<float>(Middle.Num()) / static_cast<float>(Remaining)), 0, Middle.Num() - 1);
			Kept[Middle[MiddleIndex]] = true;
		}

		const float BaseVelocity = 0.5f + 0.3f * FMath::Clamp(Motif.Warmth, 0.0f, 1.0f);
		float Beat = 0.0f;
		for (int32 Index = 0; Index < NoteCount; )
		{
			// Ausgelassene Noten klingen in der vorherigen weiter – die Phrase atmet langsamer, bleibt aber gleich lang
			int32 Sixteenths = DurationAt(Motif, Index);
			int32 Next = Index + 1;
			while (Next < NoteCount && !Kept[Next])
			{
				Sixteenths += DurationAt(Motif, Next);
				++Next;
			}
			const float DurationBeats = static_cast<float>(Sixteenths) / 4.0f;

			for (const FGenesisInstrumentLayer& InstrumentLayer : Arrangement.Layers)
			{
				FGenesisMusicNote& Note = Phrase.Notes.AddDefaulted_GetRef();
				Note.MidiPitch = FMath::Clamp(Pitches[Index] + 12 * InstrumentLayer.OctaveOffset, LowestMidiNote, HighestMidiNote);
				Note.StartBeat = Beat;
				Note.DurationBeats = DurationBeats;
				Note.Velocity = FMath::Clamp(BaseVelocity + (Index == 0 ? 0.1f : 0.0f), 0.0f, 1.0f) * InstrumentLayer.Gain;
				Note.Instrument = InstrumentLayer.Instrument;
			}

			Beat += DurationBeats;
			Index = Next;
		}
		Phrase.LengthBeats = Beat;
		return Phrase;
	}

	float MotifSimilarity(const FGenesisSoulMotif& A, const FGenesisSoulMotif& B)
	{
		return 0.6f * IntervalSimilarity(A.Intervals, B.Intervals)
			+ 0.3f * RhythmSimilarity(A.Durations, B.Durations)
			+ 0.1f * (A.ModeIndex == B.ModeIndex ? 1.0f : 0.0f);
	}

	FGenesisSoulMotif InheritMotif(const FGenesisSoulMotif& ParentA, const FGenesisSoulMotif& ParentB, uint64 Seed, const FGenesisSoulMusicTuning& Tuning)
	{
		FGenesisRandomStream Rng(Seed);

		const bool bAFirst = Rng.Bernoulli(0.5f);
		const FGenesisSoulMotif& FirstParent = bAFirst ? ParentA : ParentB;
		const FGenesisSoulMotif& SecondParent = bAFirst ? ParentB : ParentA;
		const FGenesisSoulMotif& RhythmParent = Rng.Bernoulli(0.5f) ? ParentA : ParentB;

		FGenesisSoulMotif Child;
		const int32 NoteCount = FMath::Max(2, NoteCountOf(RhythmParent));
		const int32 IntervalCount = NoteCount - 1;
		const int32 Crossover = IntervalCount >= 2 ? Rng.RandRange(1, IntervalCount - 1) : IntervalCount;

		// Melodie: zusammenhängende Abschnitte von beiden Eltern, damit Phrasen erkennbar bleiben
		for (int32 Index = 0; Index < IntervalCount; ++Index)
		{
			const FGenesisSoulMotif& Source = Index < Crossover ? FirstParent : SecondParent;
			const FGenesisSoulMotif& Fallback = Index < Crossover ? SecondParent : FirstParent;

			int32 Interval;
			if (Source.Intervals.IsValidIndex(Index))
			{
				Interval = Source.Intervals[Index];
			}
			else if (Fallback.Intervals.IsValidIndex(Index))
			{
				Interval = Fallback.Intervals[Index];
			}
			else
			{
				const int32 Step = Rng.RandRange(1, 2);
				Interval = Rng.Bernoulli(0.5f) ? Step : -Step;
			}

			if (Rng.Bernoulli(Tuning.InheritanceMutationChance))
			{
				const int32 Delta = Rng.Bernoulli(0.5f) ? 1 : -1;
				Interval = ClampInterval(Interval + Delta, Delta);
			}
			Child.Intervals.Add(ClampInterval(Interval, 1));
		}

		Child.Durations.Reserve(NoteCount);
		for (int32 Index = 0; Index < NoteCount; ++Index)
		{
			Child.Durations.Add(DurationAt(RhythmParent, Index));
		}

		Child.ModeIndex = Rng.Bernoulli(0.5f) ? ParentA.ModeIndex : ParentB.ModeIndex;
		Child.Tension = 0.5f * (ParentA.Tension + ParentB.Tension);
		Child.Warmth = 0.5f * (ParentA.Warmth + ParentB.Warmth);
		Child.Variation = 0;
		return Child;
	}

	FGenesisSoulMotif FuseMotifs(const FGenesisSoulMotif& Own, const FGenesisSoulMotif& Other, float Fusion)
	{
		// Jede Seite bewegt sich höchstens bis zur Mitte – bei voller Verschmelzung klingen beide Perspektiven gleich
		const float Alpha = 0.5f * FMath::Clamp(Fusion, 0.0f, 1.0f);
		if (Alpha <= 0.0f)
		{
			return Own;
		}

		const int32 OwnCount = Own.Intervals.Num();
		const int32 OtherCount = Other.Intervals.Num();
		const int32 IntervalCount = FMath::RoundToInt(FMath::Lerp(static_cast<float>(OwnCount), static_cast<float>(OtherCount), Alpha));

		FGenesisSoulMotif Result;
		Result.Intervals.Reserve(IntervalCount);
		for (int32 Index = 0; Index < IntervalCount; ++Index)
		{
			const int32 A = Own.Intervals.IsValidIndex(Index) ? Own.Intervals[Index] : Other.Intervals[Index];
			const int32 B = Other.Intervals.IsValidIndex(Index) ? Other.Intervals[Index] : Own.Intervals[Index];
			const int32 Blended = FMath::RoundToInt(FMath::Lerp(static_cast<float>(A), static_cast<float>(B), Alpha));
			Result.Intervals.Add(ClampInterval(Blended, A + B));
		}

		const int32 NoteCount = IntervalCount + 1;
		Result.Durations.Reserve(NoteCount);
		for (int32 Index = 0; Index < NoteCount; ++Index)
		{
			const float A = static_cast<float>(DurationAt(Own, Index));
			const float B = static_cast<float>(DurationAt(Other, Index));
			Result.Durations.Add(FMath::Max(1, FMath::RoundToInt(FMath::Lerp(A, B, Alpha))));
		}

		// Modus: bis zur vollen Verschmelzung der eigene, danach ein gemeinsamer (der wärmere, bei Gleichstand der niedrigere Index)
		if (Alpha >= 0.5f - UE_KINDA_SMALL_NUMBER)
		{
			if (!FMath::IsNearlyEqual(Own.Warmth, Other.Warmth))
			{
				Result.ModeIndex = Own.Warmth > Other.Warmth ? Own.ModeIndex : Other.ModeIndex;
			}
			else
			{
				Result.ModeIndex = FMath::Min(Own.ModeIndex, Other.ModeIndex);
			}
		}
		else
		{
			Result.ModeIndex = Own.ModeIndex;
		}

		Result.Tension = FMath::Lerp(Own.Tension, Other.Tension, Alpha);
		Result.Warmth = FMath::Lerp(Own.Warmth, Other.Warmth, Alpha);
		Result.Variation = Own.Variation;
		return Result;
	}

	float ComputeScar(float FusionAtSeparation, const FGenesisSoulMusicTuning& Tuning)
	{
		return FMath::Clamp(FusionAtSeparation, 0.0f, 1.0f) * FMath::Clamp(Tuning.ScarRetention, 0.0f, 1.0f);
	}

	FGenesisSoulMotif SeparateMotif(const FGenesisSoulMotif& Own, const FGenesisSoulMotif& Other, float FusionAtSeparation, const FGenesisSoulMusicTuning& Tuning, float& OutScar)
	{
		OutScar = ComputeScar(FusionAtSeparation, Tuning);
		return FuseMotifs(Own, Other, OutScar);
	}

	bool MakeFragment(const FGenesisSoulMotif& SourceMotif, const FGuid& OwnerId, const FGenesisMemoryTrace& Trace, const FGameplayTag& LifePhase,
		const FGenesisTimestamp& Time, const FGenesisSoulMusicTuning& Tuning, FGenesisMemoryMusicFragment& OutFragment)
	{
		const int32 NoteCount = NoteCountOf(SourceMotif);
		if (Trace.Intensity < Tuning.FragmentIntensityThreshold || NoteCount == 0)
		{
			return false;
		}

		const int32 Length = FMath::Min(NoteCount, Trace.Intensity >= 0.85f ? 4 : (Trace.Intensity >= 0.7f ? 3 : 2));
		const uint64 TraceHash = GenesisHash::FromGuid(Trace.TraceId);
		const int32 Start = Trace.Intensity >= 0.85f ? 0 : static_cast<int32>(TraceHash % static_cast<uint64>(NoteCount - Length + 1));

		FGenesisSoulMotif Snapshot;
		for (int32 Index = Start; Index < Start + Length - 1; ++Index)
		{
			if (SourceMotif.Intervals.IsValidIndex(Index))
			{
				Snapshot.Intervals.Add(SourceMotif.Intervals[Index]);
			}
		}
		for (int32 Index = Start; Index < Start + Length; ++Index)
		{
			Snapshot.Durations.Add(DurationAt(SourceMotif, Index));
		}

		// Starke Gefühle färben den Klang der Erinnerung – kein Urteil, nur Farbe
		Snapshot.ModeIndex = SourceMotif.ModeIndex;
		if (Trace.Valence <= -0.5f && IsBrightMode(SourceMotif.ModeIndex))
		{
			Snapshot.ModeIndex = AeolianMode;
		}
		else if (Trace.Valence >= 0.5f && IsDarkMode(SourceMotif.ModeIndex))
		{
			Snapshot.ModeIndex = IonianMode;
		}
		Snapshot.Tension = FMath::Max(SourceMotif.Tension, FMath::Clamp(Trace.Arousal, 0.0f, 1.0f));
		Snapshot.Warmth = FMath::Lerp(SourceMotif.Warmth, 0.5f * (FMath::Clamp(Trace.Valence, -1.0f, 1.0f) + 1.0f), 0.5f);
		Snapshot.Variation = SourceMotif.Variation;

		OutFragment = FGenesisMemoryMusicFragment();
		OutFragment.FragmentId = GuidFromHash(GenesisHash::Combine(TraceHash, GenesisHash::FromGuid(OwnerId)));
		OutFragment.OwnerId = OwnerId;
		OutFragment.TraceId = Trace.TraceId;
		OutFragment.EventId = Trace.EventId;
		OutFragment.CreatedAt = Time;
		OutFragment.LifePhase = LifePhase;
		OutFragment.Snapshot = MoveTemp(Snapshot);
		OutFragment.SourceNoteIndex = Start;
		OutFragment.Valence = FMath::Clamp(Trace.Valence, -1.0f, 1.0f);
		OutFragment.Intensity = FMath::Clamp(Trace.Intensity, 0.0f, 1.0f);
		return true;
	}

	FGenesisMusicPhrase RecallFragment(const FGenesisMemoryMusicFragment& Fragment, float Accuracy, uint64 RecallSeed, const FGenesisSoulMusicTuning& Tuning)
	{
		FGenesisMusicPhrase Phrase = RenderPhrase(Fragment.Snapshot, ResolveArrangement(Fragment.LifePhase, Tuning));
		Phrase.Presence *= 0.5f + 0.5f * Fragment.Intensity;

		const float Error = 1.0f - FMath::Clamp(Accuracy, 0.0f, 1.0f);
		if (Error <= 0.0f || Phrase.Notes.Num() == 0)
		{
			return Phrase;
		}
		Phrase.Space = FMath::Min(1.0f, Phrase.Space + 0.3f * Error);

		// Noten eines Zeitpunkts (alle Instrumentenschichten) werden gemeinsam verändert
		TArray<float> Onsets;
		for (const FGenesisMusicNote& Note : Phrase.Notes)
		{
			Onsets.AddUnique(Note.StartBeat);
		}

		struct FOnsetChange
		{
			bool bDropped = false;
			int32 PitchDelta = 0;
			float Shift = 0.0f;
		};
		TArray<FOnsetChange> Changes;
		Changes.SetNum(Onsets.Num());

		FGenesisRandomStream Rng(RecallSeed);
		for (int32 OnsetIndex = 1; OnsetIndex < Onsets.Num(); ++OnsetIndex)
		{
			FOnsetChange& Change = Changes[OnsetIndex];
			Change.bDropped = Rng.Bernoulli(Error * Tuning.RecallDropRate);
			if (Rng.Bernoulli(Error * Tuning.RecallWrongNoteRate))
			{
				Change.PitchDelta = Rng.Bernoulli(0.5f) ? 1 : -1;
			}
			Change.Shift = Rng.FRandRange(-1.0f, 1.0f) * Error * Tuning.RecallTimingJitterBeats;
		}

		TArray<FGenesisMusicNote> Remembered;
		Remembered.Reserve(Phrase.Notes.Num());
		for (const FGenesisMusicNote& Note : Phrase.Notes)
		{
			const FOnsetChange& Change = Changes[Onsets.IndexOfByKey(Note.StartBeat)];
			if (Change.bDropped)
			{
				continue;
			}
			FGenesisMusicNote& Recalled = Remembered.Add_GetRef(Note);
			Recalled.MidiPitch = FMath::Clamp(Note.MidiPitch + Change.PitchDelta, LowestMidiNote, HighestMidiNote);
			Recalled.StartBeat = FMath::Max(0.0f, Note.StartBeat + Change.Shift);
		}
		Phrase.Notes = MoveTemp(Remembered);
		return Phrase;
	}

	void RecordCue(FGenesisLifeSoundtrack& Soundtrack, const FGenesisSoundtrackEntry& Entry, const FGenesisSoulMusicTuning& Tuning)
	{
		const int32 InsertAt = Soundtrack.Entries.IndexOfByPredicate([&Entry](const FGenesisSoundtrackEntry& Existing)
		{
			return Existing.Time.Seconds > Entry.Time.Seconds;
		});
		Soundtrack.Entries.Insert(Entry, InsertAt == INDEX_NONE ? Soundtrack.Entries.Num() : InsertAt);

		while (Soundtrack.Entries.Num() > FMath::Max(8, Tuning.MaxSoundtrackEntries))
		{
			int32 Weakest = INDEX_NONE;
			for (int32 Index = 0; Index < Soundtrack.Entries.Num(); ++Index)
			{
				const FGenesisSoundtrackEntry& Candidate = Soundtrack.Entries[Index];
				if (Candidate.Cue == EGenesisSoundtrackCue::PhaseChange || Candidate.Cue == EGenesisSoundtrackCue::DeathComposition)
				{
					continue;
				}
				if (Weakest == INDEX_NONE || Candidate.Importance < Soundtrack.Entries[Weakest].Importance)
				{
					Weakest = Index;
				}
			}
			if (Weakest == INDEX_NONE)
			{
				break;
			}
			Soundtrack.Entries.RemoveAt(Weakest);
		}
	}

	float FragmentSignificance(const FGenesisMemoryMusicFragment& Fragment)
	{
		return Fragment.Intensity * (0.6f + 0.4f * FMath::Abs(Fragment.Valence));
	}

	FGenesisDeathComposition ComposeDeathPiece(const FGuid& EntityId, const FGenesisSoulMotif& SoulMotif, const FGameplayTag& FirstPhase,
		const TArray<const FGenesisMemoryMusicFragment*>& Fragments, const FGenesisSoulMotif* BondMotif, const FGuid& BondPartnerId, const FGenesisSoulMusicTuning& Tuning)
	{
		FGenesisDeathComposition Composition;
		Composition.EntityId = EntityId;

		float Cursor = 0.0f;
		const float Gap = FMath::Max(0.0f, Tuning.DeathSectionGapSeconds);
		auto AddSection = [&Composition, &Cursor, Gap](EGenesisSoundtrackCue Cue, const FGuid& ReferenceId, const FGameplayTag& Phase, FGenesisMusicPhrase&& Phrase)
		{
			if (Phrase.Notes.Num() == 0)
			{
				return;
			}
			FGenesisDeathSection& Section = Composition.Sections.AddDefaulted_GetRef();
			Section.Cue = Cue;
			Section.ReferenceId = ReferenceId;
			Section.LifePhase = Phase;
			Section.StartSeconds = Cursor;
			Section.Phrase = MoveTemp(Phrase);
			Cursor += Section.Phrase.GetLengthSeconds() + Gap;
		};

		// 1. Der Anfang: das Seelenmotiv, wie es zu Beginn klang
		const FGameplayTag OpeningPhase = FirstPhase.IsValid() ? FirstPhase : FGameplayTag(GenesisTags::LifePhase_Conception);
		AddSection(EGenesisSoundtrackCue::SoulMotif, FGuid(), OpeningPhase, RenderPhrase(SoulMotif, ResolveArrangement(OpeningPhase, Tuning)));

		// 2. Prägende Erinnerungen: die bedeutsamsten, in der Reihenfolge des Lebens
		TArray<const FGenesisMemoryMusicFragment*> Selected;
		for (const FGenesisMemoryMusicFragment* Fragment : Fragments)
		{
			if (Fragment)
			{
				Selected.Add(Fragment);
			}
		}
		Selected.StableSort([](const FGenesisMemoryMusicFragment& A, const FGenesisMemoryMusicFragment& B)
		{
			const float SignificanceA = FragmentSignificance(A);
			const float SignificanceB = FragmentSignificance(B);
			return SignificanceA != SignificanceB ? SignificanceA > SignificanceB : A.CreatedAt.Seconds < B.CreatedAt.Seconds;
		});
		if (Selected.Num() > Tuning.DeathFragmentCount)
		{
			Selected.SetNum(FMath::Max(0, Tuning.DeathFragmentCount));
		}
		Selected.StableSort([](const FGenesisMemoryMusicFragment& A, const FGenesisMemoryMusicFragment& B)
		{
			return A.CreatedAt.Seconds < B.CreatedAt.Seconds;
		});
		for (const FGenesisMemoryMusicFragment* Fragment : Selected)
		{
			AddSection(EGenesisSoundtrackCue::MemoryFragment, Fragment->FragmentId, Fragment->LifePhase, RenderPhrase(Fragment->Snapshot, ResolveArrangement(Fragment->LifePhase, Tuning)));
		}

		// 3. Die wichtigste Bindung
		if (BondMotif)
		{
			const FGameplayTag BondPhase(GenesisTags::LifePhase_Adulthood);
			AddSection(EGenesisSoundtrackCue::RelationshipFusion, BondPartnerId, BondPhase, RenderPhrase(*BondMotif, ResolveArrangement(BondPhase, Tuning)));
		}

		// 4. Die Seele im Moment des Todes, 5. Übergang
		const FGameplayTag DeathPhase(GenesisTags::LifePhase_Death);
		const FGameplayTag AfterlifePhase(GenesisTags::LifePhase_Afterlife);
		AddSection(EGenesisSoundtrackCue::SoulMotif, FGuid(), DeathPhase, RenderPhrase(SoulMotif, ResolveArrangement(DeathPhase, Tuning)));
		AddSection(EGenesisSoundtrackCue::SoulMotif, FGuid(), AfterlifePhase, RenderPhrase(SoulMotif, ResolveArrangement(AfterlifePhase, Tuning)));

		Composition.TotalSeconds = Composition.Sections.Num() > 0 ? Cursor - Gap : 0.0f;
		return Composition;
	}

	FString MidiNoteName(int32 MidiPitch)
	{
		static const TCHAR* const Names[] = { TEXT("C"), TEXT("C#"), TEXT("D"), TEXT("D#"), TEXT("E"), TEXT("F"), TEXT("F#"), TEXT("G"), TEXT("G#"), TEXT("A"), TEXT("A#"), TEXT("B") };
		const int32 PitchClass = (MidiPitch % 12 + 12) % 12;
		const int32 Octave = FMath::FloorToInt(static_cast<float>(MidiPitch) / 12.0f) - 1;
		return FString::Printf(TEXT("%s%d"), Names[PitchClass], Octave);
	}
}
