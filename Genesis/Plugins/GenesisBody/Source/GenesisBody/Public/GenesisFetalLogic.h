// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFetalTypes.h"

/**
 * Die Fetalzeit als reine Logik (GENESIS-044, Docs/34): Was eine Schwangerschaftswoche für Körper und Sinne bedeutet,
 * und was das Kind in einem Moment tut. Alle Wochen in SSW; die Weltuhr rechnet ab der Befruchtung.
 */
namespace GenesisFetalLogic
{
	/** SSW aus Wochen seit der Befruchtung. */
	constexpr double WeeksFromConceptionToGestational = 2.0;

	/** Die Referenz aus den Projekteinstellungen (in Tests: die eingebauten Werte). */
	GENESISBODY_API const FGenesisFetalReference& GetReference();

	/** Körper und Sinne in einer SSW. */
	GENESISBODY_API FGenesisFetalView Evaluate(const FGenesisFetalReference& Reference, float GestationalWeeks);

	/** Mittlere Dauer eines Zustands (s) in dieser SSW – daraus ergeben sich die Anteile über den Tag. */
	GENESISBODY_API float MeanStateSeconds(EGenesisFetalState State, float GestationalWeeks, const FGenesisFetalMilestones& Milestones);

	/**
	 * Führt das Verhalten um DeltaSeconds weiter und gibt die Ereignisse dieses Zeitraums zurück.
	 * MaternalMovement (0..1): Gehen wiegt das Kind in Ruhe.
	 */
	GENESISBODY_API TArray<EGenesisFetalEvent> AdvanceBehaviour(FGenesisFetalBehaviour& Behaviour, const FGenesisFetalView& View,
		const FGenesisFetalMilestones& Milestones, float DeltaSeconds, float MaternalMovement, FRandomStream& Random);

	/** Ab welcher SSW der Körper eine Handlung kann (Docs/37). */
	GENESISBODY_API float ActionOnsetWeeks(EGenesisFetalAction Action, const FGenesisFetalMilestones& Milestones);

	/** Kann das Kind das in dieser SSW? */
	GENESISBODY_API bool CanPerform(EGenesisFetalAction Action, float GestationalWeeks, const FGenesisFetalMilestones& Milestones);

	/**
	 * Platz zum Bewegen (0..1): bis zum letzten Drittel schwebt das Kind frei, dann füllt es die Höhle aus – am Termin
	 * bleiben kleine Bewegungen, Strecken und Drehen werden schwer.
	 */
	GENESISBODY_API float RoomToMove(float GestationalWeeks, const FGenesisFetalMilestones& Milestones);

	/** Name einer Handlung für Anzeige und Protokoll. */
	GENESISBODY_API FString GetActionName(EGenesisFetalAction Action);

	/** Name eines Zustands für Anzeige und Protokoll. */
	GENESISBODY_API FString GetStateName(EGenesisFetalState State, float GestationalWeeks, const FGenesisFetalMilestones& Milestones);
}
