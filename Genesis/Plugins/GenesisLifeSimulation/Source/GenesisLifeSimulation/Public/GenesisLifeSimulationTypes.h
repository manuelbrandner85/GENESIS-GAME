// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisKarma.h"
#include "GenesisRandom.h"
#include "GenesisTypes.h"
#include "GenesisLifeSimulationTypes.generated.h"

/** Simulation Level of Detail einer Person. */
UENUM(BlueprintType)
enum class EGenesisSimulationLevel : uint8
{
	/** Level 1: volle Simulation (Personen beim Spieler). */
	Full,
	/** Level 2: reduzierte Simulation (wichtige entfernte NPCs) – nur bedeutsame Erinnerungen. */
	Reduced,
	/** Level 3: nur statistisch – keine Einzelverarbeitung. */
	Statistical
};

/** Glaubenshaltung. Bewusst neutral – keine Option ist besser. */
UENUM(BlueprintType)
enum class EGenesisBeliefStance : uint8
{
	Seeking,
	Religious,
	Spiritual,
	Agnostic,
	Atheist
};

/** Kulturelle Prägung einer Person. */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisCulturalIdentity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Culture")
	FGameplayTag Culture;

	/** 0..1 – wie stark die Normen verinnerlicht sind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Culture")
	float Adherence = 0.6f;

	/** Werte, die diese Kultur hochhält. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Culture")
	FGameplayTagContainer ValuedTags;

	/** Handlungen/Themen, die tabu sind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Culture")
	FGameplayTagContainer TabooTags;
};

/** Glaubensprofil. */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisBeliefProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Belief")
	EGenesisBeliefStance Stance = EGenesisBeliefStance::Seeking;

	/** Konkrete Tradition/Weltanschauung (Content-Tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Belief")
	FGameplayTag Tradition;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Belief")
	float Certainty = 0.3f;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Belief")
	float Openness = 0.5f;
};

/**
 * Lebensprofil einer simulierten Person.
 * Sichtbare Stammdaten sind Blueprint-editierbar; verborgene Simulationswerte (Karma, Spektren, Belastung) nicht.
 */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisLifeProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid EntityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid SoulId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid GenomeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	EGenesisSimulationLevel SimulationLevel = EGenesisSimulationLevel::Reduced;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGenesisCulturalIdentity Culture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGenesisBeliefProfile Belief;

	/** Gruppenzugehörigkeiten (Familie, Clique, Zunft …). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGameplayTagContainer Groups;

	// --- Verborgene Simulationswerte ---

	UPROPERTY()
	FGenesisKarmaProfile Karma;

	/** −1 Egoismus … +1 Altruismus. */
	UPROPERTY()
	float Altruism = 0.0f;

	/** 0..1 je Ideologie – verinnerlichte Indoktrination. */
	UPROPERTY()
	TArray<FGenesisWeightedTag> IdeologyExposure;

	/** 0..1 je Wert – öffentlich vertretene Werte (Grundlage des Doppelmoral-Detektors). */
	UPROPERTY()
	TArray<FGenesisWeightedTag> StatedValues;

	/** Aufgelaufene epigenetische Dosis je Pfad (wird jährlich ins Genom übertragen). */
	UPROPERTY()
	TArray<FGenesisWeightedTag> EpigeneticDose;

	/** 0..1 – akute Belastung (Brücke zu GenesisBody). */
	UPROPERTY()
	float Stress = 0.0f;

	/** 0..1 – innerer Widerspruch (Brücke zu GenesisMind). */
	UPROPERTY()
	float Dissonance = 0.0f;
};

/** Gerichtetes Vertrauen: From vertraut To. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisTrustEdge
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid FromId;

	UPROPERTY()
	FGuid ToId;

	/** −1 … +1 */
	UPROPERTY()
	float Trust = 0.0f;

	/** 0..1 – gemeinsame Geschichte. Senkt Missverständnisse. */
	UPROPERTY()
	float Familiarity = 0.0f;

	UPROPERTY()
	int32 Betrayals = 0;

	UPROPERTY()
	FGenesisTimestamp LastInteraction;

	UPROPERTY()
	FGuid LastEventId;
};

/** Was eine Person über eine andere glaubt – je Eigenschaft. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisReputationEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid SubjectId;

	UPROPERTY()
	FGuid HolderId;

	UPROPERTY()
	FGameplayTag Trait;

	/** −1 … +1 */
	UPROPERTY()
	float Score = 0.0f;

	/** 0..1 */
	UPROPERTY()
	float Confidence = 0.0f;
};

