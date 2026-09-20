// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSliceTypes.h"

/**
 * Die Regeln des Durchlaufs – zustandslos, ohne Welt, ohne Karten.
 *
 * Damit lässt sich prüfen, was sonst nur im Spiel sichtbar wäre: dass keine Phase übersprungen wird,
 * dass ein abgestorbener Keim den Durchlauf ehrlich beendet und dass die erste Stunde ein Ende hat.
 */
namespace GenesisSliceLogic
{
	/** Die Phase, die auf die aktuelle folgt. Gleich, wenn noch gewartet wird. */
	GENESISSLICE_API EGenesisSlicePhase NextPhase(EGenesisSlicePhase Current, const FGenesisSliceSignals& Signals,
		const FGenesisSliceTuning& Tuning, EGenesisSliceEnding& OutEnding);

	GENESISSLICE_API FString GetPhaseName(EGenesisSlicePhase Phase);
	GENESISSLICE_API FString GetEndingName(EGenesisSliceEnding Ending);

	/** Was in dieser Phase zu sehen ist – ein Satz für Anzeige und Log. */
	GENESISSLICE_API FString GetPhaseDescription(EGenesisSlicePhase Phase);
}
