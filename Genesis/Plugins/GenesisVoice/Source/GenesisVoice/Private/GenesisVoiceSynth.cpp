// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceSynth.h"
#include "GenesisVoiceLogic.h"

namespace
{
	/** Weicher Begrenzer – lauter Schrei darf verzerren, aber nicht knacken. */
	float SoftClip(float Value)
	{
		return FMath::Tanh(Value);
	}

	float SemitoneRatio(float Semitones)
	{
		return FMath::Pow(2.0f, Semitones / 12.0f);
	}
}

void FGenesisVoiceHearing::Process(float* Audio, int32 Frames, float SampleRate)
{
	if (!Audio || Frames <= 0)
	{
		return;
	}

	// Vier einpolige Tiefpässe hintereinander: 24 dB je Oktave. Gewebe und Fruchtwasser dämpfen
	// mit der Frequenz steil – im Mutterleib gemessen fehlt oberhalb von 1 kHz fast alles,
	// während die Sprachmelodie unterhalb von 500 Hz erhalten bleibt.
	const float Cutoff = FMath::Clamp(CutoffHz, 40.0f, SampleRate * 0.45f);
	const float Alpha = 1.0f - FMath::Exp(-2.0f * PI * Cutoff / FMath::Max(8000.0f, SampleRate));

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		Stage1 += (Audio[Frame] - Stage1) * Alpha;
		Stage2 += (Stage1 - Stage2) * Alpha;
		Stage3 += (Stage2 - Stage3) * Alpha;
		Stage4 += (Stage3 - Stage4) * Alpha;
		Audio[Frame] = Stage4 * Gain;
	}
}

void FGenesisVoiceSynth::FResonator::Set(float FrequencyHz, float BandwidthHz, float InSampleRate)
{
	const float Radius = FMath::Exp(-PI * BandwidthHz / InSampleRate);
	const float Theta = 2.0f * PI * FMath::Clamp(FrequencyHz, 20.0f, InSampleRate * 0.45f) / InSampleRate;
	C = -Radius * Radius;
	B = 2.0f * Radius * FMath::Cos(Theta);
	// So normiert, dass die Resonanz bei Gleichanteil Verstärkung 1 hat – sonst explodiert die Kette
	A = 1.0f - B - C;
}

float FGenesisVoiceSynth::FResonator::Process(float In)
{
	const float Out = A * In + B * Y1 + C * Y2;
	Y2 = Y1;
	Y1 = Out;
	return Out;
}

FGenesisVoiceSynth::FGenesisVoiceSynth(uint64 Seed)
	: Rng(Seed)
{
}

void FGenesisVoiceSynth::Initialize(float InSampleRate)
{
	SampleRate = FMath::Max(8000.0f, InSampleRate);
	Reset();
}

void FGenesisVoiceSynth::Reset()
{
	Segments.Reset();
	SegmentIndex = 0;
	SegmentTime = 0.0f;
	PlannedDuration = 0.0f;
	bActive = false;
	GlottalPhase = 0.0f;
	PeriodJitter = 1.0f;
	PulseAmplitude = 1.0f;
	PreviousFlow = 0.0f;
	DcBlockX1 = 0.0f;
	DcBlockY1 = 0.0f;
	NoiseLowPass = 0.0f;
	for (FResonator& Resonator : Formants)
	{
		Resonator.Reset();
	}
	NasalResonator.Reset();
}

float FGenesisVoiceSynth::GlottalFlow(float Phase, float OpenQuotient)
{
	// Rosenberg-Modell: Die Stimmritze öffnet sich langsam und schließt schnell.
	// Der steile Schluss ist der Grund, warum eine Stimme überhaupt Obertöne hat.
	const float Open = FMath::Clamp(OpenQuotient, 0.3f, 0.9f);
	const float Rise = Open * 0.62f;
	const float Fall = Open - Rise;
	if (Phase < Rise)
	{
		return 0.5f * (1.0f - FMath::Cos(PI * Phase / Rise));
	}
	if (Phase < Open)
	{
		return FMath::Cos(PI * (Phase - Rise) / (2.0f * FMath::Max(0.0001f, Fall)));
	}
	return 0.0f;
}

