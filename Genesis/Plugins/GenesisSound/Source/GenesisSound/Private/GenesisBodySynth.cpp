// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodySynth.h"

namespace
{
	/** Gedämpfte Schwingung: der Klang eines schließenden Ventils. */
	float DampedTone(float TimeSeconds, float FrequencyHz, float DecayPerSecond)
	{
		if (TimeSeconds < 0.0f)
		{
			return 0.0f;
		}
		return FMath::Sin(2.0f * PI * FrequencyHz * TimeSeconds) * FMath::Exp(-DecayPerSecond * TimeSeconds);
	}

	/** Einpoliger Tiefpass: Glättungsfaktor aus Grenzfrequenz und Abtastrate. */
	float LowPassAlpha(float CutoffHz, float SampleRate)
	{
		const float Clamped = FMath::Clamp(CutoffHz, 20.0f, SampleRate * 0.45f);
		const float Tau = 1.0f / (2.0f * PI * Clamped);
		return 1.0f - FMath::Exp(-1.0f / (Tau * SampleRate));
	}
}

FGenesisBodySynth::FGenesisBodySynth(uint64 Seed)
	: Rng(Seed)
{
}

void FGenesisBodySynth::Initialize(float InSampleRate)
{
	SampleRate = FMath::Max(8000.0f, InSampleRate);
	Reset();
}

void FGenesisBodySynth::Reset()
{
	HeartPhase = 0.0f;
	MotherHeartPhase = 0.25f;
	BreathPhase = 0.0f;
	HeartbeatCount = 0;
	BrownState = 0.0f;
	LowPassA = 0.0f;
	LowPassB = 0.0f;
	HighPassState = 0.0f;
	HighPassLast = 0.0f;
	BandState = 0.0f;
}

float FGenesisBodySynth::NextNoise()
{
	return Rng.FRandRange(-1.0f, 1.0f);
}

float FGenesisBodySynth::RenderHeartbeat(float Phase, float RateBpm, float& OutSystole) const
{
	const float Period = 60.0f / FMath::Max(20.0f, RateBpm);
	const float Time = Phase * Period;

	// "lub": Segelklappen, tief und kräftig. "dub": Taschenklappen, kürzer und etwas höher.
	// Der Abstand beträgt etwa ein Drittel des Zyklus – bei schnellem Herzschlag rücken sie zusammen.
	const float First = DampedTone(Time, 42.0f, 26.0f) * 1.0f;
	const float Second = DampedTone(Time - 0.32f * Period, 58.0f, 34.0f) * 0.55f;

	// Die Systole ist die Zeit, in der das Blut ausgeworfen wird – danach strömt es weiter
	OutSystole = FMath::Clamp(FMath::Exp(-9.0f * Time) + 0.45f * FMath::Exp(-7.0f * FMath::Max(0.0f, Time - 0.32f * Period)), 0.0f, 1.5f);
	return First + Second;
}

