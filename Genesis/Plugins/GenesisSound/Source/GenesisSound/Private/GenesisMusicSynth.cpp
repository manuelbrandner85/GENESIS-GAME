// GENESIS: Der Kreislauf des Lebens

#include "GenesisMusicSynth.h"

namespace
{
	constexpr int32 MaxVoices = 24;
	constexpr int32 MaxPartials = 8;

	/**
	 * Was ein Instrument ausmacht: welche Obertöne wie laut mitschwingen, wie schnell es anspricht
	 * und wie lange es ausklingt. Ein Klavier hat harmonische Obertöne und fällt sofort ab,
	 * eine Spieldose unharmonische (deshalb klingt sie glockig), Streicher schwellen an.
	 */
	struct FInstrumentProfile
	{
		int32 PartialCount = 4;
		float Ratios[MaxPartials] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
		float Gains[MaxPartials] = { 1.0f, 0.5f, 0.25f, 0.12f, 0.0f, 0.0f, 0.0f, 0.0f };
		float AttackSeconds = 0.01f;
		float DecaySeconds = 0.6f;
		float SustainLevel = 0.35f;
		float ReleaseSeconds = 0.5f;
		float NoiseAmount = 0.0f;
		float VibratoHz = 0.0f;
		float VibratoDepth = 0.0f;
		bool bPlucked = false;
		float Gain = 1.0f;
	};

