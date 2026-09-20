// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisBirthTypes.h"
#include "Containers/Ticker.h"
#include "GenesisPersistence.h"
#include "GenesisBirthSubsystem.generated.h"

struct FGenesisSimulationStep;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnLaborStage, const FGenesisBirthState& /*State*/, EGenesisLaborStage /*Previous*/);

/**
 * Die Geburt als Lauf der Weltuhr.
 *
 * Das Subsystem hält den Zustand, führt ihn auf den Simulationsschritten weiter und übergibt im Moment
 * der Geburt an die anderen Systeme: Der Körper wird geboren (und hört dadurch ab sofort in Luft statt
 * in Fruchtwasser), die Musik wechselt in die Lebensphase Geburt.
 *
 * Persistenz-Ebene: World.
 */
UCLASS()
class GENESISBIRTH_API UGenesisBirthSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Birth"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Beginnt die Geburt. Reife und Lungenreife kommen aus der Körpersimulation. */
	void BeginLabor(const FGuid& EntityId);

	/** Führt die Geburt um Minuten weiter (auch aus der Konsole: genesis.Birth.Advance). */
	void AdvanceMinutes(double Minutes);

	bool HasLabor() const { return State.EntityId.IsValid(); }
	const FGenesisBirthState& GetState() const { return State; }
	FGenesisBirthPerception GetPerception() const;

	UPROPERTY(EditAnywhere, Category = "Genesis|Birth")
	FGenesisBirthTuning Tuning;

	/** Simulationsminuten je Sekunde Echtzeit, solange die Geburt läuft (0 = nur über die Weltuhr). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Birth")
	float LaborTimeScale = 0.0f;

	FGenesisOnLaborStage OnStageChanged;

private:
	/** Echtzeit-Fortschritt, wenn die Geburt als Szene gespielt wird. */
	bool TickRealTime(float DeltaSeconds);
	void HandleSimulationStep(const FGenesisSimulationStep& Step);
	void HandleStageChange(EGenesisLaborStage Previous);
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisBirthState State;

	FDelegateHandle StepHandle;
	FTSTicker::FDelegateHandle TickHandle;
	bool bDebugPageRegistered = false;
};