void FGenesisVoiceSynth::AddSyllable(EGenesisVowel Vowel, float Duration, float Gain, float PitchStart, float PitchEnd, float Voicing, float Noise, float Nasal)
{
	FSegment& Segment = Segments.AddDefaulted_GetRef();
	Segment.Vowel = Vowel;
	Segment.DurationSeconds = FMath::Max(0.01f, Duration);
	Segment.Gain = Gain;
	Segment.PitchStart = PitchStart;
	Segment.PitchEnd = PitchEnd;
	Segment.Voicing = Voicing;
	Segment.Noise = Noise;
	Segment.Nasal = Nasal;
}

void FGenesisVoiceSynth::PlanUtterance(const FGenesisUtterance& Utterance)
{
	Segments.Reset();

	const float Intensity = FMath::Clamp(Utterance.Intensity, 0.0f, 1.0f);
	const float Tempo = FMath::Max(0.3f, Profile.TempoScale);
	const float Scale = FMath::Clamp(Profile.F0RangeSemitones / 12.0f, 0.1f, 1.2f);

	auto Vary = [this](float Amount) { return 1.0f + Amount * (Rng.NextFloat() * 2.0f - 1.0f); };

	switch (Utterance.Type)
	{
	case EGenesisUtterance::Cry:
	{
		// Der Schrei ist eine Kette aus Ausatmen und hörbarem Einatmen. Genau dieses Einatmen
		// unterscheidet das Schreien eines Säuglings von jedem anderen lauten Geräusch.
		const int32 Cycles = 3 + FMath::RoundToInt(2.0f * Intensity);
		for (int32 Cycle = 0; Cycle < Cycles; ++Cycle)
		{
			const float Length = (0.75f + 0.35f * Intensity) * Vary(0.15f) / Tempo;
			const float Peak = SemitoneRatio(FMath::Lerp(2.0f, 7.0f, Intensity) * Scale * 3.0f);
			// Anstieg in den Schrei hinein, dann ein langes, fallendes Ende
			AddSyllable(EGenesisVowel::A, Length * 0.3f, 1.0f, 1.0f, Peak, 1.0f, 0.10f, 0.05f);
			AddSyllable(EGenesisVowel::A, Length * 0.7f, 1.0f, Peak, SemitoneRatio(-1.5f), 1.0f, 0.14f, 0.05f);
			// Einatmen: fast nur Luft, hoch und kurz
			AddSyllable(EGenesisVowel::I, 0.22f * Vary(0.2f), 0.35f, SemitoneRatio(6.0f), SemitoneRatio(9.0f), 0.25f, 0.9f, 0.1f);
			AddSyllable(EGenesisVowel::Schwa, 0.09f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		}
		break;
	}
	case EGenesisUtterance::Fuss:
	{
		const int32 Count = 2 + FMath::RoundToInt(2.0f * Intensity);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Length = 0.28f * Vary(0.25f) / Tempo;
			AddSyllable(EGenesisVowel::E, Length, 0.55f + 0.3f * Intensity, SemitoneRatio(1.0f), SemitoneRatio(-2.0f), 0.9f, 0.18f, 0.25f);
			AddSyllable(EGenesisVowel::Schwa, 0.12f * Vary(0.3f), 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		}
		break;
	}
	case EGenesisUtterance::Coo:
	{
		// Gurren ist ein weicher Gleitlaut, fast ohne Anstrengung – das Kind ist zufrieden
		AddSyllable(EGenesisVowel::U, 0.35f / Tempo, 0.45f, 1.0f, SemitoneRatio(2.5f * Scale * 3.0f), 0.85f, 0.25f, 0.35f);
		AddSyllable(EGenesisVowel::O, 0.45f / Tempo, 0.4f, SemitoneRatio(2.5f * Scale * 3.0f), SemitoneRatio(-2.0f), 0.8f, 0.3f, 0.3f);
		break;
	}
	case EGenesisUtterance::Babble:
	{
		// Lallen: gleiche Silbe mehrmals, mit hörbarem Verschluss dazwischen ("ba-ba-ba")
		const EGenesisVowel Vowels[] = { EGenesisVowel::A, EGenesisVowel::I, EGenesisVowel::U, EGenesisVowel::E };
		const EGenesisVowel Chosen = Vowels[Rng.NextUInt32() % UE_ARRAY_COUNT(Vowels)];
		const int32 Count = 3 + static_cast<int32>(Rng.NextUInt32() % 3);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			AddSyllable(EGenesisVowel::Schwa, 0.055f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
			const float Length = 0.2f * Vary(0.2f) / Tempo;
			const float Pitch = SemitoneRatio((Index == 0 ? 2.0f : 0.0f) * Scale * 3.0f);
			AddSyllable(Chosen, Length, 0.55f, Pitch, Pitch * SemitoneRatio(-1.0f), 1.0f, 0.12f, 0.15f);
		}
		break;
	}
	case EGenesisUtterance::Laugh:
	{
		// Lachen ist eine Kette kurzer Ausatemstöße auf fallender Tonhöhe
		const int32 Count = 4 + FMath::RoundToInt(3.0f * Intensity);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Fall = SemitoneRatio(-1.2f * Index * Scale * 3.0f);
			AddSyllable(EGenesisVowel::A, 0.085f / Tempo, 0.5f + 0.4f * Intensity, Fall * SemitoneRatio(1.0f), Fall, 0.85f, 0.35f, 0.1f);
			AddSyllable(EGenesisVowel::Schwa, 0.075f / Tempo, 0.0f, 1.0f, 1.0f, 0.0f, 0.25f, 0.0f);
		}
		break;
	}
	case EGenesisUtterance::Sigh:
	{
		AddSyllable(EGenesisVowel::Schwa, 0.5f, 0.25f, SemitoneRatio(1.0f), 1.0f, 0.45f, 0.7f, 0.15f);
		AddSyllable(EGenesisVowel::A, 0.9f, 0.3f, 1.0f, SemitoneRatio(-4.0f * Scale * 3.0f), 0.6f, 0.55f, 0.15f);
		break;
	}
	case EGenesisUtterance::Hum:
	{
		// Summen mit geschlossenem Mund: Der Ton geht durch die Nase, die Mundresonanz fehlt
		const float Notes[] = { 0.0f, 2.0f, -1.0f, -3.0f };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float Pitch = SemitoneRatio(Notes[Index] * Scale * 2.5f);
			AddSyllable(EGenesisVowel::U, 0.55f / Tempo, 0.35f, Pitch, Pitch, 1.0f, 0.05f, 0.9f);
		}
		break;
	}
	case EGenesisUtterance::Soothe:
	{
		// „Schhh" – fast reine Luft. Die Stimme ist kaum beteiligt, deshalb trägt es so weit.
		AddSyllable(EGenesisVowel::I, 1.1f / Tempo, 0.4f, 1.0f, 1.0f, 0.06f, 1.0f, 0.0f);
		AddSyllable(EGenesisVowel::Schwa, 0.18f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		AddSyllable(EGenesisVowel::I, 0.7f / Tempo, 0.3f, 1.0f, 1.0f, 0.06f, 1.0f, 0.0f);
		break;
	}
	case EGenesisUtterance::Call:
	{
		AddSyllable(EGenesisVowel::A, 0.5f / Tempo, 0.9f, SemitoneRatio(5.0f * Scale * 3.0f), SemitoneRatio(3.0f * Scale * 3.0f), 1.0f, 0.1f, 0.05f);
		AddSyllable(EGenesisVowel::O, 0.55f / Tempo, 0.8f, SemitoneRatio(3.0f * Scale * 3.0f), SemitoneRatio(-3.0f * Scale * 3.0f), 1.0f, 0.12f, 0.05f);
		break;
	}
	case EGenesisUtterance::Speak:
	default:
	{
		// Sprechen ohne Worte: Silben mit einer Sprechmelodie, die zum Satzende hin fällt.
		// Das ist ein Platzhalter für das Dialogsystem – aber ein ehrlicher: Tonhöhe, Tempo und
		// Klangfarbe stimmen, nur die Worte fehlen.
		const EGenesisVowel Vowels[] = { EGenesisVowel::A, EGenesisVowel::E, EGenesisVowel::I, EGenesisVowel::O, EGenesisVowel::U, EGenesisVowel::Schwa };
		const int32 Count = 4 + static_cast<int32>(Rng.NextUInt32() % 4);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Position = Count > 1 ? static_cast<float>(Index) / (Count - 1) : 0.0f;
			const float Declination = SemitoneRatio(FMath::Lerp(1.5f, -2.5f, Position) * Scale * 2.0f);
			const EGenesisVowel Vowel = Vowels[Rng.NextUInt32() % UE_ARRAY_COUNT(Vowels)];
			AddSyllable(EGenesisVowel::Schwa, 0.04f, 0.0f, 1.0f, 1.0f, 0.0f, 0.1f, 0.0f);
			AddSyllable(Vowel, (0.16f + 0.08f * Rng.NextFloat()) / Tempo, 0.45f + 0.35f * Intensity,
				Declination * SemitoneRatio(1.0f), Declination * SemitoneRatio(-1.0f), 1.0f, 0.12f, 0.12f);
		}
		break;
	}
	}

	PlannedDuration = 0.0f;
	for (const FSegment& Segment : Segments)
	{
		PlannedDuration += Segment.DurationSeconds;
	}

	// Vorgegebene Dauer: Der Laut wird gestreckt oder gestaucht, seine Form bleibt
	if (Utterance.DurationSeconds > 0.01f && PlannedDuration > 0.01f)
	{
		const float Stretch = Utterance.DurationSeconds / PlannedDuration;
		for (FSegment& Segment : Segments)
		{
			Segment.DurationSeconds *= Stretch;
		}
		PlannedDuration = Utterance.DurationSeconds;
	}
}