	FInstrumentProfile GetProfile(EGenesisInstrument Instrument)
	{
		FInstrumentProfile Profile;
		switch (Instrument)
		{
		case EGenesisInstrument::MusicBox:
			// Glockig: Die Teiltöne liegen nicht auf ganzen Vielfachen, deshalb der metallische Klang
			Profile.PartialCount = 5;
			Profile.Ratios[0] = 1.0f; Profile.Ratios[1] = 2.76f; Profile.Ratios[2] = 5.40f; Profile.Ratios[3] = 8.93f; Profile.Ratios[4] = 13.3f;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.45f; Profile.Gains[2] = 0.22f; Profile.Gains[3] = 0.1f; Profile.Gains[4] = 0.05f;
			Profile.AttackSeconds = 0.002f;
			Profile.DecaySeconds = 1.6f;
			Profile.SustainLevel = 0.0f;
			Profile.ReleaseSeconds = 1.2f;
			Profile.Gain = 0.8f;
			break;

		case EGenesisInstrument::Piano:
			Profile.PartialCount = 6;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.42f; Profile.Gains[2] = 0.22f; Profile.Gains[3] = 0.12f;
			Profile.Gains[4] = 0.07f; Profile.Gains[5] = 0.04f;
			Profile.AttackSeconds = 0.004f;
			Profile.DecaySeconds = 1.3f;
			Profile.SustainLevel = 0.12f;
			Profile.ReleaseSeconds = 0.6f;
			Profile.NoiseAmount = 0.05f; // Anschlaggeräusch des Hammers
			break;

		case EGenesisInstrument::Guitar:
			// Gezupfte Saite: ein Rauschimpuls, der im Kreis läuft und mit jedem Umlauf weicher wird
			Profile.bPlucked = true;
			Profile.AttackSeconds = 0.001f;
			Profile.DecaySeconds = 2.0f;
			Profile.SustainLevel = 0.0f;
			Profile.ReleaseSeconds = 0.8f;
			Profile.Gain = 0.9f;
			break;

		case EGenesisInstrument::Strings:
			Profile.PartialCount = 7;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.6f; Profile.Gains[2] = 0.42f; Profile.Gains[3] = 0.3f;
			Profile.Gains[4] = 0.2f; Profile.Gains[5] = 0.13f; Profile.Gains[6] = 0.08f;
			Profile.AttackSeconds = 0.28f; // Streicher schwellen an, sie schlagen nicht an
			Profile.DecaySeconds = 0.5f;
			Profile.SustainLevel = 0.75f;
			Profile.ReleaseSeconds = 0.9f;
			Profile.VibratoHz = 5.2f;
			Profile.VibratoDepth = 0.004f;
			Profile.Gain = 0.75f;
			break;

		case EGenesisInstrument::Woodwinds:
			// Ungerade Obertöne wie bei einer gedackten Pfeife, dazu Atemrauschen
			Profile.PartialCount = 4;
			Profile.Ratios[0] = 1.0f; Profile.Ratios[1] = 3.0f; Profile.Ratios[2] = 5.0f; Profile.Ratios[3] = 7.0f;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.3f; Profile.Gains[2] = 0.12f; Profile.Gains[3] = 0.05f;
			Profile.AttackSeconds = 0.08f;
			Profile.DecaySeconds = 0.3f;
			Profile.SustainLevel = 0.8f;
			Profile.ReleaseSeconds = 0.35f;
			Profile.NoiseAmount = 0.12f;
			Profile.VibratoHz = 4.6f;
			Profile.VibratoDepth = 0.003f;
			Profile.Gain = 0.7f;
			break;

		case EGenesisInstrument::Orchestra:
			Profile.PartialCount = 8;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.7f; Profile.Gains[2] = 0.5f; Profile.Gains[3] = 0.38f;
			Profile.Gains[4] = 0.28f; Profile.Gains[5] = 0.2f; Profile.Gains[6] = 0.14f; Profile.Gains[7] = 0.1f;
			Profile.AttackSeconds = 0.18f;
			Profile.DecaySeconds = 0.6f;
			Profile.SustainLevel = 0.7f;
			Profile.ReleaseSeconds = 1.1f;
			Profile.VibratoHz = 4.8f;
			Profile.VibratoDepth = 0.003f;
			Profile.Gain = 0.65f;
			break;

		case EGenesisInstrument::Choir:
			// Stimmen: Grundton stark, dazu zwei Formantbereiche und ein langsames Vibrato
			Profile.PartialCount = 6;
			Profile.Ratios[0] = 1.0f; Profile.Ratios[1] = 2.0f; Profile.Ratios[2] = 3.0f;
			Profile.Ratios[3] = 4.0f; Profile.Ratios[4] = 5.0f; Profile.Ratios[5] = 6.0f;
			Profile.Gains[0] = 1.0f; Profile.Gains[1] = 0.55f; Profile.Gains[2] = 0.18f;
			Profile.Gains[3] = 0.34f; Profile.Gains[4] = 0.12f; Profile.Gains[5] = 0.06f;
			Profile.AttackSeconds = 0.35f;
			Profile.DecaySeconds = 0.4f;
			Profile.SustainLevel = 0.85f;
			Profile.ReleaseSeconds = 1.4f;
			Profile.NoiseAmount = 0.04f;
			Profile.VibratoHz = 5.0f;
			Profile.VibratoDepth = 0.006f;
			Profile.Gain = 0.7f;
			break;

		case EGenesisInstrument::CosmicPad:
		default:
			// Weit und ohne Anfang: leicht verstimmte Teiltöne, die sich langsam gegeneinander bewegen
			Profile.PartialCount = 6;
			Profile.Ratios[0] = 0.5f; Profile.Ratios[1] = 1.0f; Profile.Ratios[2] = 1.005f;
			Profile.Ratios[3] = 2.0f; Profile.Ratios[4] = 3.01f; Profile.Ratios[5] = 4.0f;
			Profile.Gains[0] = 0.5f; Profile.Gains[1] = 1.0f; Profile.Gains[2] = 0.8f;
			Profile.Gains[3] = 0.35f; Profile.Gains[4] = 0.15f; Profile.Gains[5] = 0.08f;
			Profile.AttackSeconds = 1.2f;
			Profile.DecaySeconds = 1.0f;
			Profile.SustainLevel = 0.9f;
			Profile.ReleaseSeconds = 2.5f;
			Profile.VibratoHz = 0.25f;
			Profile.VibratoDepth = 0.002f;
			Profile.Gain = 0.6f;
			break;
		}
		return Profile;
	}

