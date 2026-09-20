// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"

/**
 * Wie ein Klang der Welt beim Hörer ankommt.
 *
 * Im Mutterleib hört ein Kind alles durch Bauchdecke und Fruchtwasser: Die Höhen fehlen fast ganz,
 * die Melodie bleibt. Genau deshalb erkennt ein Neugeborenes die Stimme seiner Mutter wieder –
 * es kennt ihren Klangverlauf, nicht ihre Worte.
 *
 * Vier einpolige Tiefpässe hintereinander ergeben 24 dB je Oktave. Gewebe und Flüssigkeit dämpfen
 * mit der Frequenz steil; ein flacherer Filter klänge nach Decke über dem Lautsprecher, ein steilerer
 * nach Telefon.
 *
 * Derselbe Filter gilt für Stimmen und für den Klang der Orte – es ist dasselbe Ohr.
 */
struct GENESISAUDIOCORE_API FGenesisHearingFilter
{
	/** Grenzfrequenz der Hörwahrnehmung (Hz). Im Fruchtwasser einige hundert, an Luft fast alles. */
	float CutoffHz = 16000.0f;

	/** Lautstärke beim Hörer (Entfernung, Dämpfung durch Gewebe, Mix). */
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
