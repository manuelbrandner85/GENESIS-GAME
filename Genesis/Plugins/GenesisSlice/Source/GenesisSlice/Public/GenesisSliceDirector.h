// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisPersistence.h"
#include "GenesisSliceTypes.h"
#include "GenesisSliceDirector.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnSlicePhase, const FGenesisSliceState& /*State*/, EGenesisSlicePhase /*Previous*/);

/**
 * Die Regie des Vertical Slice.
 *
 * Sie hält den Faden von der Befruchtung bis zur ersten Stunde: Sie hört, was die Systeme melden,
 * springt über die Zeit, in der nichts zu sehen ist (neun Monate), wechselt den Ort, wenn das Kind
 * ihn wechselt, und hört auf, wenn das Leben zu Ende ist oder schläft.
 *
 * Was sie **nicht** tut: Sie entscheidet nichts über den Ausgang. Ob der Keim sich einnistet, ob die
 * Geburt gut verläuft, ob das Kind zur Ruhe kommt – all das entscheiden die Systeme.
 *
 * Persistenz-Ebene: World.
 */
UCLASS()
class GENESISSLICE_API UGenesisSliceDirector : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Slice"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Startet einen Durchlauf. Ohne Seed entscheidet der Zufall, mit Seed ist er wiederholbar. */
	void StartRun(uint64 Seed = 0);

	/** Bricht den laufenden Durchlauf ab. */
	void AbortRun();

	const FGenesisSliceState& GetState() const { return State; }
	FGenesisSliceSignals ReadSignals() const;

	bool IsRunning() const { return State.IsRunning(); }

	UPROPERTY(EditAnywhere, Category = "Genesis|Slice")
	FGenesisSliceTuning Tuning;

	FGenesisOnSlicePhase OnPhaseChanged;

private:
	bool Tick(float DeltaSeconds);
	void EnterPhase(EGenesisSlicePhase NewPhase, EGenesisSliceEnding Ending);
	void DrivePhase(float DeltaSeconds);
	void TravelTo(FName MapName);
	void FadeOut(float Seconds);
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisSliceState State;

	FTSTicker::FDelegateHandle TickHandle;
	float GestationStepTimer = 0.0f;
	bool bDebugPageRegistered = false;
	bool bTravelPending = false;
	/** Ob schon geprüft wurde, ob dieser Start ein Spielstart ist. */
	/** Ob die Versorgung nach der Geburt schon geschehen ist. */
	bool bCareGiven = false;
};