	float MidiToFrequency(int32 MidiPitch)
	{
		return 440.0f * FMath::Pow(2.0f, (MidiPitch - 69) / 12.0f);
	}

	/** Weiche Sättigung: Spitzen werden gerundet statt abgeschnitten. */
	float SoftClip(float Value)
	{
		return FMath::Tanh(Value);
	}
}

FGenesisMusicSynth::FGenesisMusicSynth(uint64 Seed)
	: Rng(Seed)
{
	Voices.SetNum(MaxVoices);
}

void FGenesisMusicSynth::Initialize(float InSampleRate)
{
	SampleRate = FMath::Max(8000.0f, InSampleRate);

	// Verzögerungszeiten des Nachhalls, verhältnisgleich zur Abtastrate (Originalwerte für 44,1 kHz)
	const float Scale = SampleRate / 44100.0f;
	const int32 CombLengths[4] = { 1557, 1617, 1491, 1422 };
	const int32 AllpassLengths[2] = { 225, 556 };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		CombBuffers[Index].Init(0.0f, FMath::Max(16, FMath::RoundToInt(CombLengths[Index] * Scale)));
		CombIndices[Index] = 0;
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		AllpassBuffers[Index].Init(0.0f, FMath::Max(16, FMath::RoundToInt(AllpassLengths[Index] * Scale)));
		AllpassIndices[Index] = 0;
	}

	for (FGenesisMusicVoice& Voice : Voices)
	{
		Voice = FGenesisMusicVoice();
	}
	PlayheadSeconds = 0.0f;
}

void FGenesisMusicSynth::SetPhrase(const FGenesisMusicPhrase& Phrase, bool bInLoop)
{
	CurrentPhrase = Phrase;
	bLoop = bInLoop;
	PlayheadSeconds = 0.0f;
	NoteStarted.Init(false, Phrase.Notes.Num());
}

bool FGenesisMusicSynth::IsFinished() const
{
	if (bLoop)
	{
		return false;
	}
	for (const FGenesisMusicVoice& Voice : Voices)
	{
		if (Voice.bActive)
		{
			return false;
		}
	}
	return PlayheadSeconds >= CurrentPhrase.GetLengthSeconds();
}

int32 FGenesisMusicSynth::GetActiveVoiceCount() const
{
	int32 Count = 0;
	for (const FGenesisMusicVoice& Voice : Voices)
	{
		Count += Voice.bActive ? 1 : 0;
	}
	return Count;
}

