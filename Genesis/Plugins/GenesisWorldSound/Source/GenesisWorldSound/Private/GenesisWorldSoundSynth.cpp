// GENESIS: Der Kreislauf des Lebens

#include "GenesisWorldSoundSynth.h"

namespace
{
	float SoftClip(float Value)
	{
		return FMath::Tanh(Value);
	}

	/** Einpoliger Tiefpass als Koeffizient. */
	float LowPassAlpha(float CutoffHz, float SampleRate)
	{
		return 1.0f - FMath::Exp(-2.0f * PI * FMath::Clamp(CutoffHz, 5.0f, SampleRate * 0.45f) / SampleRate);
	}
}

FGenesisWorldSoundSynth::FGenesisWorldSoundSynth(uint64 Seed)
	: Rng(Seed)
{
	ActiveEvents.Reserve(16);
}

void FGenesisWorldSoundSynth::Initialize(float InSampleRate)
{
	SampleRate = FMath::Max(8000.0f, InSampleRate);
	Reset();
}

void FGenesisWorldSoundSynth::Reset()
{
	Time = 0.0f;
	LowNoise = 0.0f;
	MidNoise = 0.0f;
	HighNoise = 0.0f;
	BedLfoPhase = 0.0f;
	PlaceFilter1 = 0.0f;
	PlaceFilter2 = 0.0f;
	HeartPhase = 0.0f;
	ActiveEvents.Reset();
	for (int32 Index = 0; Index < EventTypeCount; ++Index)
	{
		EventCounts[Index] = 0;
		NextEventTime[Index] = 0.0f;
		ScheduleNext(static_cast<EGenesisWorldEvent>(Index));
	}
}

void FGenesisWorldSoundSynth::SetParams(const FGenesisWorldSoundParams& InParams)
{
	const bool bPlaceChanged = InParams.Place != Params.Place;
	Params = InParams;
	if (bPlaceChanged)
	{
		// Ein anderer Ort heißt andere Ereignisse – die alten Termine gelten nicht mehr
		for (int32 Index = 0; Index < EventTypeCount; ++Index)
		{
			NextEventTime[Index] = Time;
			ScheduleNext(static_cast<EGenesisWorldEvent>(Index));
		}
	}
}

int32 FGenesisWorldSoundSynth::GetEventCount(EGenesisWorldEvent Event) const
{
	const int32 Index = static_cast<int32>(Event);
	return Index >= 0 && Index < EventTypeCount ? EventCounts[Index] : 0;
}

float FGenesisWorldSoundSynth::GetEventRatePerMinute(EGenesisWorldEvent Event) const
{
	// Häufigkeiten aus der Beobachtung: Darmgeräusche treten beim Menschen 5- bis 30-mal je Minute auf,
	// im Kreißsaal klappert es unter der Geburt öfter als davor.
	switch (Params.Place)
	{
	case EGenesisPlace::Womb:
		switch (Event)
		{
		case EGenesisWorldEvent::Gurgle: return FMath::Lerp(5.0f, 26.0f, Params.Digestion);
		case EGenesisWorldEvent::PlacentalWhoosh: return Params.MaternalHeartRateBpm;
		case EGenesisWorldEvent::DistantVoices: return 2.0f * Params.Activity;
		default: return 0.0f;
		}

	case EGenesisPlace::OviductAmpulla:
		switch (Event)
		{
		case EGenesisWorldEvent::Drop: return 12.0f;
		case EGenesisWorldEvent::PlacentalWhoosh: return Params.MaternalHeartRateBpm * 0.5f;
		default: return 0.0f;
		}

	case EGenesisPlace::DeliveryRoom:
		switch (Event)
		{
		case EGenesisWorldEvent::Monitor: return Params.MaternalHeartRateBpm;
		case EGenesisWorldEvent::Clink: return FMath::Lerp(1.5f, 9.0f, Params.Activity);
		case EGenesisWorldEvent::Cloth: return FMath::Lerp(3.0f, 14.0f, Params.Activity);
		case EGenesisWorldEvent::Footstep: return FMath::Lerp(4.0f, 24.0f, Params.Activity);
		case EGenesisWorldEvent::DistantVoices: return FMath::Lerp(2.0f, 8.0f, Params.Activity);
		case EGenesisWorldEvent::Drop: return 2.0f;
		default: return 0.0f;
		}

	default:
		return 0.0f;
	}
}

