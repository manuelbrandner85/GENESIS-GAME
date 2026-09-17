// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GenesisTypes.h"
#include "GenesisPersistence.h"
#include "GenesisWorldClockSubsystem.generated.h"

/** Ein Simulationsschritt der Weltzeit. */
USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisSimulationStep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Time")
	FGenesisTimestamp From;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Time")
	FGenesisTimestamp To;

	/**
	 * true bei großen Zeitsprüngen (z. B. Jahre zwischen Lebensabschnitten).
	 * Systeme rechnen dann statistisch/aggregiert statt Schritt für Schritt (Simulation LOD).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Time")
	bool bIsTimeSkip = false;

	int64 GetDeltaSeconds() const { return To - From; }
	double GetDeltaYears() const { return FGenesisTimestamp::YearsBetween(From, To); }
};

/** Persistenter Zustand der Weltuhr. */
USTRUCT()
struct GENESISCORE_API FGenesisWorldClockState
{
	GENERATED_BODY()

	UPROPERTY()
	FGenesisTimestamp Now;

	UPROPERTY()
	double TimeScale = 60.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnSimulationStep, const FGenesisSimulationStep& /*Step*/);

/**
 * Weltuhr von GENESIS.
 *
 * Entkoppelt Simulationszeit von Framerate: Die Uhr sammelt Echtzeit × TimeScale und löst feste
 * Simulationsschritte aus. Alle Lebenssysteme (Körper, Beziehungen, Konsequenzen, Gerüchte ...)
 * rechnen auf diesen Schritten – nie direkt im Frame-Tick.
 *
 * TimeScale ist das technische Fundament der "subjektiven Zeit": Der Living World Director setzt ihn
 * je nach Lebensphase und Situation (Kindheit langsam, Erwachsenenleben schnell, stille Momente gedehnt).
 */
UCLASS()
class GENESISCORE_API UGenesisWorldClockSubsystem : public UGameInstanceSubsystem, public FTickableGameObject, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.WorldClock"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Aktueller Weltzeitpunkt. */
	const FGenesisTimestamp& GetNow() const { return State.Now; }

	/** Setzt die Weltzeit hart (Neues Leben, neue Epoche). Löst keinen Simulationsschritt aus. */
	void SetNow(const FGenesisTimestamp& NewNow);

	/** Weltsekunden pro Echtzeitsekunde. 0 friert die Weltzeit ein. */
	void SetTimeScale(double NewTimeScale);
	double GetTimeScale() const { return State.TimeScale; }

	void SetPaused(bool bInPaused) { bPaused = bInPaused; }
	bool IsPaused() const { return bPaused; }

	/** Springt um DeltaSeconds vorwärts und meldet einen einzigen Schritt mit bIsTimeSkip = true. */
	void SkipTime(int64 DeltaSeconds);

	/** Rückt die Zeit ohne Echtzeit synchron vor (Tests, Server-Simulation, Debug-Befehle). */
	void AdvanceImmediately(int64 DeltaSeconds);

	/** Länge eines regulären Simulationsschritts in Weltsekunden. */
	void SetStepSeconds(int64 NewStepSeconds);
	int64 GetStepSeconds() const { return StepSeconds; }

	/** Wird für jeden Simulationsschritt ausgelöst (auch für Zeitsprünge). */
	FGenesisOnSimulationStep OnSimulationStep;

private:
	void EmitStep(int64 DeltaSeconds, bool bIsTimeSkip);

	UPROPERTY()
	FGenesisWorldClockState State;

	/** Standard: eine Weltstunde pro Schritt. */
	int64 StepSeconds = FGenesisTimestamp::SecondsPerHour;

	/** Obergrenze regulärer Schritte pro Frame; Überhang wird zu einem Schritt zusammengefasst. */
	int32 MaxStepsPerFrame = 8;

	double AccumulatedWorldSeconds = 0.0;
	bool bPaused = false;
	bool bInitialized = false;
};