/** Gerücht: verbreitet sich über das Vertrauensnetz und verändert sich dabei. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisRumor
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid RumorId;

	UPROPERTY()
	FGuid SourceEventId;

	UPROPERTY()
	FGuid SubjectId;

	/** Behauptete Themen – können sich beim Weitererzählen verlieren. */
	UPROPERTY()
	FGameplayTagContainer ClaimThemes;

	/** Rufwirkung je Eigenschaft beim Glauben des Gerüchts. */
	UPROPERTY()
	TArray<FGenesisWeightedTag> ReputationEffects;

	/** 0..1 – Nähe zur Wahrheit. */
	UPROPERTY()
	float Truth = 1.0f;

	/** 0..1 – Antrieb zum Weitererzählen. Zerfällt mit der Zeit. */
	UPROPERTY()
	float Juiciness = 0.0f;

	UPROPERTY()
	TArray<FGuid> Holders;

	UPROPERTY()
	int32 Hops = 0;

	UPROPERTY()
	FGenesisTimestamp CreatedAt;

	UPROPERTY()
	FGenesisTimestamp LastSpreadAt;
};

/** Dominante Werte einer Epoche. */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisZeitgeistState
{
	GENERATED_BODY()

	/** 0..1 je Wert (Glaube, Ehre, Arbeit, Nation, Individualismus, Technologie …). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Zeitgeist")
	TArray<FGenesisWeightedTag> DominantValues;

	/** Gesammelter gesellschaftlicher Druck aus Handlungen – wird jährlich eingearbeitet. */
	UPROPERTY()
	TArray<FGenesisWeightedTag> PendingPressure;
};

/** Wen eine verzögerte Konsequenz betrifft. */
UENUM(BlueprintType)
enum class EGenesisConsequenceSubject : uint8
{
	Actor,
	Targets,
	Witnesses,
	ActorAndTargets
};

/** Definition einer verzögerten Folge (Authoring, Teil der Handlungsdefinition). */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisDelayedConsequenceSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence")
	FGameplayTag ConsequenceType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence")
	FGameplayTagContainer Themes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence")
	EGenesisConsequenceSubject Subject = EGenesisConsequenceSubject::Actor;

	/** Wahrscheinlichkeit (skaliert mit der Intensität der Handlung). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "0", ClampMax = "1"))
	float Probability = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "0"))
	float MinDelayYears = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "0"))
	float MaxDelayYears = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "0", ClampMax = "1"))
	float Magnitude = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "-1", ClampMax = "1"))
	float Valence = 0.0f;

	/** Nur, wenn die Handlung bemerkt wurde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence")
	bool bRequiresNoticed = false;

	/** Die Folge ist ein offener Faden. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence")
	bool bOpensThread = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Consequence", meta = (ClampMin = "0", ClampMax = "1"))
	float LinkStrength = 0.6f;
};

/** Eingeplante Folge. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisScheduledConsequence
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ConsequenceId;

	UPROPERTY()
	FGuid SourceEventId;

	UPROPERTY()
	FGuid SourceActorId;

	UPROPERTY()
	TArray<FGuid> SubjectIds;

	UPROPERTY()
	FGenesisDelayedConsequenceSpec Spec;

	UPROPERTY()
	FGenesisTimestamp DueTime;
};

/** Anwesende Gruppe bei einer Handlung (Gruppenzwang). */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisGroupContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Group")
	FGameplayTag GroupTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Group")
	TArray<FGuid> PresentMemberIds;

	/** Was die Gruppe erwartet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Group")
	FGameplayTagContainer ExpectedValues;

	/** Was die Gruppe ablehnt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Group")
	FGameplayTagContainer RejectedValues;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Group")
	float Cohesion = 0.5f;
};

/** Alle Stellschrauben der 14 Systeme (Project Settings → Genesis → Life Simulation). */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisLifeSimulationTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Karma")
	FGenesisKarmaTuning Karma;

	// --- Missverständnis ---
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float BaseMisreadChance = 0.08f;
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float AmbiguityMisreadWeight = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float CulturalDistanceMisreadWeight = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float StressMisreadWeight = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float FamiliarityMisreadReduction = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Misunderstanding") float MaxMisreadChance = 0.85f;

	// --- Indoktrination / Glaube / Propaganda ---
	UPROPERTY(EditAnywhere, Category = "Indoctrination") float IndoctrinationHabituation = 0.03f;
	UPROPERTY(EditAnywhere, Category = "Indoctrination") float IndoctrinationErosion = 0.03f;
	UPROPERTY(EditAnywhere, Category = "Belief") float BeliefShiftRate = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Propaganda") float PropagandaGain = 0.15f;

	// --- Zeitgeist / Gruppenzwang ---
	UPROPERTY(EditAnywhere, Category = "Zeitgeist") float ZeitgeistResistanceStress = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Zeitgeist") float ZeitgeistPressureGain = 0.01f;
	UPROPERTY(EditAnywhere, Category = "GroupPressure") float GroupDefianceCourage = 4.0f;
	UPROPERTY(EditAnywhere, Category = "GroupPressure") float GroupPressureStress = 0.15f;

	// --- Egoismus/Altruismus / Doppelmoral ---
	UPROPERTY(EditAnywhere, Category = "EgoismAltruism") float AltruismShift = 0.03f;
	UPROPERTY(EditAnywhere, Category = "DoubleStandard") float AdvocacyGain = 0.2f;

	// --- Vertrauen / Ruf / Gerüchte ---
	UPROPERTY(EditAnywhere, Category = "Trust") float TrustScale = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Trust") float WitnessTrustFactor = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Trust") float NegativityBias = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Trust") float BetrayalThreshold = -0.1f;
	UPROPERTY(EditAnywhere, Category = "Reputation") float ReputationScale = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Reputation") float WitnessReputationFactor = 0.6f;
	UPROPERTY(EditAnywhere, Category = "Rumors") float RumorMinJuiciness = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Rumors") float RumorSpreadRatePerDay = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Rumors") float RumorDistortionPerHop = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Rumors") float RumorHalfLifeDays = 30.0f;
	UPROPERTY(EditAnywhere, Category = "Rumors") int32 RumorMaxHoldersPerStep = 64;

	// --- Belastung / Epigenetik ---
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float StressGain = 0.2f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float StressRecoveryPerDay = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float DissonanceGain = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float DissonanceRecoveryPerDay = 0.01f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float StressDoseScale = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float EpigeneticPlasticity = 0.1f;
	UPROPERTY(EditAnywhere, Category = "Epigenetics") float EpigeneticReversionPerYear = 0.05f;

	// --- Konsequenz-Netzwerk / Simulation LOD ---
	UPROPERTY(EditAnywhere, Category = "Consequences") float AutoLinkWindowYears = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Consequences") int32 AutoLinkMaxLinks = 3;
	UPROPERTY(EditAnywhere, Category = "Consequences") float AutoLinkStrength = 0.35f;
	UPROPERTY(EditAnywhere, Category = "SimulationLOD") float ReducedEncodingMinMagnitude = 0.35f;
};

