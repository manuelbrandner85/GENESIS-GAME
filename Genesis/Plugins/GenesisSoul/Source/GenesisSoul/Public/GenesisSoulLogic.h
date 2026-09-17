// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSoulTypes.h"

/**
 * Zustandslose Logik der Soul Engine.
 * Alle Funktionen arbeiten auf FGenesisSoulSeed und sind deterministisch (Zufall aus OriginSeed abgeleitet).
 */
namespace GenesisSoulLogic
{
	/**
	 * Erzeugt eine neue Seele.
	 * @param InnatePatternPool Mögliche angeborene Resonanzen (Content). 1–2 davon werden schwach angelegt.
	 */
	GENESISSOUL_API FGenesisSoulSeed CreateSoulSeed(uint64 OriginSeed, const TArray<FGameplayTag>& InnatePatternPool);

	/** Beginnt eine neue Inkarnation. Schlägt fehl (nullptr), wenn die aktuelle noch nicht abgeschlossen ist. */
	GENESISSOUL_API FGenesisIncarnationRecord* BeginIncarnation(FGenesisSoulSeed& Soul, const FGenesisIncarnationRecord& Template);

	/** Verstärkt ein Muster (Amount 0..1). Legt es bei Bedarf an. Sättigt gegen 1. */
	GENESISSOUL_API void ReinforceResonance(FGenesisSoulSeed& Soul, const FGameplayTag& Pattern, float Amount, const FGenesisSoulCarryOverParams& Params);

	/**
	 * Schließt die aktuelle Inkarnation ab und überträgt das Leben auf die Seele:
	 * Resonanzen verstärken/verblassen, Karma-Gericht-Themen → Echos, Bindungen, Motiv-Entwicklung, Loslassen.
	 * @return false, wenn keine offene Inkarnation existiert.
	 */
	GENESISSOUL_API bool CloseIncarnation(FGenesisSoulSeed& Soul, const FGenesisLifeClosure& Closure, const FGenesisSoulCarryOverParams& Params);

	/** Höchste Intensität aller Resonanzen, deren Muster PatternQuery entspricht (hierarchisch). */
	GENESISSOUL_API float GetResonanceIntensity(const FGenesisSoulSeed& Soul, const FGameplayTag& PatternQuery);

	/**
	 * Wie stark eine Situation mit den Echos der Seele schwingt (0..1).
	 * Der Living World Director nutzt das, um thematische Echo-Situationen zu erzeugen – nie als Strafe.
	 * Offene Themen ziehen stärker als integrierte.
	 */
	GENESISSOUL_API float ComputeEchoPull(const FGenesisSoulSeed& Soul, const FGameplayTagContainer& SituationThemes);

	/**
	 * Wiedererkennungsstärke einer anderen Seele (0..1) – Grundlage für Déjà-vu, Musik, Traum, Gefühl.
	 * Es gibt keine direkte Kennzeichnung im Spiel; nur diese Stärke steuert subtile Signale.
	 */
	GENESISSOUL_API float ComputeRecognition(const FGenesisSoulSeed& Soul, const FGuid& OtherSoulId, const FGameplayTagContainer& ContextThemes);

	GENESISSOUL_API const FGenesisSoulEcho* FindEcho(const FGenesisSoulSeed& Soul, const FGameplayTag& Theme);
	GENESISSOUL_API const FGenesisSoulBond* FindBond(const FGenesisSoulSeed& Soul, const FGuid& OtherSoulId);
}
