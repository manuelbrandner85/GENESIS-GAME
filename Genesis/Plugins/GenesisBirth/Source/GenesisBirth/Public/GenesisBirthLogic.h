// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisBirthTypes.h"

/**
 * Die Geburt als reine Rechenlogik – ohne Welt, ohne Actor.
 *
 * Alles hier ist aus der Sicht des Kindes gerechnet: Wehen sind Druckwellen mit Sauerstoffeinbrüchen,
 * Fortschritt ist Öffnung und Tiefertreten, das Ergebnis ist der Zustand in der ersten Minute.
 */
namespace GenesisBirthLogic
{
	/** Beginnt die Geburt. Schwangerschaftswochen und Lungenreife kommen aus der Körpersimulation. */
	GENESISBIRTH_API FGenesisBirthState BeginLabor(const FGuid& EntityId, uint64 Seed, float GestationalWeeks,
		float LungMaturity, const FGenesisTimestamp& Now, const FGenesisBirthTuning& Tuning);

	/** Führt die Geburt um Minuten weiter. Liefert true, wenn sich der Abschnitt geändert hat. */
	GENESISBIRTH_API bool Advance(FGenesisBirthState& State, const FGenesisBirthTuning& Tuning, double Minutes);

	/** Was das Kind in diesem Moment erlebt. */
	GENESISBIRTH_API FGenesisBirthPerception GetPerception(const FGenesisBirthState& State);

	/** Erstes Zustandsbild (0–10) aus Sauerstoff, Belastung, Reife und Atmung. */
	GENESISBIRTH_API int32 ComputeApgar(const FGenesisBirthState& State);

	GENESISBIRTH_API FString GetStageName(EGenesisLaborStage Stage);
	GENESISBIRTH_API FString GetComplicationName(EGenesisBirthComplication Complication);
}
