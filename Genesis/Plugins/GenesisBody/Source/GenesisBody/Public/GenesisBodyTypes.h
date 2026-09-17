// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisTypes.h"
#include "GenesisBodyTypes.generated.h"

/** Organsysteme. */
UENUM(BlueprintType)
enum class EGenesisOrgan : uint8
{
	Heart,
	Lungs,
	Liver,
	Brain,
	Immune,
	Musculature,
	Skeleton,
	Nervous,
	Metabolism
};

static constexpr int32 GenesisOrganCount = 9;

/** Die fünf Sinne. */
UENUM(BlueprintType)
enum class EGenesisBodySense : uint8
{
	Sight,
	Hearing,
	Smell,
	Taste,
	Touch
};

static constexpr int32 GenesisSenseCount = 5;

/** Aktuelle körperliche Aktivität (setzt Gameplay/Animation). */
UENUM(BlueprintType)
enum class EGenesisActivity : uint8
{
	Sleep,
	Rest,
	Light,
	Moderate,
	Intense
};

/** Entwicklungsstufe – steuert u. a. UI-Phasen und Wahrnehmung. */
UENUM(BlueprintType)
enum class EGenesisDevelopmentStage : uint8
{
	Zygote,
	Embryo,
	Fetus,
	Newborn,
	Infant,
	Child,
	Adolescent,
	Adult,
	Elder,
	Deceased
};

/** Zustand eines Organsystems. */
USTRUCT()
struct GENESISBODY_API FGenesisOrganState
{
	GENERATED_BODY()

	/** 0..1 – strukturelle Ausbildung (pränatal, bei Frühgeborenen nachreifend). */
	UPROPERTY()
	float Development = 0.0f;

	/** 0..1 – funktionelle Reserve (steigt bis zum Erwachsenenalter, sinkt im Alter). */
	UPROPERTY()
	float Capacity = 0.0f;

	/** 0..1 – heilbarer Schaden durch Verletzung und Krankheit. */
	UPROPERTY()
	float Damage = 0.0f;

	/** 0..1 – dauerhafter Verschleiß (z. B. Herz durch jahrelangen Stress). Heilt nicht. */
	UPROPERTY()
	float Wear = 0.0f;

	/** 0..1 – Qualität der Entwicklung (Embryo-Phase beeinflusst sie). */
	UPROPERTY()
	float DevelopmentQuality = 1.0f;

	/** Gesamtzustand 0..1. */
	float GetHealth() const { return Development * Capacity * (1.0f - Damage) * (1.0f - Wear); }

	/** Funktion im Alltag – ein gesundes Kinderherz ist nicht "schwach", nur noch nicht belastbar. */
	float GetFunction() const { return Development * (1.0f - Damage) * (1.0f - Wear) * (0.5f + 0.5f * Capacity); }
};

/** Zustand eines Sinnes. */
USTRUCT()
struct GENESISBODY_API FGenesisSenseState
{
	GENERATED_BODY()

	/** 0..1 – strukturelle Ausbildung. */
	UPROPERTY()
	float Development = 0.0f;

	/** 0..1 – aktuelle Schärfe (reift nach der Geburt, sinkt im Alter). */
	UPROPERTY()
	float Acuity = 0.0f;
};

/** Hormone, normiert 0..1. */
USTRUCT()
struct GENESISBODY_API FGenesisHormoneState
{
	GENERATED_BODY()

	UPROPERTY() float Cortisol = 0.15f;
	UPROPERTY() float Adrenaline = 0.0f;
	UPROPERTY() float Oxytocin = 0.1f;
	UPROPERTY() float Melatonin = 0.2f;
	UPROPERTY() float GrowthHormone = 0.3f;
	UPROPERTY() float SexHormones = 0.0f;
};

/** Vitalwerte in realen Einheiten bzw. normiert. */
USTRUCT()
struct GENESISBODY_API FGenesisVitalState
{
	GENERATED_BODY()