void FGenesisVoiceSynth::Begin(const FGenesisUtterance& Utterance)
{
	if (Utterance.Seed != 0)
	{
		Rng = FGenesisRandomStream(Utterance.Seed);
	}

	PlanUtterance(Utterance);
	SegmentIndex = 0;
	SegmentTime = 0.0f;
	GlottalPhase = 0.0f;
	PreviousFlow = 0.0f;
	bActive = Segments.Num() > 0;
}

void FGenesisVoiceSynth::Render(float* OutAudio, int32 Frames)
{
	if (!OutAudio || Frames <= 0)
	{
		return;
	}

	if (!bActive)
	{
		FMemory::Memzero(OutAudio, sizeof(float) * Frames);
		return;
	}

	const float Step = 1.0f / SampleRate;
	// Formanten wandern nicht sprunghaft – der Übergang von einem Vokal zum nächsten dauert etwa 25 ms,
	// und genau dieser Übergang klingt wie ein Konsonant.
	const float FormantGlide = 1.0f - FMath::Exp(-1.0f / (0.025f * SampleRate));
	const float NoiseAlpha = 1.0f - FMath::Exp(-2.0f * PI * 700.0f / SampleRate);

	// Rauigkeit ist gemessene Unregelmäßigkeit: Jitter bis etwa 5 % der Periode, Shimmer bis 30 % der Amplitude.
	// Bei einer gesunden Stimme liegt der Jitter unter 1 % – hörbare Heiserkeit beginnt darüber.
	const float JitterAmount = 0.004f + 0.05f * Profile.Roughness;
	const float ShimmerAmount = 0.02f + 0.30f * Profile.Roughness;
	const float OpenQuotient = FMath::Lerp(0.52f, 0.82f, FMath::Clamp(Profile.Breathiness, 0.0f, 1.0f));
	const float BandwidthScale = FMath::Max(1.0f, Profile.FormantScale * 0.75f);

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		if (!bActive)
		{
			OutAudio[Frame] = 0.0f;
			continue;
		}

		const FSegment& Segment = Segments[SegmentIndex];
		const float SegmentAlpha = FMath::Clamp(SegmentTime / FMath::Max(0.001f, Segment.DurationSeconds), 0.0f, 1.0f);

		// 1. Formantziele des aktuellen Vokals, skaliert mit der Länge des Ansatzrohrs
		float Targets[4];
		GenesisVoiceLogic::GetVowelFormants(Segment.Vowel, Targets);
		const float Bandwidths[4] = { 80.0f, 90.0f, 120.0f, 130.0f };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const float Target = Targets[Index] * Profile.FormantScale;
			SmoothedFormants[Index] += (Target - SmoothedFormants[Index]) * FormantGlide;
			Formants[Index].Set(SmoothedFormants[Index], Bandwidths[Index] * BandwidthScale, SampleRate);
		}

		// 2. Quelle: Tonhöhe dieses Augenblicks
		const float Pitch = FMath::Lerp(Segment.PitchStart, Segment.PitchEnd, SegmentAlpha);
		const float Frequency = FMath::Clamp(Profile.F0Hz * Pitch, 50.0f, 1200.0f);

		float Voiced = 0.0f;
		if (Segment.Voicing > 0.0f)
		{
			GlottalPhase += Frequency * PeriodJitter * Step;
			if (GlottalPhase >= 1.0f)
			{
				GlottalPhase -= FMath::FloorToFloat(GlottalPhase);
				// Jede Periode ist ein bisschen anders lang und ein bisschen anders laut.
				// Ohne diese Unregelmäßigkeit klingt eine synthetische Stimme sofort nach Maschine.
				PeriodJitter = 1.0f + JitterAmount * (Rng.NextFloat() * 2.0f - 1.0f);
				PulseAmplitude = 1.0f + ShimmerAmount * (Rng.NextFloat() * 2.0f - 1.0f);
			}

			const float Flow = GlottalFlow(GlottalPhase, OpenQuotient) * PulseAmplitude;
			// Die Abstrahlung an den Lippen wirkt wie eine Ableitung – daher der helle, obertonreiche Klang
			Voiced = (Flow - PreviousFlow) * SampleRate / FMath::Max(100.0f, Frequency) * 0.5f;
			PreviousFlow = Flow;
		}

		// 3. Luft: Behauchtheit und Rauschanteile des Lauts
		const float White = Rng.NextFloat() * 2.0f - 1.0f;
		NoiseLowPass += (White - NoiseLowPass) * NoiseAlpha;
		const float Breath = White - NoiseLowPass;
		// Eine raue Stimme schließt die Stimmritze nicht mehr vollständig – ein Teil der Luft strömt
		// ungenutzt hindurch und rauscht. Deshalb gehört zur Heiserkeit nicht nur Unregelmäßigkeit,
		// sondern auch Geräusch.
		const float NoiseGain = Segment.Noise
			+ (Profile.Breathiness * 0.6f + Profile.Roughness * 0.5f) * Segment.Voicing;

		const float Source = Voiced * Segment.Voicing + Breath * NoiseGain * 0.6f;

		// 4. Filter: das Ansatzrohr als Kette von Resonanzen
		float Shaped = Source;
		for (FResonator& Resonator : Formants)
		{
			Shaped = Resonator.Process(Shaped);
		}

		// 5. Nase: eine tiefe, schmale Resonanz. Beim Summen ist sie fast alles, was man hört.
		const float NasalAmount = FMath::Clamp(Segment.Nasal + Profile.Nasality * 0.5f, 0.0f, 1.0f);
		if (NasalAmount > 0.001f)
		{
			NasalResonator.Set(280.0f * Profile.FormantScale, 150.0f, SampleRate);
			Shaped = FMath::Lerp(Shaped, NasalResonator.Process(Source) * 1.4f, NasalAmount * 0.6f);
		}

		// 6. Hüllkurve des Abschnitts: schneller Einsatz, weicher Abfall
		const float Attack = FMath::Clamp(SegmentTime / 0.012f, 0.0f, 1.0f);
		const float Release = FMath::Clamp((Segment.DurationSeconds - SegmentTime) / 0.022f, 0.0f, 1.0f);
		const float Envelope = Attack * Release * Segment.Gain;

		// Die Resonanzkette verstärkt kräftig (jeder Formant hat eine Güte um 10), und die Abstrahlung
		// an den Lippen tut es auch. Ohne diesen Pegel liefe alles dauerhaft in die Begrenzung –
		// dann klingt jede Stimme gleich laut und gleich hart.
		float Sample = Shaped * Envelope * Profile.Strength * 0.055f;

		// 7. Gleichanteil entfernen und weich begrenzen
		const float Blocked = Sample - DcBlockX1 + 0.995f * DcBlockY1;
		DcBlockX1 = Sample;
		DcBlockY1 = Blocked;
		OutAudio[Frame] = SoftClip(Blocked);

		SegmentTime += Step;
		if (SegmentTime >= Segment.DurationSeconds)
		{
			SegmentTime = 0.0f;
			++SegmentIndex;
			if (!Segments.IsValidIndex(SegmentIndex))
			{
				bActive = false;
			}
		}
	}
}
