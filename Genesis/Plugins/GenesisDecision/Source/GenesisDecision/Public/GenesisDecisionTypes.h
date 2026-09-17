// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisLifeAction.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisTypes.h"
#include "GenesisDecisionTypes.generated.h"

struct FGenesisMemoryWorld;
struct FGenesisSoulSeed;

/** Eine Wahlmöglichkeit. */
USTRUCT(BlueprintType)
struct GENESISDECISION_API FGenesisDecisionOption
{
	GENERATED_BODY()

	/** Anzeigetext für die Spieler-UI (neutral formuliert, ohne Wertung). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	TObjectPtr<UGenesisLifeActionDefinition> Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	TArray<FGuid> TargetIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision", meta = (ClampMin = "0", ClampMax = "2"))
	float Intensity = 1.0f;
};

/** Eine Entscheidungssituation – für Spieler und NPCs identisch aufgebaut. */
USTRUCT(BlueprintType)
struct GENESISDECISION_API FGenesisDecisionSituation
{
	GENERATED_BODY()

	/** Wer entscheidet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	FGuid DeciderId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	TArray<FGenesisDecisionOption> Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	TArray<FGuid> WitnessIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	TArray<FGenesisGroupContext> Groups;

	/** Themen der Situation selbst (für Erinnerungs- und Echo-Resonanz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision", meta = (Categories = "Genesis.Theme"))
	FGameplayTagContainer SituationThemes;

	/** Ereignis, auf das reagiert wird. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision")
	FGuid CausedByEventId;

	/** 0 = viel Zeit zum Nachdenken … 1 = sofort reagieren. Unterbewusstsein gewinnt an Gewicht. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision", meta = (ClampMin = "0", ClampMax = "1"))
	float TimePressure = 0.0f;

	/** 0..1 – Tragweite. Hohe Tragweite = überlegtere, weniger zufällige Wahl. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Decision", meta = (ClampMin = "0", ClampMax = "1"))
	float Stakes = 0.5f;
};

/** Beitrag einer Consideration (nur Developer HUD / Tests – enthält indirekt verborgene Werte). */
USTRUCT()
struct GENESISDECISION_API FGenesisConsiderationScore
{
	GENERATED_BODY()

	FName Consideration;
	float Score = 0.0f;
	float Weight = 0.0f;
	bool bSubconscious = false;
};

/** Bewertung einer Option. */
USTRUCT()
struct GENESISDECISION_API FGenesisOptionEvaluation
{
	GENERATED_BODY()

	int32 OptionIndex = INDEX_NONE;
	bool bValid = false;

	TArray<FGenesisConsiderationScore> Breakdown;

	/** −1..1 – bewusste Abwägung. */
	float Conscious = 0.0f;

	/** −1..1 – unterbewusster Zug. */
	float Subconscious = 0.0f;

	/** −1..1 – Bauchgefühl (Unterbewusstsein + Charakter). */
	float Impulse = 0.0f;

	/** Gesamtbewertung nach Zeitdruck und Stress. */
	float Total = 0.0f;

	/** Wahrscheinlichkeit, dass ein NPC diese Option wählt. */
	float Probability = 0.0f;

	/** Ereignisse (Erinnerungen), die diese Option geprägt haben. */
	TArray<FGuid> InfluenceEventIds;
};

/** Ergebnis einer Entscheidung. */
USTRUCT(BlueprintType)
struct GENESISDECISION_API FGenesisDecisionResult
{
	GENERATED_BODY()

	/** Gewählte Option (INDEX_NONE = noch nicht entschieden). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	int32 ChosenIndex = INDEX_NONE;

	/** Option, zu der das Bauchgefühl drängt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	int32 ImpulseIndex = INDEX_NONE;

	/** Option mit der besten bewussten Abwägung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	int32 ConsciousBestIndex = INDEX_NONE;

	/** 0..1 – Zögern (knappe Abwägung). Für Körpersprache, Kamera, Audio. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	float Hesitation = 0.0f;

	/** 0..1 – Kopf und Bauch widersprechen sich. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	float InnerConflict = 0.0f;

	/** true, wenn die Wahl dem Impuls entsprach. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	bool bFollowedImpulse = false;

	/** Ereignis der ausgeführten Handlung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Decision")
	FGuid EventId;

	/** Detailbewertung (nicht für Spiel-UI). */
	TArray<FGenesisOptionEvaluation> Evaluations;
};

/** Stellschrauben der Entscheidungsfindung. */
USTRUCT(BlueprintType)
struct GENESISDECISION_API FGenesisDecisionTuning
{
	GENERATED_BODY()

	/** Grundzufälligkeit der Wahl (Softmax-Temperatur). */
	UPROPERTY(EditAnywhere, Category = "Choice") float BaseTemperature = 0.12f;
	UPROPERTY(EditAnywhere, Category = "Choice") float StressTemperature = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Choice") float LowStakesTemperature = 0.1f;

	/** Verlust an Überlegung durch Zeitdruck bzw. Stress. */
	UPROPERTY(EditAnywhere, Category = "Deliberation") float TimePressureDeliberationLoss = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Deliberation") float StressDeliberationLoss = 0.3f;
	UPROPERTY(EditAnywhere, Category = "Deliberation") float MinDeliberation = 0.2f;

	/** Anteil, mit dem das Unterbewusstsein immer mitwirkt – auch mit viel Zeit. */
	UPROPERTY(EditAnywhere, Category = "Deliberation") float SubconsciousFloor = 0.2f;

	/** Gewicht des Charakters im Bauchgefühl. */
	UPROPERTY(EditAnywhere, Category = "Impulse") float CharacterInImpulse = 0.4f;

	/** Maximal verknüpfte prägende Erinnerungen pro Entscheidung. */
	UPROPERTY(EditAnywhere, Category = "Causality") int32 MaxInfluenceEvents = 3;
};

/** Eingaben einer Bewertung (alle optional außer State). */
struct GENESISDECISION_API FGenesisDecisionInputs
{
	const FGenesisLifeSimulationState* State = nullptr;
	const FGenesisMemoryWorld* Memory = nullptr;
	const FGenesisSoulSeed* DeciderSoul = nullptr;
	FGenesisTimestamp Now;
};