	/** Schläge pro Minute. */
	UPROPERTY() float HeartRate = 0.0f;
	/** Atemzüge pro Minute. */
	UPROPERTY() float RespiratoryRate = 0.0f;
	/** °C */
	UPROPERTY() float BodyTemperature = 36.8f;
	/** % */
	UPROPERTY() float BloodOxygen = 98.0f;
	/** 0..1 */
	UPROPERTY() float Energy = 1.0f;
	/** 0..1.2 – Schlafdruck. */
	UPROPERTY() float SleepPressure = 0.0f;
	/** 0..1 */
	UPROPERTY() float Hunger = 0.0f;
	/** 0..1 */
	UPROPERTY() float Hydration = 1.0f;
};

/** Verletzung oder Krankheit. */
USTRUCT()
struct GENESISBODY_API FGenesisBodyCondition
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTag Condition;
	UPROPERTY() FGameplayTag Region;
	UPROPERTY() EGenesisOrgan AffectedOrgan = EGenesisOrgan::Immune;
	/** 0..1 */
	UPROPERTY() float Severity = 0.0f;
	UPROPERTY() float PeakSeverity = 0.0f;
	/** Verschlechterung pro Tag (vor Immunantwort). */
	UPROPERTY() float ProgressionPerDay = 0.0f;
	/** Heilung pro Tag bei voller Immunfunktion. */
	UPROPERTY() float RecoveryPerDay = 0.05f;
	UPROPERTY() bool bChronic = false;
	UPROPERTY() bool bCausesFever = false;
	UPROPERTY() bool bCausesPain = false;
	UPROPERTY() bool bCanScar = false;
	UPROPERTY() FGenesisTimestamp Onset;
};

/** Sichtbare Narbe – physische Erinnerung. */
USTRUCT()
struct GENESISBODY_API FGenesisScar
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTag Region;
	/** 0..1 */
	UPROPERTY() float Visibility = 0.0f;
	UPROPERTY() FGenesisTimestamp Acquired;
};

/** Aus dem Genom abgeleitete Körperanlagen (bei Zeugung festgelegt). */
USTRUCT()
struct GENESISBODY_API FGenesisBodyGenetics
{
	GENERATED_BODY()

	UPROPERTY() float AdultHeightCm = 172.0f;
	/** 0..1 */
	UPROPERTY() float MuscleBase = 0.5f;
	/** 0..1 */
	UPROPERTY() float CardioRisk = 0.3f;
	/** 0..1 */
	UPROPERTY() float MetabolismRate = 0.5f;
	/** 0..1 */
	UPROPERTY() float SmellAcuity = 0.5f;
};

/** Basisparameter des ersten Körpers aus dem Spermium-Prolog (keine DNA-Änderung). */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisConceptionVitality
{
	GENERATED_BODY()

	/** 0..1 – allgemeine Lebenskraft. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Body")
	float Vitality = 0.5f;

	/** 0..1 – Widerstandskraft (Immunsystem, Heilung). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Body")
	float Resilience = 0.5f;
};

/** Ein erlebbares Symptom. */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisSymptom
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Body")
	FGameplayTag Symptom;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Body")
	float Intensity = 0.0f;
};

/** Vollständiger Körperzustand einer Person. */
USTRUCT()
struct GENESISBODY_API FGenesisBodyState
{
	GENERATED_BODY()

	UPROPERTY() FGuid EntityId;
	UPROPERTY() FGuid GenomeId;
	UPROPERTY() EGenesisSimulationLevel SimulationLevel = EGenesisSimulationLevel::Full;

	UPROPERTY() FGenesisTimestamp ConceptionTime;
	UPROPERTY() FGenesisTimestamp BirthTime;
	UPROPERTY() FGenesisTimestamp DeathTime;
	UPROPERTY() bool bBorn = false;
	UPROPERTY() bool bAlive = true;
	/** Schwangerschaftswoche bei der Geburt (Frühgeborene reifen nach). */
	UPROPERTY() float GestationalWeeksAtBirth = 0.0f;

