// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisEarlyLifeTypes.h"

/**
 * Die erste Stunde als reine Rechenlogik – ohne Welt, ohne Actor.
 *
 * Der Kern ist eine einzige Abhängigkeit: Ein Neugeborenes kann seine Wärme nicht halten.
 * Liegt es auf der Haut der Mutter, steigt alles andere mit – Ruhe, Bindung, die Bereitschaft
 * zu trinken. Liegt es allein, fällt zuerst die Temperatur und dann alles andere.
 */
namespace GenesisEarlyLifeLogic
{
	/** Beginnt die erste Stunde. Die Temperatur startet bei der Körpertemperatur der Mutter. */
	GENESISEARLYLIFE_API FGenesisNewbornState BeginNewborn(const FGuid& EntityId, const FGuid& MotherId, uint64 Seed,
		const FGenesisTimestamp& BirthTime);

	/** Führt die erste Stunde um Minuten weiter. Liefert true, wenn sich die Stufe geändert hat. */
	GENESISEARLYLIFE_API bool Advance(FGenesisNewbornState& State, const FGenesisEarlyLifeTuning& Tuning, double Minutes);

	GENESISEARLYLIFE_API FGenesisNewbornPerception GetPerception(const FGenesisNewbornState& State, const FGenesisEarlyLifeTuning& Tuning);

	GENESISEARLYLIFE_API FString GetStageName(EGenesisNewbornStage Stage);
}
