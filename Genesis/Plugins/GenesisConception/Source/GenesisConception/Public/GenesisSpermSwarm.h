// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisSpermRace.h"
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

	// --- Das Wettrennen (GENESIS-037) ---

	/**
	 * Beginnt das Rennen: Das Feld wird als geschlossener Pulk aufgestellt, eine Zelle in der ersten Reihe
	 * gehört dem Spieler. Wird von der Regie aufgerufen, sobald ein Leben beginnt – im Vorspann und im
	 * Menü schwimmt der Schwarm ohne Spieler.
	 */
	void StartRace(uint64 RunSeed = 0);

	bool IsRacing() const { return PlayerCellIndex != INDEX_NONE; }
	int32 GetPlayerCellIndex() const { return PlayerCellIndex; }
	EGenesisRaceOutcome GetRaceOutcome() const { return RaceOutcome; }

	/** Eingabe des Spielers in diesem Bild: Lenken (−1..1 je Achse) und Tastendrücke zum Schlagen. */
	void SetPlayerInput(const FVector2D& Steer, int32 StrokePresses);

	/** Für die Anzeige: Platz im Feld (1 = vorn), Abstand zur Zona (µm), Kraft (0..1), Bohrtiefe (µm). */
	int32 GetPlayerPlace() const { return PlayerPlace; }
	float GetPlayerDistanceToZonaUm() const;
	float GetPlayerVigor() const { return PlayerVigor; }
	float GetPlayerPenetrationUm() const;
	float GetZonaThicknessUm() const;
	EGenesisSpermPhase GetPlayerPhase() const;
	bool IsPlayerHyperactivated() const;

	UPROPERTY(EditAnywhere, Category = "Race")
	FGenesisRaceTuning RaceTuning;

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

	/**
	 * Zeitlupe, solange Zellen an der Zona hängen und bohren (GENESIS-037).
	 *
	 * Die Zeitlupe ist nötig, damit der Geißelschlag als Welle sichtbar bleibt (GENESIS-030). An der Zona
	 * hängen aber nur hyperaktivierte Zellen, und die schlagen langsamer (9–15 Hz statt 16–24 Hz). Die
	 * Szene darf dort deshalb schneller laufen, ohne dass ein Schlag unter fünf Bilder fällt – und die
	 * Minute, in der sich außer dem Bohren nichts ändert, wird eine halbe.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0", ClampMax = "4"))
	float PenetrationTimeScale = 0.5f;

	/** Übergang zwischen den beiden Zeitlupen (s Echtzeit) – kein Sprung, ein Anziehen wie beim Filmschnitt mit Rampe. */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0.1"))
	float TimeScaleRampSeconds = 3.0f;

	/** Zeitlupe, die gerade tatsächlich gilt. */
	float GetEffectiveTimeScale() const { return EffectiveTimeScale >= 0.0f ? EffectiveTimeScale : TimeScale; }

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
	float EffectiveTimeScale = -1.0f;

	bool bRaceLayout = false;
	int32 PlayerCellIndex = INDEX_NONE;
	FVector2D PlayerSteer = FVector2D::ZeroVector;
	int32 PendingStrokes = 0;
	float PlayerVigor = 0.0f;
	EGenesisRaceOutcome RaceOutcome = EGenesisRaceOutcome::None;
	int32 PlayerPlace = 0;
	float PlaceTimer = 0.0f;
	/** Protokoll der Stationen der eigenen Zelle – für die Abstimmung des Rennens im Spiel. */
	uint8 LastReportedPhase = 255;
	bool bReportedCumulus = false;
	bool bReportedHyper = false;
	void ReportPlayerProgress();

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
