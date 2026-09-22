// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisEmbryogenesisTypes.h"

/**
 * Die dritte und vierte Woche als reine Rechenlogik (GENESIS-041), ohne Welt und ohne Actor.
 *
 * Alles rechnet sich aus dem Tag nach der Befruchtung, nicht Schritt für Schritt – dasselbe Ergebnis bei jeder
 * Schrittweite und nach dem Laden (wie in der zweiten Woche, Docs/30).
 */
namespace GenesisEmbryogenesisLogic
{
	/**
	 * Führt den Keim auf den Stand des angegebenen Tages nach der Befruchtung.
	 * NutritionQuality ist die Ernährung der Mutter (0..1) – sie entscheidet über das Risiko eines Neuralrohrdefekts.
	 * bPlayerEmbryo: Der Keim des Spielers bekommt keinen Defekt (sonst gäbe es kein Leben zu spielen).
	 * Liefert true, wenn sich die Stufe geändert hat.
	 */
	GENESISEMBRYO_API bool Advance(FGenesisEmbryogenesisState& State, const FGenesisEmbryogenesisTuning& Tuning,
		float DayAfterFertilisation, float NutritionQuality, bool bPlayerEmbryo, uint64 Seed);

	/** Herzfrequenz am Tag X (Schläge je Minute); 0, solange das Herz nicht schlägt. */
	GENESISEMBRYO_API float HeartRateAt(float DayAfterFertilisation, const FGenesisEmbryogenesisTuning& Tuning);

	/** Scheitel-Steiß-Länge (mm) am Tag X. */
	GENESISEMBRYO_API float LengthAt(float DayAfterFertilisation);

	/** Risiko eines Neuralrohrdefekts (0..1) bei dieser Ernährung der Mutter. */
	GENESISEMBRYO_API float DefectRiskFor(float NutritionQuality, const FGenesisEmbryogenesisTuning& Tuning);

	GENESISEMBRYO_API FString GetStageName(EGenesisEmbryogenesisStage Stage);
	/** Ein Satz für den Spieler: was in dieser Stufe geschieht. */
	GENESISEMBRYO_API FString DescribeStage(EGenesisEmbryogenesisStage Stage);
	GENESISEMBRYO_API FString GetDefectName(EGenesisNeuralTubeDefect Defect);
}
