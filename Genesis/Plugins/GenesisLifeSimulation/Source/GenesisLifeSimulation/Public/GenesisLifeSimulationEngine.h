// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenesisLifeAction.h"
#include "GenesisLifeSimulationProcessor.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisLifeSimulationEngine.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnLifeActionProcessed, const FGenesisLifeAction& /*Action*/, const FGenesisLifeSimulationFrame& /*Frame*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnConsequenceTriggered, const FGenesisScheduledConsequence& /*Consequence*/, const FGuid& /*EventId*/);

/**
 * Life Simulation Engine – führt jede Handlung durch alle 14 Systeme.
 *
 * Unabhängig von GameInstance/World: Tests erzeugen die Engine direkt mit eigenem Gedächtnis und Genom-Pool.
 * Das Subsystem verbindet sie mit Weltuhr, Persistenz und den Nachbarmodulen.
 */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisLifeSimulationEngine : public UObject
{
	GENERATED_BODY()

public:
	/** Verbindet die Engine mit Nachbarsystemen (jeweils optional) und setzt die Parameter. */
	void Initialize(FGenesisMemoryWorld* InMemory, FGenesisGenomePool* InGenomes, const TArray<FGenesisTraitDefinition>* InTraits, const FGenesisLifeSimulationTuning& InTuning);

	/** Legt die 14 Standard-Prozessoren an. */
	void AddDefaultProcessors();

	/** Fügt einen Prozessor hinzu und ordnet die Pipeline neu (Stufe, dann Reihenfolge). */
	void AddProcessor(UGenesisLifeSimulationProcessor* Processor);

	const TArray<TObjectPtr<UGenesisLifeSimulationProcessor>>& GetProcessors() const { return Processors; }

	void ResetState(uint64 WorldSeed);

	/** Legt eine Person an oder aktualisiert ihre Stammdaten (verborgene Werte bleiben erhalten). */
	void RegisterEntity(const FGenesisLifeProfile& Profile);

	/**
	 * Verarbeitet eine Handlung durch alle Systeme.
	 * @return EventId im Kausalgraph (auch ohne Gedächtnis gültig)
	 */
	FGuid ProcessAction(const FGenesisLifeAction& Action, const FGenesisTimestamp& Now, FGenesisLifeSimulationFrame* OutFrame = nullptr);

	/** Zeitbasierte Dynamik aller Systeme. */
	void AdvanceTime(const FGenesisSimulationStep& Step);

	FGenesisLifeSimulationState& GetState() { return State; }
	const FGenesisLifeSimulationState& GetState() const { return State; }

	const FGenesisLifeSimulationTuning& GetTuning() const { return Tuning; }
	void SetTuning(const FGenesisLifeSimulationTuning& InTuning) { Tuning = InTuning; }

	FGenesisOnLifeActionProcessed OnActionProcessed;
	FGenesisOnConsequenceTriggered OnConsequenceTriggered;

private:
	FGenesisLifeSimulationContext MakeContext(const FGenesisTimestamp& Now);

	UPROPERTY()
	TArray<TObjectPtr<UGenesisLifeSimulationProcessor>> Processors;

	UPROPERTY()
	FGenesisLifeSimulationState State;

	UPROPERTY()
	FGenesisLifeSimulationTuning Tuning;

	FGenesisMemoryWorld* Memory = nullptr;
	FGenesisGenomePool* Genomes = nullptr;
	const TArray<FGenesisTraitDefinition>* Traits = nullptr;
};
