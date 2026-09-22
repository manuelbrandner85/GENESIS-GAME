// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisEmbryoTypes.h"
#include "GenesisEmbryogenesisTypes.h"
#include "GenesisPersistence.h"
#include "GenesisEmbryoSubsystem.generated.h"

struct FGenesisSimulationStep;
struct FGenesisConceptionRecord;

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnEmbryoStage, const FGenesisEmbryoState& /*State*/, EGenesisEmbryoStage /*Previous*/);

/**
 * Die erste Woche eines Menschen: von der Zygote bis zur Einnistung.
 *
 * Der Keim entsteht aus der Zeugung (GenesisConception) und läuft auf der Weltuhr weiter.
 * Mit der Einnistung übergibt er an die Körpersimulation, die die Schwangerschaft in Wochen weiterführt –
 * die Entwicklungsqualität der ersten Woche bleibt dabei als Startwert der Organbildung erhalten.
 *
 * Persistenz-Ebene: World.
 */
UCLASS()
class GENESISEMBRYO_API UGenesisEmbryoSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Embryo"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Legt den Keim an. Geschieht sonst automatisch, sobald eine Zeugung gemeldet wird. */
	void BeginEmbryo(const FGuid& EntityId, const FGuid& GenomeId, float Vitality, float Resilience);

	/** Führt den Keim um Stunden weiter (auch aus der Konsole: genesis.Embryo.Advance). */
	void AdvanceHours(double Hours);

	bool HasEmbryo() const { return State.EntityId.IsValid(); }
	const FGenesisEmbryoState& GetState() const { return State; }

	UPROPERTY(EditAnywhere, Category = "Genesis|Embryo")
	FGenesisEmbryoTuning Tuning;

	/** Stellschrauben der dritten und vierten Woche (GENESIS-041). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Embryo")
	FGenesisEmbryogenesisTuning EmbryogenesisTuning;

	/**
	 * Ernährung der Mutter (0..1). Sie entscheidet über das Risiko, dass sich das Neuralrohr nicht schließt (Folat).
	 * Solange die Mutter keine eigene Person mit Körper ist, steht hier der Wert aus der Regie.
	 */
	UPROPERTY(EditAnywhere, Category = "Genesis|Embryo")
	float MotherNutrition = 0.7f;

	/** Der Embryo steht: Ende der vierten Woche. Ab hier führt die Körpersimulation die Schwangerschaft weiter. */
	bool IsEmbryogenesisComplete() const { return State.Embryogenesis.Stage == EGenesisEmbryogenesisStage::Complete; }

	/** Stufenwechsel – für Kamera, Musik und Erzählung. */
	FGenesisOnEmbryoStage OnStageChanged;

private:
	void HandleSimulationStep(const FGenesisSimulationStep& Step);
	void HandleConceived(const FGenesisConceptionRecord& Record);
	void HandleStageChange(EGenesisEmbryoStage Previous);
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisEmbryoState State;

	FDelegateHandle StepHandle;
	FDelegateHandle ConceivedHandle;
	bool bDebugPageRegistered = false;
};