	UPROPERTY() FGenesisConceptionVitality Vitality;
	UPROPERTY() FGenesisBodyGenetics Genetics;

	/** Indiziert über EGenesisOrgan. */
	UPROPERTY() TArray<FGenesisOrganState> Organs;
	/** Indiziert über EGenesisBodySense. */
	UPROPERTY() TArray<FGenesisSenseState> Senses;

	UPROPERTY() FGenesisHormoneState Hormones;
	UPROPERTY() FGenesisVitalState Vitals;
	UPROPERTY() EGenesisActivity Activity = EGenesisActivity::Rest;

	// Körperbau und Lebensstil
	UPROPERTY() float HeightCm = 0.0f;
	UPROPERTY() float WeightKg = 0.0f;
	UPROPERTY() float BodyFat = 0.18f;
	UPROPERTY() float Fitness = 0.3f;
	UPROPERTY() float TrainingLoad = 0.0f;
	UPROPERTY() float NutritionQuality = 0.7f;
	UPROPERTY() float SleepDebtHours = 0.0f;
	UPROPERTY() float SleptHoursToday = 0.0f;
	UPROPERTY() float ChronicStress = 0.0f;

	/** Biologisches Alter in Jahren – kann schneller oder langsamer laufen als die Zeit. */
	UPROPERTY() double BiologicalAgeYears = 0.0;

	/** Stunden seit dem letzten Tagesschritt. */
	UPROPERTY() float HoursSinceDailyUpdate = 0.0f;

	UPROPERTY() TArray<FGenesisBodyCondition> Conditions;
	UPROPERTY() TArray<FGenesisScar> Scars;

	FGenesisOrganState& Organ(EGenesisOrgan Type) { return Organs[static_cast<int32>(Type)]; }
	const FGenesisOrganState& Organ(EGenesisOrgan Type) const { return Organs[static_cast<int32>(Type)]; }
	FGenesisSenseState& Sense(EGenesisBodySense Type) { return Senses[static_cast<int32>(Type)]; }
	const FGenesisSenseState& Sense(EGenesisBodySense Type) const { return Senses[static_cast<int32>(Type)]; }
};

/** Stellschrauben der Körpersimulation. */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisBodyTuning
{
	GENERATED_BODY()

	/** Beschleunigung der biologischen Alterung durch chronischen Stress. */
	UPROPERTY(EditAnywhere, Category = "Aging") float StressAgingFactor = 0.5f;
	/** … durch Schlafschuld (bei 40 h Schuld voll). */
	UPROPERTY(EditAnywhere, Category = "Aging") float SleepDebtAgingFactor = 0.3f;
	/** Verlangsamung durch Fitness (nur Erwachsene). */
	UPROPERTY(EditAnywhere, Category = "Aging") float FitnessAgingReduction = 0.2f;

	/** Dauerhafter Herzverschleiß pro Tag bei maximalem chronischem Stress. */
	UPROPERTY(EditAnywhere, Category = "Damage") float StressHeartWearPerDay = 0.00004f;
	/** Reparatur nicht-chronischer Schäden pro Tag bei voller Immunfunktion. */
	UPROPERTY(EditAnywhere, Category = "Damage") float DamageRepairPerDay = 0.001f;

	/** Unter dieser Organgesundheit (Herz, Gehirn, Lunge) versagen die Vitalfunktionen. */
	UPROPERTY(EditAnywhere, Category = "Vital") float VitalFailureThreshold = 0.03f;

	/** Lungenreife, ab der ein Neugeborenes ohne Atemnot atmet. */
	UPROPERTY(EditAnywhere, Category = "Birth") float LungMaturityForBirth = 0.85f;
};
