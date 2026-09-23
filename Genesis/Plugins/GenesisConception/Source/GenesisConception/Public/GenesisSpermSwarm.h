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

	/**
	 * Zellen im Abschnitt. Rund 18 Stunden nach der Besamung finden sich in beiden Eileitern zusammen im
	 * Median 251 Spermien (79–1.386; Williams 1993). Vorher standen hier 6.000 – ein Bild, das nach Schwarm
	 * aussah und falsch war (Docs/38). In diesen 3 mm vor der Eizelle sind es 80 – mit 150 fanden
	 * zehn und mehr den Spalt unter der Zona, mehr als bei der Maus im Reagenzglas (2–9, Dubois 2025).
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "1", ClampMax = "20000"))
	int32 CellCount = 80;

	UPROPERTY(EditAnywhere, Category = "Swarm")
	int32 Seed = 1;

	/**
	 * Wie weit der Pulk beim Start unterhalb der Eizelle steht (µm, Richtung Gebärmutter).
	 * Die Zellen ziehen von dort flussaufwärts – so, wie sie tatsächlich ankommen.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm")
	float StartBandDistanceUm = 1000.0f;

	/** Streuung des Pulks entlang des Kanals (µm). 0 = gleichmäßig über den ganzen Abschnitt verteilt. */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0"))
	float StartBandSpreadUm = 350.0f;

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
	 * Zeitlupe, solange die eigene Zelle in der Gallerte des Cumulus schwimmt (GENESIS-047 Teil 2). Der Weg durch
	 * die 1,4 mm große Wolke dauert eine Minute; mit der Zeitlupe des freien Kanals (0,3) wären das über drei Minuten.
	 * In der Gallerte schlägt die Zelle hyperaktiviert mit 9–15 Hz – bei 0,5 bleiben auch dann acht Bilder je Schlag.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0", ClampMax = "4"))
	float CumulusTimeScale = 0.5f;

	/**
	 * Zeitraffer an der Eizelle (Simulationssekunden je Echtzeitsekunde, GENESIS-047).
	 *
	 * Durch die Zona braucht eine Zelle rund 13 Minuten, im Spalt darunter bis zur Verschmelzung weitere
	 * 16 ± 6 (Docs/38). Vorher war das in einer halben Minute Zeitlupe geschehen – heimlich zwanzig- bis
	 * sechzigmal zu schnell. Jetzt läuft die Zeit dort sichtbar gerafft, mit Uhr, wie in einer
	 * Zeitrafferaufnahme am Mikroskop: Eine halbe Stunde Biologie dauert so gut vierzig Sekunden.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "1", ClampMax = "600"))
	float TimeLapseScale = 45.0f;

	/**
	 * Schrittweite im Zeitraffer (s Simulationszeit). Die feine Schrittweite des Schwimmmodells (1/240 s) ergäbe
	 * bei 45-facher Raffung 180 Schritte je Bild; 1/30 s bleibt für eine schwimmende Zelle (unter 2 µm je Schritt)
	 * genau genug und kostet ein Achtel.
	 */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float TimeLapseStepSeconds = 1.0f / 30.0f;

	/** Übergang zwischen Zeitlupe und Zeitraffer (s Echtzeit) – kein Sprung, ein Anziehen wie beim Filmschnitt mit Rampe. */
	UPROPERTY(EditAnywhere, Category = "Swarm", meta = (ClampMin = "0.1"))
	float TimeScaleRampSeconds = 3.0f;

	/** Zeitmaßstab, der gerade tatsächlich gilt (unter 1 Zeitlupe, über 1 Zeitraffer). */
	float GetEffectiveTimeScale() const { return EffectiveTimeScale >= 0.0f ? EffectiveTimeScale : TimeScale; }

	/** Läuft die Szene gerade sichtbar gerafft? */
	bool IsTimeLapse() const { return GetEffectiveTimeScale() > 1.5f; }

	/** Biologische Zeit, seit die eigene Zelle an der Zona gebunden hat (s; −1 = noch nicht). */
	float GetPlayerSecondsAtEgg() const { return PlayerAtEggSeconds >= 0.0 ? static_cast<float>(SimulationSeconds - PlayerAtEggSeconds) : -1.0f; }

	/** Zellen im perivitellinen Spalt (für die Anzeige). */
	int32 GetPerivitellineCells() const;

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
	double PlayerAtEggSeconds = -1.0;
	bool WantsTimeLapse() const;

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

	void SimulateFor(float SimulationDelta, float StepSeconds);
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
