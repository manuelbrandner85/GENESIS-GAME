// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisPersistence.h"
#include "GenesisEarlyLifeSubsystem.generated.h"

struct FGenesisSimulationStep;
struct FGenesisBirthState;
enum class EGenesisLaborStage : uint8;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnNewbornStage, const FGenesisNewbornState& /*State*/, EGenesisNewbornStage /*Previous*/);

/**
 * Die erste Stunde eines Lebens – und die Stelle, an der aus Wahrnehmung Biografie wird.
 *
 * Hier entsteht die **erste Erinnerung** überhaupt: kein Bild und kein Satz, sondern Wärme, ein Herzschlag
 * und eine Stimme, die das Kind schon kennt. Sie wird als Spur im Gedächtnis abgelegt und als Resonanz
 * in der Seele verstärkt – beides trägt ein Leben lang.
 *
 * Persistenz-Ebene: World.
 */
UCLASS()
class GENESISEARLYLIFE_API UGenesisEarlyLifeSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.EarlyLife"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Beginnt die erste Stunde. Geschieht sonst automatisch mit der Geburt. */
	void BeginNewborn(const FGuid& EntityId);

	void AdvanceMinutes(double Minutes);

	bool HasNewborn() const { return State.EntityId.IsValid(); }
	const FGenesisNewbornState& GetState() const { return State; }
	FGenesisNewbornPerception GetPerception() const;

	/** Das Kind liegt auf der Haut der Mutter – die wichtigste Entscheidung der ersten Stunde. */
	void SetSkinToSkin(bool bEnabled);

	/** Jemand spricht mit dem Kind. */
	void SetMotherSpeaking(bool bEnabled);

	/** Kind und Mutter sehen einander in die Augen. */
	void SetEyeContact(bool bEnabled);

	/**
	 * Das Kind schreit aus eigenem Antrieb (0..1). Sein einziges Werkzeug: Die Welt hört es.
	 * Es kostet Wärme und Ruhe – aber es holt Hilfe.
	 */
	void SetCryEffort(float Effort);

	/** Das Kind sucht die Brust (0..1). Wirkt nur auf der Haut der Mutter. */
	void SetRootingEffort(float Effort);

	UPROPERTY(EditAnywhere, Category = "Genesis|EarlyLife")
	FGenesisEarlyLifeTuning Tuning;

	/** Simulationsminuten je Sekunde Echtzeit (0 = nur Weltuhr). */
	UPROPERTY(EditAnywhere, Category = "Genesis|EarlyLife")
	float TimeScale = 0.0f;

	FGenesisOnNewbornStage OnStageChanged;

private:
	bool TickRealTime(float DeltaSeconds);
	void HandleSimulationStep(const FGenesisSimulationStep& Step);
	void HandleBirthStage(const FGenesisBirthState& BirthState, EGenesisLaborStage Previous);
	void HandleStageChange(EGenesisNewbornStage Previous);
	void EncodeFirstMemory();
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisNewbornState State;

	FDelegateHandle StepHandle;
	FDelegateHandle BirthHandle;
	FTSTicker::FDelegateHandle TickHandle;
	bool bDebugPageRegistered = false;
};