void FGenesisWorldSoundSynth::ScheduleNext(EGenesisWorldEvent Event)
{
	const int32 Index = static_cast<int32>(Event);
	const float RatePerMinute = GetEventRatePerMinute(Event);
	if (RatePerMinute <= 0.0f)
	{
		NextEventTime[Index] = TNumericLimits<float>::Max();
		return;
	}

	if (Event == EGenesisWorldEvent::Monitor || Event == EGenesisWorldEvent::PlacentalWhoosh)
	{
		// Herzschlag: kein Zufall, sondern Takt – mit einer Spur Schwankung, wie ein echtes Herz
		const float Period = 60.0f / FMath::Max(20.0f, RatePerMinute);
		NextEventTime[Index] = Time + Period * (1.0f + 0.03f * (Rng.NextFloat() * 2.0f - 1.0f));
		return;
	}

	// Poisson-Prozess: Die Wartezeit ist exponentiell verteilt. Dadurch schwankt der Abstand,
	// die mittlere Häufigkeit stimmt trotzdem.
	const float MeanSeconds = 60.0f / RatePerMinute;
	const float Uniform = FMath::Max(0.0001f, Rng.NextFloat());
	NextEventTime[Index] = Time - MeanSeconds * FMath::Loge(Uniform);
}

void FGenesisWorldSoundSynth::StartEvent(EGenesisWorldEvent Event)
{
	const int32 Index = static_cast<int32>(Event);
	++EventCounts[Index];

	if (ActiveEvents.Num() >= 12)
	{
		return;
	}

	FActiveEvent Active;
	Active.Type = Event;

	switch (Event)
	{
	case EGenesisWorldEvent::Gurgle:
		// Ein Darmgeräusch ist Flüssigkeit in einem Schlauch: ein Gluckern, das die Tonhöhe verliert
		Active.Duration = 0.35f + 0.9f * Rng.NextFloat();
		Active.Frequency = 180.0f + 420.0f * Rng.NextFloat();
		Active.FrequencyEnd = Active.Frequency * (0.35f + 0.4f * Rng.NextFloat());
		Active.Gain = 0.35f + 0.4f * Rng.NextFloat();
		break;

	case EGenesisWorldEvent::PlacentalWhoosh:
		// Das Blut schießt in den Mutterkuchen – ein weiches Rauschen im Takt der Systole
		Active.Duration = 0.28f;
		Active.Frequency = 120.0f;
		Active.FrequencyEnd = 60.0f;
		Active.Gain = 0.5f;
		break;

	case EGenesisWorldEvent::Drop:
		Active.Duration = 0.18f;
		Active.Frequency = 700.0f + 900.0f * Rng.NextFloat();
		Active.FrequencyEnd = Active.Frequency * 0.45f;
		Active.Gain = 0.22f + 0.2f * Rng.NextFloat();
		break;

	case EGenesisWorldEvent::Clink:
		// Metall: zwei hohe, lange nachklingende Resonanzen
		Active.Duration = 0.5f;
		Active.Frequency = 2400.0f + 3200.0f * Rng.NextFloat();
		Active.FrequencyEnd = Active.Frequency;
		Active.Gain = 0.3f + 0.25f * Rng.NextFloat();
		break;

	case EGenesisWorldEvent::Cloth:
		Active.Duration = 0.22f + 0.25f * Rng.NextFloat();
		Active.Frequency = 2600.0f;
		Active.FrequencyEnd = 1400.0f;
		Active.Gain = 0.18f + 0.15f * Rng.NextFloat();
		break;

	case EGenesisWorldEvent::Monitor:
		Active.Duration = 0.09f;
		Active.Frequency = 980.0f;
		Active.FrequencyEnd = 980.0f;
		Active.Gain = 0.22f;
		break;

	case EGenesisWorldEvent::DistantVoices:
		// Stimmen im Gang: die Melodie ist da, die Worte sind es nicht
		Active.Duration = 0.9f + 1.8f * Rng.NextFloat();
		Active.Frequency = 300.0f + 260.0f * Rng.NextFloat();
		Active.FrequencyEnd = Active.Frequency * 0.8f;
		Active.Gain = 0.12f + 0.1f * Rng.NextFloat();
		break;

	case EGenesisWorldEvent::Footstep:
	default:
		Active.Duration = 0.14f;
		Active.Frequency = 110.0f + 60.0f * Rng.NextFloat();
		Active.FrequencyEnd = Active.Frequency * 0.6f;
		Active.Gain = 0.22f + 0.15f * Rng.NextFloat();
		break;
	}

	ActiveEvents.Add(Active);
}

