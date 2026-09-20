// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.h"
#include "GenesisWorldSoundTypes.h"

/**
 * Der Klang eines Ortes als Synthese.
 *
 * Zwei Schichten:
 *
 * 1. **Grundton** – gefiltertes Rauschen, tief und langsam moduliert. Jeder Raum hat einen,
 *    und man hört ihn erst, wenn er fehlt.
 * 2. **Ereignisse** – einzelne Geräusche zu Zeitpunkten, die aus einem Poisson-Prozess stammen:
 *    im Mittel so und so viele je Minute, aber nie im Takt. Ein Raum, in dem es alle fünf Sekunden
 *    klappert, klingt nach Maschine; ein Raum, in dem es im Mittel alle fünf Sekunden klappert,
 *    klingt nach Raum.
 *
 * Die Struktur ist frei von Unreal-Objekten: Sie läuft im Test ohne Welt und ohne Audiogerät.
 */
struct GENESISWORLDSOUND_API FGenesisWorldSoundSynth
{
	explicit FGenesisWorldSoundSynth(uint64 Seed = 0x4F5254ull);

	void Initialize(float InSampleRate);
	void SetParams(const FGenesisWorldSoundParams& InParams);
	const FGenesisWorldSoundParams& GetParams() const { return Params; }

	/** Erzeugt Frames Mono-Abtastwerte. Der Puffer wird überschrieben. */
	void Render(float* OutAudio, int32 Frames);

	void Reset();

	/** Wie oft ein Ereignis seit dem letzten Zurücksetzen vorkam – für Tests und Anzeige. */
	int32 GetEventCount(EGenesisWorldEvent Event) const;

	/** Mittlere Häufigkeit je Minute, die dieser Ort für dieses Ereignis vorsieht. */
	float GetEventRatePerMinute(EGenesisWorldEvent Event) const;

private:
	/** Ein laufendes Ereignis. */
	struct FActiveEvent
	{
		EGenesisWorldEvent Type = EGenesisWorldEvent::Gurgle;
		float Time = 0.0f;
		float Duration = 0.3f;
		float Frequency = 400.0f;
		float FrequencyEnd = 300.0f;
		float Gain = 0.5f;
		/** Zustand des eigenen Filters, damit Ereignisse sich nicht gegenseitig verfärben. */
		float Filter1 = 0.0f;
		float Filter2 = 0.0f;
		float Phase = 0.0f;
	};

	void ScheduleNext(EGenesisWorldEvent Event);
	void StartEvent(EGenesisWorldEvent Event);
	float RenderEvent(FActiveEvent& Event, float Noise);
	float NextNoise();

	FGenesisWorldSoundParams Params;
	FGenesisRandomStream Rng;
	float SampleRate = 48000.0f;
	float Time = 0.0f;

	// Grundton
	float LowNoise = 0.0f;
	/** Bandgrenze des Ortes – sie gilt für den Grundton **und** für die Ereignisse. */
	float PlaceFilter1 = 0.0f;
	float PlaceFilter2 = 0.0f;
	float MidNoise = 0.0f;
	float HighNoise = 0.0f;
	float BedLfoPhase = 0.0f;

	// Ereignisse
	static constexpr int32 EventTypeCount = 8;
	float NextEventTime[EventTypeCount] = { 0.0f };
	int32 EventCounts[EventTypeCount] = { 0 };
	TArray<FActiveEvent> ActiveEvents;

	/** Der Takt des mütterlichen Herzens – er taktet Mutterkuchen und Monitor. */
	float HeartPhase = 0.0f;
};