void FGenesisMusicSynth::StartDueNotes()
{
	const float Tempo = FMath::Max(20.0f, CurrentPhrase.TempoBpm);
	const float SecondsPerBeat = 60.0f / Tempo;

	for (int32 Index = 0; Index < CurrentPhrase.Notes.Num(); ++Index)
	{
		if (NoteStarted.IsValidIndex(Index) && NoteStarted[Index])
		{
			continue;
		}

		const FGenesisMusicNote& Note = CurrentPhrase.Notes[Index];
		const float StartSeconds = Note.StartBeat * SecondsPerBeat;
		if (StartSeconds > PlayheadSeconds)
		{
			continue;
		}

		// Freie Stimme suchen; ist keine frei, wird die älteste ersetzt
		int32 Slot = INDEX_NONE;
		float OldestAge = -1.0f;
		for (int32 VoiceIndex = 0; VoiceIndex < Voices.Num(); ++VoiceIndex)
		{
			if (!Voices[VoiceIndex].bActive)
			{
				Slot = VoiceIndex;
				break;
			}
			if (Voices[VoiceIndex].AgeSeconds > OldestAge)
			{
				OldestAge = Voices[VoiceIndex].AgeSeconds;
				Slot = VoiceIndex;
			}
		}

		FGenesisMusicVoice& Voice = Voices[Slot];
		Voice = FGenesisMusicVoice();
		Voice.bActive = true;
		Voice.Frequency = MidiToFrequency(Note.MidiPitch);
		Voice.Velocity = FMath::Clamp(Note.Velocity, 0.05f, 1.0f);
		Voice.HoldSeconds = FMath::Max(0.05f, Note.DurationBeats * SecondsPerBeat);
		Voice.Instrument = Note.Instrument;
		Voice.VibratoPhase = Rng.FRandRange(0.0f, 1.0f);

		// Leicht verschobene Anfangsphasen: Sonst klingen gleichzeitige Töne wie ein einziger
		for (int32 Partial = 0; Partial < MaxPartials; ++Partial)
		{
			Voice.PartialPhases[Partial] = Rng.FRandRange(0.0f, 1.0f);
		}

		if (GetProfile(Note.Instrument).bPlucked)
		{
			const int32 Length = FMath::Clamp(FMath::RoundToInt(SampleRate / Voice.Frequency), 8, 4096);
			Voice.StringBuffer.SetNumUninitialized(Length);
			for (int32 Sample = 0; Sample < Length; ++Sample)
			{
				Voice.StringBuffer[Sample] = Rng.FRandRange(-1.0f, 1.0f);
			}
			Voice.StringIndex = 0;
			Voice.StringLast = 0.0f;
		}

		if (NoteStarted.IsValidIndex(Index))
		{
			NoteStarted[Index] = true;
		}
	}
}

float FGenesisMusicSynth::RenderVoice(FGenesisMusicVoice& Voice)
{
	const FInstrumentProfile Profile = GetProfile(Voice.Instrument);
	const float Inverse = 1.0f / SampleRate;
	Voice.AgeSeconds += Inverse;

	// Hüllkurve: anschwellen, abfallen, halten, ausklingen
	if (!Voice.bReleased && Voice.AgeSeconds >= Voice.HoldSeconds)
	{
		Voice.bReleased = true;
		Voice.ReleaseLevel = Voice.Envelope;
	}

	if (!Voice.bReleased)
	{
		if (Voice.AgeSeconds < Profile.AttackSeconds)
		{
			Voice.Envelope = Voice.AgeSeconds / FMath::Max(0.0005f, Profile.AttackSeconds);
		}
		else
		{
			const float SinceAttack = Voice.AgeSeconds - Profile.AttackSeconds;
			const float Decayed = FMath::Exp(-SinceAttack / FMath::Max(0.01f, Profile.DecaySeconds));
			Voice.Envelope = Profile.SustainLevel + (1.0f - Profile.SustainLevel) * Decayed;
		}
	}
	else
	{
		const float SinceRelease = Voice.AgeSeconds - Voice.HoldSeconds;
		Voice.Envelope = Voice.ReleaseLevel * FMath::Exp(-SinceRelease / FMath::Max(0.01f, Profile.ReleaseSeconds));
		if (Voice.Envelope < 0.0005f)
		{
			Voice.bActive = false;
			return 0.0f;
		}
	}

	// Vibrato: eine lebendige Note steht nie ganz still
	Voice.VibratoPhase += Profile.VibratoHz * Inverse;
	const float Vibrato = 1.0f + FMath::Sin(2.0f * PI * Voice.VibratoPhase) * Profile.VibratoDepth;
	const float Frequency = Voice.Frequency * Vibrato;

	float Sample = 0.0f;
	if (Profile.bPlucked && Voice.StringBuffer.Num() > 1)
	{
		// Karplus-Strong: Mittelwert zweier benachbarter Stellen – die Saite verliert mit jedem Umlauf Höhen
		const int32 Length = Voice.StringBuffer.Num();
		const int32 Next = (Voice.StringIndex + 1) % Length;
		const float Averaged = 0.5f * (Voice.StringBuffer[Voice.StringIndex] + Voice.StringBuffer[Next]);
		const float Damped = Averaged * 0.996f;
		Sample = Voice.StringBuffer[Voice.StringIndex];
		Voice.StringBuffer[Voice.StringIndex] = Damped;
		Voice.StringIndex = Next;
	}
	else
	{
		float Normalizer = 0.0f;
		for (int32 Partial = 0; Partial < Profile.PartialCount; ++Partial)
		{
			const float PartialFrequency = Frequency * Profile.Ratios[Partial];
			if (PartialFrequency > SampleRate * 0.45f)
			{
				continue; // über der halben Abtastrate entstünden Spiegelfrequenzen
			}
			Voice.PartialPhases[Partial] += PartialFrequency * Inverse;
			if (Voice.PartialPhases[Partial] >= 1.0f)
			{
				Voice.PartialPhases[Partial] -= 1.0f;
			}
			// Hohe Teiltöne klingen schneller aus als der Grundton – das macht den Klang lebendig
			const float PartialDecay = FMath::Exp(-Voice.AgeSeconds * 0.35f * Partial);
			Sample += FMath::Sin(2.0f * PI * Voice.PartialPhases[Partial]) * Profile.Gains[Partial] * PartialDecay;
			Normalizer += Profile.Gains[Partial];
		}
		Sample /= FMath::Max(0.001f, Normalizer);
	}

	if (Profile.NoiseAmount > 0.0f)
	{
		const float Breath = Voice.bReleased ? 0.3f : 1.0f;
		Sample += Rng.FRandRange(-1.0f, 1.0f) * Profile.NoiseAmount * Breath
			* (Profile.bPlucked ? 1.0f : FMath::Exp(-Voice.AgeSeconds * 12.0f) + 0.15f);
	}

	return Sample * Voice.Envelope * Voice.Velocity * Profile.Gain;
}