float FGenesisWorldSoundSynth::NextNoise()
{
	return Rng.NextFloat() * 2.0f - 1.0f;
}

float FGenesisWorldSoundSynth::RenderEvent(FActiveEvent& Event, float Noise)
{
	const float Alpha = FMath::Clamp(Event.Time / FMath::Max(0.001f, Event.Duration), 0.0f, 1.0f);
	const float Frequency = FMath::Lerp(Event.Frequency, Event.FrequencyEnd, Alpha);

	// Hüllkurve: schneller Einsatz, exponentieller Ausklang – so klingt fast jedes kurze Geräusch
	const float Attack = FMath::Clamp(Event.Time / 0.006f, 0.0f, 1.0f);
	const float Decay = FMath::Exp(-4.0f * Alpha);
	const float Envelope = Attack * Decay;

	float Value = 0.0f;
	switch (Event.Type)
	{
	case EGenesisWorldEvent::Monitor:
	{
		Event.Phase += Frequency / SampleRate;
		Value = FMath::Sin(2.0f * PI * Event.Phase) * 0.8f;
		break;
	}
	case EGenesisWorldEvent::Clink:
	{
		// Zwei Teiltöne im unharmonischen Abstand: daran erkennt das Ohr Metall
		Event.Phase += Frequency / SampleRate;
		Value = FMath::Sin(2.0f * PI * Event.Phase) * 0.6f
			+ FMath::Sin(2.0f * PI * Event.Phase * 2.71f) * 0.35f;
		break;
	}
	case EGenesisWorldEvent::Gurgle:
	case EGenesisWorldEvent::Drop:
	{
		// Gluckern: ein Ton, der die Tonhöhe verliert, mit etwas Flüssigkeitsrauschen darin
		Event.Phase += Frequency / SampleRate;
		Value = FMath::Sin(2.0f * PI * Event.Phase) * 0.7f + Noise * 0.35f;
		break;
	}
	default:
	{
		// Rauschbasierte Ereignisse (Tücher, Schritte, Stimmen, Mutterkuchen): Bandpass um die Frequenz
		const float Alpha1 = LowPassAlpha(Frequency, SampleRate);
		const float Alpha2 = LowPassAlpha(Frequency * 0.25f, SampleRate);
		Event.Filter1 += (Noise - Event.Filter1) * Alpha1;
		Event.Filter2 += (Event.Filter1 - Event.Filter2) * Alpha2;
		Value = (Event.Filter1 - Event.Filter2) * 3.0f;

		if (Event.Type == EGenesisWorldEvent::DistantVoices)
		{
			// Sprechen hat einen Rhythmus: etwa vier Silben je Sekunde
			Value *= 0.6f + 0.4f * FMath::Sin(2.0f * PI * 4.0f * Event.Time);
		}
		break;
	}
	}

	Event.Time += 1.0f / SampleRate;
	return Value * Envelope * Event.Gain;
}

