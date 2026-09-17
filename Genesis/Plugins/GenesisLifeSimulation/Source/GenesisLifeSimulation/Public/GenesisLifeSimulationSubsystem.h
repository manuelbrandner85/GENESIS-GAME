// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisLifeAction.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisPersistence.h"
#include "GenesisLifeSimulationSubsystem.generated.h"

class UGenesisLifeSimulationEngine;
struct FGenesisSimulationStep;

/** Projekteinstellungen (Project Settings → Genesis → Life Simulation). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Life Simulation"))
class GENESISLIFESIMULATION_API UGenesisLifeSimulationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Tuning")
	FGenesisLifeSimulationTuning Tuning;
};

/**
 * Bindet die Life Simulation Engine an das Spiel: Weltuhr, Gedächtnis, Genetik, Persistenz.
 * Persistenz-Ebene: World.
 */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisLifeSimulationSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.LifeSimulation"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Meldet eine Handlung (Spieler, NPC, Director). Zeitpunkt = aktuelle Weltzeit. @return EventId */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Life")
	FGuid ReportAction(const FGenesisLifeAction& Action);

	/** Legt eine Person an oder aktualisiert ihre Stammdaten. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Life")
	void RegisterEntity(const FGenesisLifeProfile& Profile);

	/** Simulation LOD umschalten (z. B. NPC kommt in Spielernähe). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Life")
	void SetSimulationLevel(const FGuid& EntityId, EGenesisSimulationLevel Level);

	/** Dominante Werte der Epoche setzen (GenesisWorld beim Epochenwechsel). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Life")
	void SetZeitgeist(const TArray<FGenesisWeightedTag>& DominantValues);

	UGenesisLifeSimulationEngine* GetEngine() const { return Engine; }

private:
	void HandleSimulationStep(const FGenesisSimulationStep& Step);
	void HandleConsequenceTriggered(const FGenesisScheduledConsequence& Consequence, const FGuid& EventId);
	void RegisterDebugPage();

	UPROPERTY()
	TObjectPtr<UGenesisLifeSimulationEngine> Engine;

	FDelegateHandle ClockHandle;
};