float FGenesisMusicSynth::RenderReverb(float Input)
{
	float Sum = 0.0f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		TArray<float>& Buffer = CombBuffers[Index];
		float& Value = Buffer[CombIndices[Index]];
		Sum += Value;
		Value = Input + Value * CombFeedback[Index];
		CombIndices[Index] = (CombIndices[Index] + 1) % Buffer.Num();
	}
	Sum *= 0.25f;

	for (int32 Index = 0; Index < 2; ++Index)
	{
		TArray<float>& Buffer = AllpassBuffers[Index];
		float& Value = Buffer[AllpassIndices[Index]];
		const float Output = -Sum + Value;
		Value = Sum + Value * 0.5f;
		AllpassIndices[Index] = (AllpassIndices[Index] + 1) % Buffer.Num();
		Sum = Output;
	}
	return Sum;
}

void FGenesisMusicSynth::Render(float* OutAudio, int32 Frames)
{
	if (!OutAudio || Frames <= 0)
	{
		return;
	}

	const float Inverse = 1.0f / SampleRate;
	// "Space" der Phrase ist der Raum, in dem sie steht; "Presence" ihre Nähe
	const float Wet = FMath::Clamp(CurrentPhrase.Space, 0.0f, 1.0f) * 0.55f;
	const float Gain = MasterGain * FMath::Lerp(0.6f, 1.25f, FMath::Clamp(CurrentPhrase.Presence, 0.0f, 1.0f));
	const float PhraseLength = CurrentPhrase.GetLengthSeconds();

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		StartDueNotes();

		float Dry = 0.0f;
		for (FGenesisMusicVoice& Voice : Voices)
		{
			if (Voice.bActive)
			{
				Dry += RenderVoice(Voice);
			}
		}

		const float Wetted = RenderReverb(Dry);
		OutAudio[Frame] = SoftClip((Dry * (1.0f - Wet) + Wetted * Wet) * Gain);

		PlayheadSeconds += Inverse;
		if (bLoop && PhraseLength > 0.0f && PlayheadSeconds >= PhraseLength)
		{
			PlayheadSeconds -= PhraseLength;
			for (bool& Started : NoteStarted)
			{
				Started = false;
			}
		}
	}
}
