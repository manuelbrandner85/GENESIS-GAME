// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.h"
#include "GenesisVoiceTypes.h"

/**
 * Die Stimme als Synthese – nach dem Quelle-Filter-Modell, mit dem die Phonetik den Menschen beschreibt.
 *
 * **Quelle**: Die Stimmlippen öffnen und schließen sich und lassen dabei Luftstöße durch. Jeder Stoß hat
 * eine Form (steiler Schluss, weicher Anstieg) – daraus entsteht der Obertongehalt. Unregelmäßigkeiten
 * von Periode zu Periode (Jitter) und von Stoß zu Stoß (Shimmer) machen eine Stimme rau; durchströmende
 * Luft, die nicht in Schwingung umgesetzt wird, macht sie behaucht.
 *
 * **Filter**: Der Raum zwischen Kehlkopf und Lippen hat Resonanzen – die Formanten. Sie entscheiden,
 * welcher Vokal zu hören ist, und ihre Lage hängt allein von der Länge dieses Raums ab.
 * Deshalb klingt ein Kind nicht wie ein leiser Erwachsener.
 *
 * Nichts davon ist eine Aufnahme. Das hat denselben Grund wie beim Körperklang: Der Ton folgt den Werten
 * der Simulation, er ist reproduzierbar, und er lässt sich messen.
 */
/**
 * Wie eine Stimme beim Hörer ankommt.
 *
 * Im Mutterleib hört ein Kind die Stimme seiner Mutter durch Bauchdecke und Fruchtwasser:
 * Die Höhen fehlen fast ganz, die Sprachmelodie bleibt. Genau deshalb erkennt ein Neugeborenes
 * die Stimme wieder, die es neun Monate lang gehört hat – es kennt ihre Melodie, nicht ihre Worte.
 */
struct GENESISVOICE_API FGenesisVoiceHearing
{
	/** Grenzfrequenz der Hörwahrnehmung (Hz). Im Fruchtwasser einige hundert, an Luft fast alles. */
	float CutoffHz = 16000.0f;

	/** Lautstärke beim Hörer (Entfernung, Dämpfung durch Gewebe). */
	float Gain = 1.0f;

	/** Filtert einen Block an Ort und Stelle. Der Zustand läuft über Aufrufe hinweg weiter. */
	void Process(float* Audio, int32 Frames, float SampleRate);

	void Reset() { Stage1 = 0.0f; Stage2 = 0.0f; Stage3 = 0.0f; Stage4 = 0.0f; }

private:
	float Stage1 = 0.0f;
	float Stage2 = 0.0f;
	float Stage3 = 0.0f;
	float Stage4 = 0.0f;
};

struct GENESISVOICE_API FGenesisVoiceSynth
{
	explicit FGenesisVoiceSynth(uint64 Seed = 0x564F4943ull);

	void Initialize(float InSampleRate);
	void SetProfile(const FGenesisVoiceProfile& InProfile) { Profile = InProfile; }
	const FGenesisVoiceProfile& GetProfile() const { return Profile; }

	/** Beginnt einen Laut. Ein laufender Laut wird dabei abgebrochen. */
	void Begin(const FGenesisUtterance& Utterance);

	/** true, solange noch etwas zu hören ist. */
	bool IsActive() const { return bActive; }

	/** Erzeugt Frames Mono-Abtastwerte und addiert sie **nicht** – der Puffer wird überschrieben. */
	void Render(float* OutAudio, int32 Frames);

	void Reset();

	/** Gesamtdauer des laufenden Lauts (s) – für Anzeige und Tests. */
	float GetPlannedDurationSeconds() const { return PlannedDuration; }

private:
	/** Ein Abschnitt eines Lauts: ein Vokal, eine Lautstärke, ein Stück Tonhöhenverlauf. */
	struct FSegment
	{
		float DurationSeconds = 0.2f;
		EGenesisVowel Vowel = EGenesisVowel::Schwa;
		/** 0..1 – Lautstärke dieses Abschnitts. 0 ist der Verschluss vor einer Silbe. */
		float Gain = 1.0f;
		/** Tonhöhe relativ zur Grundfrequenz, am Anfang und am Ende des Abschnitts. */
		float PitchStart = 1.0f;
		float PitchEnd = 1.0f;
		/** 0..1 – Anteil Stimme (der Rest ist Luft). */
		float Voicing = 1.0f;
		/** 0..1 – zusätzliches Rauschen (Einatmen, „schhh"). */
		float Noise = 0.0f;
		/** 0..1 – Nasenresonanz (Summen, Schnupfen). */
		float Nasal = 0.0f;
	};

	void PlanUtterance(const FGenesisUtterance& Utterance);
	void AddSyllable(EGenesisVowel Vowel, float Duration, float Gain, float PitchStart, float PitchEnd, float Voicing, float Noise, float Nasal);

	/** Ein Resonator zweiter Ordnung – ein Formant. */
	struct FResonator
	{
		float A = 0.0f, B = 0.0f, C = 0.0f;
		float Y1 = 0.0f, Y2 = 0.0f;

		void Set(float FrequencyHz, float BandwidthHz, float SampleRate);
		float Process(float In);
		void Reset() { Y1 = 0.0f; Y2 = 0.0f; }
	};

	/** Ein Luftstoß der Stimmlippen (Rosenberg-Modell): weicher Anstieg, steiler Schluss. */
	static float GlottalFlow(float Phase, float OpenQuotient);

	FGenesisVoiceProfile Profile;
	FGenesisRandomStream Rng;
	float SampleRate = 48000.0f;

	TArray<FSegment> Segments;
	int32 SegmentIndex = 0;
	float SegmentTime = 0.0f;
	float PlannedDuration = 0.0f;
	bool bActive = false;

	// Quelle
	float GlottalPhase = 0.0f;
	float PeriodJitter = 1.0f;
	float PulseAmplitude = 1.0f;
	float PreviousFlow = 0.0f;

	// Filter
	FResonator Formants[4];
	FResonator NasalResonator;
	float SmoothedFormants[4] = { 500.0f, 1500.0f, 2500.0f, 3400.0f };
	float DcBlockX1 = 0.0f;
	float DcBlockY1 = 0.0f;
	float NoiseLowPass = 0.0f;
};
