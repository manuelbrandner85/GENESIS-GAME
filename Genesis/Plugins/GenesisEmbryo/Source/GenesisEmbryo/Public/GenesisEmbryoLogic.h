// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisEmbryoTypes.h"

/**
 * Die erste Woche als reine Rechenlogik – ohne Welt, ohne Actor, damit sie testbar bleibt.
 *
 * Grundsatz der Furchung: Der Keim wächst nicht. Aus einer Zelle von 110 µm werden viele kleine,
 * der Gesamtdurchmesser bleibt bis zum Schlüpfen derselbe. Erst die Blastozyste dehnt sich.
 */
namespace GenesisEmbryoLogic
{
	/** Legt die Zygote an: eine Zelle, deren erste Teilung gut einen Tag auf sich warten lässt. */
	GENESISEMBRYO_API FGenesisEmbryoState CreateZygote(const FGuid& EntityId, const FGuid& GenomeId,
		float Vitality, float Resilience, const FGenesisTimestamp& FusionTime, const FGenesisEmbryoTuning& Tuning);

	/** Führt den Keim um Stunden weiter. Liefert true, wenn sich die Entwicklungsstufe geändert hat. */
	GENESISEMBRYO_API bool Advance(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning, double Hours);

	/** 0..1 – Entwicklungsqualität, die in die Organbildung des Körpers eingeht. */
	GENESISEMBRYO_API float GetDevelopmentQuality(const FGenesisEmbryoState& State);

	/** Anzahl der Zellen des Embryoblasten (daraus entsteht der Mensch; der Rest wird Mutterkuchen). */
	GENESISEMBRYO_API int32 CountInnerCellMass(const FGenesisEmbryoState& State);

	GENESISEMBRYO_API FString GetStageName(EGenesisEmbryoStage Stage);
	GENESISEMBRYO_API FString GetArrestReasonName(EGenesisEmbryoArrestReason Reason);
}
