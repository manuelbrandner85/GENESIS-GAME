// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisDecisionTypes.h"
#include "GenesisDecisionSubsystem.generated.h"

class UGenesisDecisionEngine;

/** Projekteinstellungen (Project Settings → Genesis → Decision). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Decision"))
class GENESISDECISION_API UGenesisDecisionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Tuning")
	FGenesisDecisionTuning Tuning;

	/** Gewichtung einzelner Considerations überschreiben (Name → Gewicht), z. B. "Character" = 2.0. */
	UPROPERTY(Config, EditAnywhere, Category = "Tuning")
	TMap<FName, float> ConsiderationWeights;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnDecisionMade, const FGenesisDecisionSituation& /*Situation*/, const FGenesisDecisionResult& /*Result*/);

/**
 * Entscheidungen im Spiel.
 * - NPCs: DecideForNpc bewertet, wählt und führt die Handlung in der Life Simulation aus.
 * - Spieler: EvaluateSituation liefert Zögern/inneren Konflikt (Körpersprache, Kamera, Audio),
 *   ResolvePlayerChoice führt die Wahl aus – ohne Wahl innerhalb der Zeit entscheidet der Impuls.
 */
UCLASS()
class GENESISDECISION_API UGenesisDecisionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Genesis|Decision")
	FGenesisDecisionResult EvaluateSituation(const FGenesisDecisionSituation& Situation) const;

	UFUNCTION(BlueprintCallable, Category = "Genesis|Decision")
	FGenesisDecisionResult DecideForNpc(const FGenesisDecisionSituation& Situation);

	/** ChosenIndex = INDEX_NONE (-1): Zeit abgelaufen, der Impuls entscheidet. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Decision")
	FGenesisDecisionResult ResolvePlayerChoice(const FGenesisDecisionSituation& Situation, int32 ChosenIndex);

	UGenesisDecisionEngine* GetEngine() const { return Engine; }

	FGenesisOnDecisionMade OnDecisionMade;

private:
	FGenesisDecisionInputs MakeInputs(const FGuid& DeciderId) const;
	FGenesisDecisionResult Execute(const FGenesisDecisionSituation& Situation, FGenesisDecisionResult Result);
	void RegisterDebugPage();

	UPROPERTY()
	TObjectPtr<UGenesisDecisionEngine> Engine;

	/** Letzte Entscheidung für das Developer HUD. */
	FGenesisDecisionSituation LastSituation;
	FGenesisDecisionResult LastResult;
};
