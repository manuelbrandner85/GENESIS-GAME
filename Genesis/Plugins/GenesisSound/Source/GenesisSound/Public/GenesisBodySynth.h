// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.h"
#include "GenesisBodySynth.generated.h"

/**
 * Was der Körper gerade klingen lässt. Alle Werte kommen aus der Simulation –
 * Herzfrequenz aus dem Körper, Tiefpass und Lautstärken aus der Hörwahrnehmung,
 * Druck aus der Geburt. Nichts davon ist ein künstlerischer Regler.
 */
USTRUCT(BlueprintType)
struct GENESISSOUND_API FGenesisBodySoundParams
{
	GENERATED_BODY()

	/** Eigener Herzschlag (Schläge/min). Beim Ungeborenen 110–160, beim Erwachsenen 60–80. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float HeartRateBpm = 140.0f;

	/** Herzschlag der Mutter – das Lauteste, was ein Ungeborenes hört. 0, wenn keine Mutter da ist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float MotherHeartRateBpm = 72.0f;

	/** Atemzüge je Minute (nach der Geburt der eigene Atem, davor der der Mutter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float BreathsPerMinute = 14.0f;

	/** true, solange das Kind im Fruchtwasser hört. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") bool bInWomb = true;

	/** Tiefpass der Hörwahrnehmung (Hz). Im Mutterleib wenige hundert, an Luft fast der ganze Bereich. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float LowPassCutoffHz = 400.0f;

	/** 0..1 – wie laut Geräusche des eigenen Körpers sind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float BodyAudibility = 1.0f;

	/** 0..1 – wie laut die Welt draußen ist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float ExternalAudibility = 0.1f;

	/** 0..1 – Druck einer Wehe. Er presst das Gewebe und macht den Klang dumpfer und lauter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float Pressure = 0.0f;

	/** 0..1 – Sauerstoff. Zu wenig davon lässt das Rauschen im Ohr steigen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float Oxygen = 1.0f;

	/** Gesamtlautstärke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Sound") float MasterGain = 0.8f;
};

/**
 * Der Körperklang als Synthese.
 *
 * Nichts hier ist eine Aufnahme. Ein Herzschlag ist zwei gedämpfte Schwingungen ("lub" und "dub"),
 * der Blutstrom gefiltertes Rauschen im Takt der Systole, der Mutterleib ein tiefes Grundrauschen.
 * Das hat drei Gründe: Der Klang folgt jederzeit exakt den Werten der Simulation, er ist reproduzierbar,
 * und er lässt sich messen – ein abgespielter Klangschnipsel könnte nichts davon.
 *
 * Die Struktur ist bewusst frei von Unreal-Objekten: So läuft sie im Test ohne Welt und ohne Audiogerät.
 */
struct GENESISSOUND_API FGenesisBodySynth
{
	explicit FGenesisBodySynth(uint64 Seed = 0x50554C53ull);

	void Initialize(float InSampleRate);
	void SetParams(const FGenesisBodySoundParams& InParams) { Params = InParams; }
	const FGenesisBodySoundParams& GetParams() const { return Params; }

	/** Erzeugt Frames Mono-Abtastwerte. Der Zustand läuft über Aufrufe hinweg weiter. */
	void Render(float* OutAudio, int32 Frames);

	/** Setzt Phasen und Filter zurück (für reproduzierbare Messungen). */
	void Reset();

	/** Anzahl der Herzschläge seit dem letzten Zurücksetzen – für Tests und Anzeige. */
	int32 GetHeartbeatCount() const { return HeartbeatCount; }

private:
	/** Zwei Töne je Schlag: der erste beim Schließen der Segelklappen, der zweite beim Schließen der Taschenklappen. */
	float RenderHeartbeat(float Phase, float RateBpm, float& OutSystole) const;
	float NextNoise();

	FGenesisBodySoundParams Params;
	FGenesisRandomStream Rng;

	float SampleRate = 48000.0f;

	float HeartPhase = 0.0f;
	float MotherHeartPhase = 0.25f;
	float BreathPhase = 0.0f;
	int32 HeartbeatCount = 0;

	/** Brownsches Rauschen: integriertes weißes Rauschen, deshalb ein laufender Wert. */
	float BrownState = 0.0f;

	/** Zwei einpolige Tiefpässe in Reihe ergeben 12 dB je Oktave – genug für den Mutterleib. */
	float LowPassA = 0.0f;
	float LowPassB = 0.0f;

	/** Hochpass, damit der Blutstrom nicht im Grundrauschen versinkt. */
	float HighPassState = 0.0f;
	float HighPassLast = 0.0f;

	float BandState = 0.0f;
};
