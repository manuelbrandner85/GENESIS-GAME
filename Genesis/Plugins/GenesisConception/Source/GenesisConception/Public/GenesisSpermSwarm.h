// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisSpermSwarm.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class AGenesisOocyte;

/** Wird ausgelöst, sobald eine Zelle mit der Eizelle verschmilzt. */
DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnFertilized, const FGenesisFertilizationResult& /*Result*/);

/**
 * Spermienschwarm im Eileiterabschnitt. Die Kanalachse ist die lokale X-Achse des Actors.
 *
 * Simulation in festen Schritten auf der Simulationszeit (TimeScale × Echtzeit). Standard 0,25 –
 * wie eine Hochgeschwindigkeits-Mikroskopaufnahme (240 fps, abgespielt mit 60 fps): Der Geißelschlag (12–18 Hz)
 * bleibt so ohne Stroboskop-Effekt sichtbar. Darstellung über Instanzen mit Per-Instance-Daten für den Geißelschlag.
 */
UCLASS()
class GENESISCONCEPTION_API AGenesisSpermSwarm : public AActor
{
	GENERATED_BODY()

public:
	AGenesisSpermSwarm();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return bSimulateInEditor; }

	/** Legt alle Zellen neu an (deterministisch aus Seed). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Genesis|Conception")
	void RebuildSwarm();

	int32 GetCellCount() const { return Cells.Num(); }
	const FGenesisSpermCell* GetCell(int32 Index) const { return Cells.IsValidIndex(Index) ? &Cells[Index] : nullptr; }

	/** Weltposition des Kopfes einer Zelle (für Kamera und Gameplay). */
	FVector GetCellHeadWorldPosition(int32 Index) const;
	FTransform GetCellWorldTransform(int32 Index) const;

	const FGenesisOviductChannel& GetChannel() const { return Channel; }
	const FGenesisFertilizationResult& GetFertilizationResult() const { return FertilizationResult; }

	/** Die Eizelle, um die der Schwarm konkurriert (optional – ohne sie schwimmen die Zellen nur). */
	UPROPERTY(EditAnywhere, Category = "Swarm")
	TObjectPtr<AGenesisOocyte> Oocyte;

	AGenesisOocyte* GetOocyte() const { return Oocyte; }

	/** Index der Zelle, die an der Zona hängt oder sich hindurchbohrt (INDEX_NONE = keine). Für die Nahaufnahme. */
	int32 FindAttachedCell() const;

	FGenesisOnFertilized OnFertilized;

	/** Aus der Verschmelzung entsteht sofort ein Mensch (Genom, Körper, Inkarnation). */
	UPROPERTY(EditAnywhere, Category = "Swarm")
	bool bCreateLifeOnFertilization = true;

	double GetSimulationSeconds() const { return SimulationSeconds; }

	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "1", ClampMax = "20000"))
	int32 CellCount = 6000;

	UPROPERTY(EditAnywhere, Category = "Swarm")
	int32 Seed = 1;

	/**
	 * Wie weit der Pulk beim Start unterhalb der Eizelle steht (µm, Richtung Gebärmutter).
	 * Die Zellen ziehen von dort flussaufwärts – so, wie sie tatsächlich ankommen.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm")
	float StartBandDistanceUm = 320.0f;

	/** Streuung des Pulks entlang des Kanals (µm). 0 = gleichmäßig über den ganzen Abschnitt verteilt. */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0"))
	float StartBandSpreadUm = 220.0f;

	/** Vitalität des Ejakulats (0..1); einzelne Zellen streuen darum. */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0", ClampMax = "1"))
	float MeanVitality = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Swarm")
	FGenesisOviductChannel Channel;

	UPROPERTY(EditAnywhere, Category = "Swarm")
	FGenesisSpermSwimTuning Tuning;

	/** Simulationszeit je Echtzeitsekunde (0,25 = Hochgeschwindigkeitsaufnahme 4× verlangsamt). */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0", ClampMax = "4"))
	float TimeScale = 0.25f;

	/** Im Editor-Viewport ohne Play weiterlaufen lassen. */
	UPROPERTY(EditAnywhere, Category = "Swarm")
	bool bSimulateInEditor = false;

	UPROPERTY(EditAnywhere, Category = "Rendering")
	TObjectPtr<UStaticMesh> CellMesh;

	UPROPERTY(EditAnywhere, Category = "Rendering")
	TObjectPtr<UMaterialInterface> CellMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> Instances;

private:
	void SimulateFor(float SimulationDelta);
	void PushInstances(bool bTeleport);
	void RegisterDebugPage();

	TArray<FGenesisSpermCell> Cells;
	TArray<FTransform> TransformBuffer;
	TArray<FTransform> PreviousTransformBuffer;
	TArray<float> CustomDataBuffer;
	FGenesisFertilizationResult FertilizationResult;
	double SimulationSeconds = 0.0;
	float StepAccumulator = 0.0f;
	float LastTickMs = 0.0f;
	FName DebugPageId;
};
