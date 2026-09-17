// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenesisDecisionTypes.h"
#include "GenesisDecisionConsiderations.generated.h"

/**
 * Ein Blickwinkel, aus dem eine Person eine Option bewertet (Utility-AI-Consideration).
 * Liefert −1 (spricht dagegen) … +1 (spricht dafür). Zustandslos.
 * Weitere Module (z. B. GenesisMind mit Gedankeninventar) ergänzen eigene Considerations.
 */
UCLASS(Abstract)
class GENESISDECISION_API UGenesisDecisionConsideration : public UObject
{
	GENERATED_BODY()

public:
	virtual FName GetConsiderationName() const PURE_VIRTUAL(UGenesisDecisionConsideration::GetConsiderationName, return NAME_None;);

	/** Unterbewusste Considerations fließen in Impuls und Zeitdruck-Gewichtung ein. */
	virtual bool IsSubconscious() const { return false; }

	/**
	 * @param OutInfluences Ereignisse (z. B. Erinnerungen), die diese Bewertung geprägt haben
	 */
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const PURE_VIRTUAL(UGenesisDecisionConsideration::Evaluate, return 0.0f;);

	/** Gewichtung im Verhältnis zu den anderen Considerations. */
	UPROPERTY(EditAnywhere, Category = "Consideration", meta = (ClampMin = "0"))
	float Weight = 1.0f;
};

/** Charakter: passt die Handlung zu dem, was die Person durch ihr bisheriges Handeln geworden ist (Karma)? */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Character : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	UGenesisConsideration_Character() { Weight = 1.5f; }
	virtual FName GetConsiderationName() const override { return TEXT("Character"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Motiv: Eigennutz vs. Nutzen für andere – gewichtet nach dem Egoismus/Altruismus-Spektrum. */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Motive : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	virtual FName GetConsiderationName() const override { return TEXT("Motive"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Überzeugungen: verinnerlichte Ideologien und Glaubenstradition. */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Conviction : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	virtual FName GetConsiderationName() const override { return TEXT("Conviction"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Kultur: Tabus und hochgehaltene Werte. */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Culture : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	virtual FName GetConsiderationName() const override { return TEXT("Culture"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Umfeld: anwesende Gruppen, Zeitgeist, Angst um den eigenen Ruf vor Zeugen. */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Social : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	virtual FName GetConsiderationName() const override { return TEXT("Social"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Beziehung: Wie stehe ich zu den Menschen, die die Handlung betrifft? */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Relationship : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	UGenesisConsideration_Relationship() { Weight = 1.2f; }
	virtual FName GetConsiderationName() const override { return TEXT("Relationship"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/** Erfahrung: Wie haben sich ähnliche erlebte Situationen angefühlt (bewusst zugängliche Erinnerungen)? */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Experience : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	virtual FName GetConsiderationName() const override { return TEXT("Experience"); }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};

/**
 * Unterbewusstsein: verdrängte Erinnerungen (Vermeidung), Stress (Selbstschutz) und Seelen-Echos (thematischer Sog).
 * Die Person kennt diese Einflüsse nicht – sie spürt nur einen Impuls.
 */
UCLASS()
class GENESISDECISION_API UGenesisConsideration_Subconscious : public UGenesisDecisionConsideration
{
	GENERATED_BODY()
public:
	UGenesisConsideration_Subconscious() { Weight = 1.0f; }
	virtual FName GetConsiderationName() const override { return TEXT("Subconscious"); }
	virtual bool IsSubconscious() const override { return true; }
	virtual float Evaluate(const FGenesisDecisionSituation& Situation, const FGenesisDecisionOption& Option, const FGenesisLifeProfile& Decider,
		const FGenesisDecisionInputs& Inputs, TArray<TPair<FGuid, float>>& OutInfluences) const override;
};