void FGenesisWorldSoundSynth::Render(float* OutAudio, int32 Frames)
{
	if (!OutAudio || Frames <= 0)
	{
		return;
	}

	if (Params.Place == EGenesisPlace::None)
	{
		FMemory::Memzero(OutAudio, sizeof(float) * Frames);
		return;
	}

	// Grundton je Ort: wie tief, wie breit, wie hell
	float LowCutoff = 90.0f;
	float MidCutoff = 700.0f;
	float HighCutoff = 6000.0f;
	float LowGain = 0.5f;
	float MidGain = 0.2f;
	float HighGain = 0.02f;
	/** Oberhalb dieser Frequenz kommt aus diesem Ort nichts mehr heraus. */
	float PlaceCutoff = 12000.0f;

	switch (Params.Place)
	{
	case EGenesisPlace::OviductAmpulla:
		// Flüssigkeit und Flimmerhärchen: fast nur tiefes Strömen, darüber ein feines Schimmern
		LowCutoff = 120.0f; MidCutoff = 500.0f; HighCutoff = 9000.0f;
		LowGain = 0.45f; MidGain = 0.16f; HighGain = 0.05f;
		PlaceCutoff = 7000.0f;
		break;

	case EGenesisPlace::Womb:
		// Der Mutterleib ist ein Tiefpass: Alles über ein paar hundert Hertz bleibt draußen
		LowCutoff = 80.0f; MidCutoff = 320.0f; HighCutoff = 900.0f;
		LowGain = 0.6f; MidGain = 0.22f; HighGain = 0.01f;
		// Der Mutterleib lässt oberhalb von etwa 700 Hz kaum noch etwas durch – gemessen an
		// Aufnahmen aus der Gebärmutter fehlt dort fast der gesamte hörbare Rest.
		PlaceCutoff = 700.0f;
		break;

	case EGenesisPlace::DeliveryRoom:
		// Lüftung und Geräte: ein leises, breites Grundrauschen, das man erst vermisst, wenn es weg ist
		LowCutoff = 110.0f; MidCutoff = 1200.0f; HighCutoff = 8000.0f;
		LowGain = 0.3f; MidGain = 0.13f; HighGain = 0.05f;
		PlaceCutoff = 13000.0f;
		break;

	default:
		break;
	}

	const float LowAlpha = LowPassAlpha(LowCutoff, SampleRate);
	const float MidAlpha = LowPassAlpha(MidCutoff, SampleRate);
	const float HighAlpha = LowPassAlpha(HighCutoff, SampleRate);
	const float PlaceAlpha = LowPassAlpha(PlaceCutoff, SampleRate);
	const float Step = 1.0f / SampleRate;

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		const float Noise = NextNoise();

		// 1. Grundton aus drei Rauschbändern, langsam atmend
		LowNoise += (Noise - LowNoise) * LowAlpha;
		MidNoise += (Noise - MidNoise) * MidAlpha;
		HighNoise += (Noise - HighNoise) * HighAlpha;

		BedLfoPhase += 0.07f * Step;
		const float Breath = 0.85f + 0.15f * FMath::Sin(2.0f * PI * BedLfoPhase);
		float Sample = (LowNoise * LowGain * 4.0f + (MidNoise - LowNoise) * MidGain * 3.0f
			+ (HighNoise - MidNoise) * HighGain * 3.0f) * Breath;

		// 2. Fällige Ereignisse starten
		for (int32 Index = 0; Index < EventTypeCount; ++Index)
		{
			while (Time >= NextEventTime[Index])
			{
				StartEvent(static_cast<EGenesisWorldEvent>(Index));
				ScheduleNext(static_cast<EGenesisWorldEvent>(Index));
			}
		}

		// 3. Laufende Ereignisse mischen
		for (int32 Index = ActiveEvents.Num() - 1; Index >= 0; --Index)
		{
			FActiveEvent& Event = ActiveEvents[Index];
			Sample += RenderEvent(Event, NextNoise());
			if (Event.Time >= Event.Duration)
			{
				ActiveEvents.RemoveAtSwap(Index);
			}
		}

		// 4. Bandgrenze des Ortes. Sie gilt für alles, was hier geschieht: Ein Gluckern hinter der
		// Gebärmutterwand ist genauso gedämpft wie das Grundrauschen – sonst klänge der Mutterleib
		// dumpf, aber jedes einzelne Geräusch darin hell.
		PlaceFilter1 += (Sample - PlaceFilter1) * PlaceAlpha;
		PlaceFilter2 += (PlaceFilter1 - PlaceFilter2) * PlaceAlpha;

		Time += Step;
		OutAudio[Frame] = SoftClip(PlaceFilter2 * Params.Loudness * 0.7f);
	}
}
