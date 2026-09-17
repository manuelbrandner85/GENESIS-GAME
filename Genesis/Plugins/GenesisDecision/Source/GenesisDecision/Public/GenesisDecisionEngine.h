// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenesisDecisionTypes.h"
#include "GenesisDecisionEngine.generated.h"

class UGenesisDecisionConsideration;
struct FGenesisRandomStream;

/**
 * Decision Engine – bewertet Entscheidungssituationen aus Sicht einer Person.
 *
 * Bewusste Abwägung (Charakter, Motiv, Überzeugung, Kultur, Umfeld, Beziehung, Erfahrung)
 * + Unterbewusstsein (Verdrängtes, Stress, Seelen-Echos) → Gesamtbewertung, Bauchgefühl, Zögern, innerer Konflikt.
 *
 * Unabhängig von GameInstance – vollständig testbar. Keine moralische Bewertung: Die Engine bildet ab,
 * wie DIESE Person entscheiden würde, nicht wie "richtig" entschieden wird.
 */
UCLASS()
class GENESISDECISION_API UGenesisDecisionEngine : public UObject
{
	GENERATED_BODY()

public:
	void AddDefaultConsiderations();
	void AddConsideration(UGenesisDecisionConsideration* Consideration);
	const TArray<TObjectPtr<UGenesisDecisionConsideration>>& GetConsiderations() const { return Considerations; }

	void SetTuning(const FGenesisDecisionTuning& InTuning) { Tuning = InTuning; }
	const FGenesisDecisionTuning& GetTuning() const { return Tuning; }

	/** Bewertet alle Optionen (deterministisch, ohne Wahl). */
	FGenesisDecisionResult Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionInputs& Inputs) const;

	/** NPC-Wahl: gewichtete Zufallsauswahl nach den Wahrscheinlichkeiten. Setzt ChosenIndex und bFollowedImpulse. */
	void ChooseForNpc(FGenesisDecisionResult& Result, FGenesisRandomStream& Rng) const;

	/** Spielerwahl übernehmen. Ungültiger Index (z. B. Zeit abgelaufen) → der Impuls entscheidet. */
	static void ApplyPlayerChoice(FGenesisDecisionResult& Result, int32 ChosenIndex);

	/** Baut aus der gewählten Option die Handlung für die Life Simulation (inkl. prägender Erinnerungen als Einflüsse). */
	static FGenesisLifeAction BuildAction(const FGenesisDecisionSituation& Situation, const FGenesisDecisionResult& Result);

private:
	UPROPERTY()
	TArray<TObjectPtr<UGenesisDecisionConsideration>> Considerations;

	UPROPERTY()
	FGenesisDecisionTuning Tuning;
};
