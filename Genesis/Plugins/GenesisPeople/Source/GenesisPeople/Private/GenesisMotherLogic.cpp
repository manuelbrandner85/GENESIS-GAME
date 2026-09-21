// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherLogic.h"

namespace
{
	/** xorshift32 – klein, schnell, reproduzierbar. */
	float NextUnit(uint32& StateBits)
	{
		uint32 X = StateBits ? StateBits : 0x9E3779B9u;
		X ^= X << 13;
		X ^= X >> 17;
		X ^= X << 5;
		StateBits = X;
		return static_cast<float>(X & 0xFFFFFFu) / static_cast<float>(0x1000000u);
	}

	/**
	 * Abstände zwischen Lidschlägen sind nicht gleichmäßig, sondern rechtsschief verteilt:
	 * viele kurze, einige lange Pausen. Eine Exponentialverteilung mit Mindestabstand trifft das gut.
	 */
	float DrawBlinkInterval(uint32& StateBits, float BlinksPerMinute, float MinInterval)
	{
		const float Mean = 60.0f / FMath::Max(1.0f, BlinksPerMinute);
		const float U = FMath::Clamp(NextUnit(StateBits), 1.0e-4f, 0.9999f);
		const float Exponential = -FMath::Loge(U) * FMath::Max(0.05f, Mean - MinInterval);
		return MinInterval + FMath::Min(Exponential, Mean * 4.0f);
	}
}

FGenesisMotherState GenesisMotherLogic::Begin(uint32 Seed)
{
	FGenesisMotherState State;
	State.RandomState = Seed ? Seed : 0x9E3779B9u;
	const FGenesisMotherTuning Defaults;
	State.NextBlinkIn = DrawBlinkInterval(State.RandomState, Defaults.BlinksPerMinute, Defaults.MinBlinkIntervalSeconds);
	State.BreathPhase = NextUnit(State.RandomState);
	State.Smile = Defaults.RestingSmile;
	return State;
}

void GenesisMotherLogic::Advance(FGenesisMotherState& State, const FGenesisMotherTuning& Tuning, const FGenesisMotherInputs& Inputs, float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.0f, 0.25f);
	if (Dt <= 0.0f)
	{
		return;
	}

	// Atem
	State.BreathPhase = FMath::Fmod(State.BreathPhase + Dt * Tuning.BreathsPerMinute / 60.0f, 1000.0f);

	// Heben und Zurücklegen. Vor das Gesicht holt sie das Kind nur, wenn es auf ihr liegt.
	const bool bWantsEnFace = Inputs.bChildOnChest && Inputs.bChildSeeksFace;
	const float Step = Dt / FMath::Max(0.5f, Tuning.EnFaceSeconds);
	State.EnFace = FMath::Clamp(State.EnFace + (bWantsEnFace ? Step : -Step), 0.0f, 1.0f);

	// Blickkontakt zählt erst, wenn die Gesichter einander gegenüber sind
	const bool bEyeContact = Inputs.bChildLooksAtEyes && IsFaceToFace(State);
	State.EyeContactSeconds = bEyeContact ? State.EyeContactSeconds + Dt : 0.0f;

	// Lidschlag
	if (State.BlinkElapsed >= 0.0f)
	{
		State.BlinkElapsed += Dt;
		if (State.BlinkElapsed >= Tuning.BlinkSeconds)
		{
			State.BlinkElapsed = -1.0f;
			const float Rate = Tuning.BlinksPerMinute * (bEyeContact ? Tuning.EyeContactBlinkFactor : 1.0f);
			State.NextBlinkIn = DrawBlinkInterval(State.RandomState, Rate, Tuning.MinBlinkIntervalSeconds);
		}
	}
	else
	{
		State.NextBlinkIn -= Dt;
		if (State.NextBlinkIn <= 0.0f)
		{
			State.BlinkElapsed = 0.0f;
			++State.Blinks;
		}
	}

	// Lächeln: Es kommt mit dem Blickkontakt, nicht mit der Zeit
	const float SmileTarget = bEyeContact ? Tuning.EyeContactSmile : Tuning.RestingSmile;
	const float Rate = SmileTarget > State.Smile ? Tuning.SmileRisePerSecond : Tuning.SmileFallPerSecond;
	State.Smile = FMath::FInterpConstantTo(State.Smile, SmileTarget, Dt, Rate);
}

float GenesisMotherLogic::GetBreathLift(const FGenesisMotherState& State, const FGenesisMotherTuning& Tuning)
{
	const float Phase = FMath::Frac(State.BreathPhase);
	const float Inhale = FMath::Clamp(Tuning.InhaleFraction, 0.2f, 0.6f);
	if (Phase < Inhale)
	{
		return FMath::SmoothStep(0.0f, 1.0f, Phase / Inhale);
	}
	return 1.0f - FMath::SmoothStep(0.0f, 1.0f, (Phase - Inhale) / (1.0f - Inhale));
}

float GenesisMotherLogic::GetBlinkClosure(const FGenesisMotherState& State, const FGenesisMotherTuning& Tuning)
{
	if (State.BlinkElapsed < 0.0f)
	{
		return 0.0f;
	}
	const float T = State.BlinkElapsed / FMath::Max(0.05f, Tuning.BlinkSeconds);
	const float Close = 1.0f / 3.0f;
	return T < Close
		? FMath::SmoothStep(0.0f, 1.0f, T / Close)
		: 1.0f - FMath::SmoothStep(0.0f, 1.0f, (T - Close) / (1.0f - Close));
}

float GenesisMotherLogic::GetEnFaceBlend(const FGenesisMotherState& State)
{
	return FMath::SmoothStep(0.0f, 1.0f, State.EnFace);
}

bool GenesisMotherLogic::IsFaceToFace(const FGenesisMotherState& State)
{
	return State.EnFace >= 0.95f;
}