/** Gesamter persistenter Zustand der Lebenssimulation einer Welt. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisLifeSimulationState
{
	GENERATED_BODY()

	void Reset(uint64 WorldSeed);

	/** Muss nach dem Laden aufgerufen werden. */
	void RebuildIndices();

	FGenesisLifeProfile* FindProfile(const FGuid& EntityId);
	const FGenesisLifeProfile* FindProfile(const FGuid& EntityId) const;

	/** Fügt ein Profil ein oder ersetzt ein bestehendes (Achtung: kann Profil-Zeiger ungültig machen). */
	FGenesisLifeProfile& AddOrUpdateProfile(const FGenesisLifeProfile& Profile);

	FGenesisTrustEdge* FindTrust(const FGuid& FromId, const FGuid& ToId);
	const FGenesisTrustEdge* FindTrust(const FGuid& FromId, const FGuid& ToId) const;

	/** Achtung: kann Kanten-Zeiger ungültig machen. */
	FGenesisTrustEdge& FindOrAddTrust(const FGuid& FromId, const FGuid& ToId);

	float GetTrust(const FGuid& FromId, const FGuid& ToId) const;
	float GetFamiliarity(const FGuid& FromId, const FGuid& ToId) const;

	/** Achtung: kann Einträge-Zeiger ungültig machen. */
	FGenesisReputationEntry& FindOrAddReputation(const FGuid& SubjectId, const FGuid& HolderId, const FGameplayTag& Trait);
	float GetReputation(const FGuid& SubjectId, const FGuid& HolderId, const FGameplayTag& Trait) const;

	UPROPERTY()
	TArray<FGenesisLifeProfile> Profiles;

	UPROPERTY()
	TArray<FGenesisTrustEdge> TrustEdges;

	UPROPERTY()
	TArray<FGenesisReputationEntry> Reputation;

	UPROPERTY()
	TArray<FGenesisRumor> Rumors;

	UPROPERTY()
	TArray<FGenesisScheduledConsequence> PendingConsequences;

	UPROPERTY()
	FGenesisZeitgeistState Zeitgeist;

	UPROPERTY()
	FGenesisRandomStream Rng;

	/** Zeitpunkt der letzten Tages- und Jahresverarbeitung. */
	UPROPERTY()
	FGenesisTimestamp LastDailyUpdate;

	UPROPERTY()
	FGenesisTimestamp LastYearlyUpdate;

	UPROPERTY()
	bool bTimeInitialized = false;

private:
	TMap<FGuid, int32> ProfileIndex;
	TMap<FGuid, TArray<int32>> TrustByFrom;
	TMap<FGuid, TArray<int32>> ReputationBySubject;
};