void FGenesisBodySynth::Render(float* OutAudio, int32 Frames)
{
	if (!OutAudio || Frames <= 0)
	{
		return;
	}

	const float Inverse = 1.0f / SampleRate;

	// Der Druck einer Wehe presst Gewebe gegen das Ohr: alles wird dumpfer und näher
	const float PressureFactor = FMath::Clamp(Params.Pressure, 0.0f, 1.0f);
	const float Cutoff = FMath::Max(60.0f, Params.LowPassCutoffHz * (1.0f - 0.45f * PressureFactor));
	const float Alpha = LowPassAlpha(Cutoff, SampleRate);
	// Unter Sauerstoffmangel rauscht es im Ohr – das ist keine Stilentscheidung, sondern ein Symptom
	const float Deficit = FMath::Clamp(1.0f - Params.Oxygen, 0.0f, 1.0f);

	const float HeartPeriod = 60.0f / FMath::Max(20.0f, Params.HeartRateBpm);
	const float MotherPeriod = 60.0f / FMath::Max(20.0f, Params.MotherHeartRateBpm);
	const float BreathPeriod = 60.0f / FMath::Max(2.0f, Params.BreathsPerMinute);

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		// 1. Eigener Herzschlag
		float Systole = 0.0f;
		float Heart = RenderHeartbeat(HeartPhase, Params.HeartRateBpm, Systole);

		// 2. Herzschlag der Mutter – für ein Ungeborenes das vertrauteste Geräusch überhaupt
		float MotherSystole = 0.0f;
		float MotherHeart = 0.0f;
		if (Params.bInWomb && Params.MotherHeartRateBpm > 0.0f)
		{
			MotherHeart = RenderHeartbeat(MotherHeartPhase, Params.MotherHeartRateBpm, MotherSystole) * 0.85f;
		}

		// 3. Blutstrom: Rauschen, das mit dem Auswurf lauter wird (Bandpass aus Hoch- und Tiefpass)
		const float Noise = NextNoise();
		BandState += (Noise - BandState) * 0.25f;                 // grober Tiefpass
		const float Banded = BandState - HighPassState;           // minus Tiefanteil = Bandpass
		HighPassState += (BandState - HighPassState) * 0.02f;
		const float Flow = Banded * (0.35f + 0.65f * FMath::Min(1.5f, Systole + MotherSystole)) * 0.5f;

		// 4. Mutterleib: braunes Rauschen (integriertes weißes Rauschen), tief und dauernd
		BrownState = FMath::Clamp(BrownState + Noise * 0.02f, -1.0f, 1.0f);
		BrownState *= 0.9995f;
		const float Womb = Params.bInWomb ? BrownState * 1.4f : BrownState * 0.25f;

		// 5. Atem: langsame Hüllkurve auf gefiltertem Rauschen
		const float BreathEnvelope = FMath::Max(0.0f, FMath::Sin(2.0f * PI * BreathPhase));
		const float Breath = Banded * BreathEnvelope * BreathEnvelope * 0.35f;

		// 6. Ohrenrauschen bei Sauerstoffmangel
		const float Hiss = Noise * Deficit * Deficit * 0.25f;

		const float BodyMix = (Heart * 0.55f + MotherHeart * 0.45f + Flow * 0.5f + Womb * 0.45f + Breath * 0.3f)
			* FMath::Clamp(Params.BodyAudibility, 0.0f, 2.0f);
		const float ExternalMix = (Hiss + Noise * 0.05f) * FMath::Clamp(Params.ExternalAudibility, 0.0f, 2.0f);

		// 7. Hörwahrnehmung: zwei Tiefpässe in Reihe. Der Mutterleib lässt nur tiefe Frequenzen durch;
		// an Luft steht der Filter weit offen und die Welt wird schlagartig hell.
		const float Raw = BodyMix + ExternalMix;
		LowPassA += (Raw - LowPassA) * Alpha;
		LowPassB += (LowPassA - LowPassB) * Alpha;

		// Gleichanteil entfernen – sonst wandert das Signal mit dem braunen Rauschen davon
		const float Filtered = LowPassB - HighPassLast;
		HighPassLast += (LowPassB - HighPassLast) * 0.0008f;

		const float Gain = Params.MasterGain * (1.0f + 0.35f * PressureFactor);
		OutAudio[Frame] = FMath::Clamp(Filtered * Gain, -1.0f, 1.0f);

		// Phasen weiterdrehen
		HeartPhase += Inverse / HeartPeriod;
		if (HeartPhase >= 1.0f)
		{
			HeartPhase -= 1.0f;
			++HeartbeatCount;
		}
		MotherHeartPhase += Inverse / MotherPeriod;
		if (MotherHeartPhase >= 1.0f)
		{
			MotherHeartPhase -= 1.0f;
		}
		BreathPhase += Inverse / BreathPeriod;
		if (BreathPhase >= 1.0f)
		{
			BreathPhase -= 1.0f;
		}
	}
}
