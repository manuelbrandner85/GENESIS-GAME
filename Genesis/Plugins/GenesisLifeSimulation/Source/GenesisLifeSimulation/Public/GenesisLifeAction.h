// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GenesisKarma.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisLifeAction.generated.h"

/**
 * Definition einer Handlung (Data Asset, Präfix DA_Action_).
 *
 * Designer beschreiben, WAS eine Handlung ausdrückt – nicht, was sie "wert" ist.
 * Die 14 Systeme entscheiden im Kontext (Kultur, Zeugen, Gruppe, Vorgeschichte), was daraus folgt.
 */
UCLASS(BlueprintType)
class GENESISLIFESIMULATION_API UGenesisLifeActionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("GenesisLifeAction"), GetFName());
	}

	/** Handlungstyp + Themen als ein Container (für Tabu- und Normprüfungen). */
	FGameplayTagContainer GetActionTags() const
	{
		FGameplayTagContainer Tags = Themes;
		if (ActionType.IsValid())
		{
			Tags.AddTag(ActionType);
		}
		return Tags;
	}

	// --- Identität ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FGameplayTag ActionType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Genesis.Theme"))
	FGameplayTagContainer Themes;

	/** Objektive Tragweite 0..1. */
	UPROPERTY(EditAnywhere, Category = "Identity", meta = (ClampMin = "0", ClampMax = "1"))
	float BaseMagnitude = 0.3f;

	/** Emotionale Färbung für Betroffene −1..1. */
	UPROPERTY(EditAnywhere, Category = "Identity", meta = (ClampMin = "-1", ClampMax = "1"))
	float Valence = 0.0f;

	/** Die Handlung hinterlässt einen offenen Faden. */
	UPROPERTY(EditAnywhere, Category = "Identity")
	bool bOpensUnresolvedThread = false;

	// --- Karma (verborgen, intentionsbasiert) ---

	UPROPERTY(EditAnywhere, Category = "Karma")
	FGenesisKarmaVector KarmaImpulse;

	// --- Motiv ---

	/** Nutzen für sich selbst −1..1. */
	UPROPERTY(EditAnywhere, Category = "Motive", meta = (ClampMin = "-1", ClampMax = "1"))
	float SelfBenefit = 0.0f;

	/** Nutzen für andere −1..1. */
	UPROPERTY(EditAnywhere, Category = "Motive", meta = (ClampMin = "-1", ClampMax = "1"))
	float OthersBenefit = 0.0f;

	/** Eigene Kosten 0..1 (Opfer). */
	UPROPERTY(EditAnywhere, Category = "Motive", meta = (ClampMin = "0", ClampMax = "1"))
	float CostToSelf = 0.0f;

	// --- Soziale Wahrnehmung ---

	/** Wirkung auf das Vertrauen der Betroffenen, wenn richtig verstanden −1..1. */
	UPROPERTY(EditAnywhere, Category = "Social", meta = (ClampMin = "-1", ClampMax = "1"))
	float TrustImpact = 0.0f;

	/** Wie leicht die Handlung missverstanden wird 0..1. */
	UPROPERTY(EditAnywhere, Category = "Social", meta = (ClampMin = "0", ClampMax = "1"))
	float Ambiguity = 0.2f;

	/** Wie auffällig die Handlung ist 0..1 (heimliche Handlungen niedrig). */
	UPROPERTY(EditAnywhere, Category = "Social", meta = (ClampMin = "0", ClampMax = "1"))
	float Visibility = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Social", meta = (Categories = "Genesis.Theme"))
	FGameplayTagContainer ExpressesValues;

	UPROPERTY(EditAnywhere, Category = "Social", meta = (Categories = "Genesis.Theme"))
	FGameplayTagContainer ViolatesValues;

	/** Die Person vertritt ExpressesValues öffentlich (Rede, Urteil über andere). */
	UPROPERTY(EditAnywhere, Category = "Social")
	bool bIsAdvocacy = false;

	// --- Ideologie, Glaube, Propaganda ---

	UPROPERTY(EditAnywhere, Category = "Ideology")
	FGameplayTagContainer IdeologiesAligned;

	UPROPERTY(EditAnywhere, Category = "Ideology")
	FGameplayTagContainer IdeologiesContradicted;

	/** −1..1 – Ritual (+) oder Zweifel (−). */
	UPROPERTY(EditAnywhere, Category = "Belief", meta = (ClampMin = "-1", ClampMax = "1"))
	float BeliefCertaintyImpulse = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Belief", meta = (ClampMin = "-1", ClampMax = "1"))
	float BeliefOpennessImpulse = 0.0f;

	/** Ideologie, die diese Handlung bei anderen verbreitet. */
	UPROPERTY(EditAnywhere, Category = "Propaganda")
	FGameplayTag PropagatedIdeology;

	UPROPERTY(EditAnywhere, Category = "Propaganda", meta = (ClampMin = "0", ClampMax = "1"))
	float ManipulationStrength = 0.0f;

	// --- Körper ---

	/** Belastung für den Handelnden 0..1. */
	UPROPERTY(EditAnywhere, Category = "Body", meta = (ClampMin = "0", ClampMax = "1"))
	float StressLoad = 0.0f;

	/** Direkte epigenetische Exposition (Ernährung, Bewegung …) je Pfad. */
	UPROPERTY(EditAnywhere, Category = "Body")
	TArray<FGenesisWeightedTag> EpigeneticExposure;

	// --- Verzögerte Folgen ---

	UPROPERTY(EditAnywhere, Category = "Consequences")
	TArray<FGenesisDelayedConsequenceSpec> DelayedConsequences;
};

/** Eine konkrete Handlung im Spiel (von Spieler, NPC oder Director gemeldet). */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisLifeAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	TObjectPtr<UGenesisLifeActionDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid ActorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	TArray<FGuid> TargetIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	TArray<FGuid> WitnessIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid LocationId;

	/** 0..2 – wie stark die Handlung ausgeführt wurde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life", meta = (ClampMin = "0", ClampMax = "2"))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	TArray<FGenesisGroupContext> Groups;

	/** Reaktion auf dieses Ereignis (direkte Ursache). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FGuid CausedByEventId;

	/** Einflüsse (Rat, Propaganda, Vorbild), die die Entscheidung mitgeformt haben. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	TArray<FGuid> InfluenceEventIds;

	/** Erinnerungssatz aus Sicht des Handelnden (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Life")
	FText Narrative;
};
