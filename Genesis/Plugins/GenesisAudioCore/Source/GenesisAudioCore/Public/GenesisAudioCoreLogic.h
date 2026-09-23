// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisAudioTypes.h"

struct FGenesisBodyState;
struct FGenesisTimestamp;

/**
 * Zustandslose Audio-Grundlogik: Wie hört diese Person, wie klingt ihr Körper, wie verteilt sich der Mix?
 * Audio vermittelt Zustände ohne Zahlen – deshalb leitet sich alles aus der Simulation ab.
 */
namespace GenesisAudioCoreLogic
{
	/** Hörwahrnehmung aus dem Körperzustand (kein Körper → neutrales Hören). */
	GENESISAUDIOCORE_API FGenesisHearingPerception ComputeHearing(const FGenesisBodyState* Body, const FGenesisTimestamp& Now, const FGenesisHearingTuning& Tuning);

	/** Parameter der Körperklänge – abhängig davon, wie deutlich der Körper gerade gehört wird. */
	GENESISAUDIOCORE_API FGenesisBodyAudioParams ComputeBodyAudio(const FGenesisBodyState* Body, const FGenesisTimestamp& Now, const FGenesisHearingPerception& Perception);

	/**
	 * Glättet Wahrnehmungswechsel. Der Tiefpass wird logarithmisch interpoliert (wie das Ohr Tonhöhe empfindet).
	 * Wechsel zwischen Mutterleib und Luft nutzt den schnellen Geburtsübergang.
	 */
	GENESISAUDIOCORE_API FGenesisHearingPerception SmoothPerception(const FGenesisHearingPerception& Current, const FGenesisHearingPerception& Target,
		float DeltaSeconds, const FGenesisHearingTuning& Tuning);

	/** Aktualisiert den Mix: Ducking nach Regeln und Wichtigkeit, stetige Übergänge über Zeitkonstanten. */
	GENESISAUDIOCORE_API void UpdateMix(FGenesisMixState& State, const TArray<FGenesisMixRequest>& ActiveRequests, const FGenesisAudioMixTuning& Tuning, float DeltaSeconds);

	GENESISAUDIOCORE_API float DbToLinear(float Db);

	/**
	 * Wiedergabe-Lautstärke für das, was das Ungeborene hört. Wie das Auge an die Lichtmenge passt sich das Ohr an seine
	 * Schwelle an: Was das Kind überhaupt hört, muss am Lautsprecher leise, aber wahrnehmbar sein (sonst wäre der erste
	 * Ton in SSW 19 nur Stille); die Abstufung bleibt erhalten. 0 bleibt 0.
	 */
	GENESISAUDIOCORE_API float UnbornPresentationGain(float Audibility);
}
