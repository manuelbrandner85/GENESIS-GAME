// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisBodyTypes.h"

struct FGenesisGenome;
struct FGenesisTraitDefinition;

/**
 * Zustandslose Körperlogik.
 *
 * Die Werte sind physiologisch plausibel angenähert (Größenordnungen, Verläufe), aber bewusst keine
 * medizinische Simulation. Ziel: glaubwürdig erlebbare Symptome und Lebensverläufe.
 */
namespace GenesisBodyLogic
{
	/** Körperanlagen aus dem Genom (fehlendes Genom → Durchschnittswerte). */
	GENESISBODY_API FGenesisBodyGenetics MakeGenetics(const FGenesisGenome* Genome, const TArray<FGenesisTraitDefinition>& Traits);

	/** Neuer Körper im Moment der Befruchtung. */
	GENESISBODY_API FGenesisBodyState CreateAtConception(const FGuid& EntityId, const FGuid& GenomeId, const FGenesisBodyGenetics& Genetics,
		const FGenesisConceptionVitality& Vitality, const FGenesisTimestamp& ConceptionTime, EGenesisSimulationLevel Level);

	GENESISBODY_API double GetGestationalWeeks(const FGenesisBodyState& Body, const FGenesisTimestamp& Now);

	/** Chronologisches Alter seit der Geburt (0 vor der Geburt). */
	GENESISBODY_API double GetAgeYears(const FGenesisBodyState& Body, const FGenesisTimestamp& Now);

	GENESISBODY_API EGenesisDevelopmentStage GetStage(const FGenesisBodyState& Body, const FGenesisTimestamp& Now);

	/** Geburt: erster Atemzug. Unreife Lungen führen zu Atemnot. */
	GENESISBODY_API void Birth(FGenesisBodyState& Body, const FGenesisTimestamp& Now, const FGenesisBodyTuning& Tuning);

	/**
	 * Stündliche Physiologie (nur Simulation Level 1): Hormone mit Tagesrhythmus, Vitalwerte, Schlafdruck, Hunger, Durst, Energie.
	 * @param End Zeitpunkt am Ende des Zeitraums
	 * @param PsychologicalStress 0..1 aus der Life Simulation
	 */
	GENESISBODY_API void AdvanceHours(FGenesisBodyState& Body, const FGenesisTimestamp& End, double Hours, float PsychologicalStress);

	/**
	 * Tägliche/langfristige Entwicklung (Level 1 und 2): Organ- und Sinnesentwicklung, Wachstum, Fitness, Schlafschuld,
	 * chronischer Stress, Zustände, Schäden, biologische Alterung.
	 * @return true, wenn in diesem Zeitraum die Vitalfunktionen versagt haben
	 */
	GENESISBODY_API bool AdvanceDays(FGenesisBodyState& Body, const FGenesisTimestamp& End, double Days, float PsychologicalStress, const FGenesisBodyTuning& Tuning);

	// --- Ereignisse aus dem Gameplay ---
	GENESISBODY_API void ApplyInjury(FGenesisBodyState& Body, const FGameplayTag& Region, float Severity, const FGenesisTimestamp& Now);
	GENESISBODY_API void ApplyIllness(FGenesisBodyState& Body, const FGameplayTag& Condition, float Severity, float ProgressionPerDay, float RecoveryPerDay,
		bool bChronic, bool bCausesFever, EGenesisOrgan AffectedOrgan, const FGenesisTimestamp& Now);
	GENESISBODY_API void ApplyAcuteStressor(FGenesisBodyState& Body, float Intensity);
	GENESISBODY_API void ApplyBonding(FGenesisBodyState& Body, float Intensity);
	GENESISBODY_API void Eat(FGenesisBodyState& Body, float Quality);
	GENESISBODY_API void Drink(FGenesisBodyState& Body);
	GENESISBODY_API void RecordExercise(FGenesisBodyState& Body, float Hours, float Intensity);

	/** Was die Person gerade körperlich erlebt (nur Symptome über 0,05). */
	GENESISBODY_API TArray<FGenesisSymptom> DeriveSymptoms(const FGenesisBodyState& Body, const FGenesisTimestamp& Now);

	GENESISBODY_API float GetSymptomIntensity(const TArray<FGenesisSymptom>& Symptoms, const FGameplayTag& Symptom);

	// --- Referenzwerte ---
	GENESISBODY_API float GetBaselineHeartRate(double AgeYears);
	GENESISBODY_API float GetBaselineRespiratoryRate(double AgeYears);
	GENESISBODY_API float GetRequiredSleepHours(double AgeYears);
}
