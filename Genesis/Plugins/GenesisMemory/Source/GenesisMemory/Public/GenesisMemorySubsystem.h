// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisMemoryWorld.h"
#include "GenesisPersistence.h"
#include "GenesisMemorySubsystem.generated.h"

struct FGenesisSimulationStep;

/** Projekteinstellungen des Gedächtnisses (Project Settings → Genesis → Memory). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Memory"))
class GENESISMEMORY_API UGenesisMemorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Dynamics")
	FGenesisMemoryDynamicsParams Dynamics;

	/** Weltzeit zwischen zwei Zerfallsschritten (Tage). */
	UPROPERTY(Config, EditAnywhere, Category = "Dynamics", meta = (ClampMin = "0.04"))
	float DecayIntervalDays = 1.0f;

	/** Verdichtung: Ereignisse bis zu dieser Tragweite dürfen entfernt werden. */
	UPROPERTY(Config, EditAnywhere, Category = "Compaction", meta = (ClampMin = "0", ClampMax = "1"))
	float CompactionMaxMagnitude = 0.15f;

	UPROPERTY(Config, EditAnywhere, Category = "Compaction", meta = (ClampMin = "0", ClampMax = "1"))
	float CompactionMinBridgedStrength = 0.05f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnEventRecorded, const FGenesisCausalEvent& /*Event*/);

/**
 * Causal Memory Graph der Welt.
 * Persistenz-Ebene: World (Ereignisse und Erinnerungen überdauern ihre Personen).
 */
UCLASS()
class GENESISMEMORY_API UGenesisMemorySubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Memory"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	void ResetWorld(uint64 WorldSeed);

	/** Nimmt ein Ereignis auf und benachrichtigt Beobachter (Story Director, Cinematics …). */
	const FGenesisCausalEvent* RecordEvent(const FGenesisCausalEvent& Event);

	FGenesisMemoryWorld& GetMemoryWorld() { return MemoryWorld; }
	const FGenesisMemoryWorld& GetMemoryWorld() const { return MemoryWorld; }

	/** Verdichtung, typischerweise beim Tod bzw. Generationswechsel. */
	int32 CompactGraph();

	/** Wird von GenesisLifeSimulation nach eigenen Aufzeichnungen aufgerufen, damit Beobachter informiert werden. */
	void NotifyEventRecorded(const FGenesisCausalEvent& Event) { OnEventRecorded.Broadcast(Event); }

	FGenesisOnEventRecorded OnEventRecorded;

private:
	void HandleSimulationStep(const FGenesisSimulationStep& Step);

	UPROPERTY()
	FGenesisMemoryWorld MemoryWorld;

	FDelegateHandle ClockHandle;
};
