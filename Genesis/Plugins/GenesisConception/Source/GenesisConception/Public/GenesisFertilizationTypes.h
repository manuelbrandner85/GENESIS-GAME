// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisTypes.h"
#include "GenesisFertilizationTypes.generated.h"

/** Was eine Zelle gerade tut. Der Weg ist einbahnig: schwimmen → gebunden → durchdringen → verschmolzen oder abgewiesen. */
UENUM(BlueprintType)
enum class EGenesisSpermPhase : uint8
{
	Swimming,
	/** An der Zona pellucida gebunden; die Akrosomreaktion läuft. */
	Bound,
	/** Bohrt sich durch die Zona (Enzyme + Schlagkraft). */
	Penetrating,
	/** Mit der Eizelle verschmolzen – nur eine einzige Zelle erreicht das. */
	Fused,
	/** Zu spät: Die Zona ist nach der Cortikalreaktion verhärtet. */
	Blocked
};

/**
 * Reife Eizelle im Eileiter (Metaphase II), mit Hülle und Cumulus.
 * Alle Maße in µm, Ursprung = Mittelpunkt, Koordinaten wie im Kanal (X = Kanalachse).
 */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisOocyteState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	FVector Position = FVector::ZeroVector;

	/** Zellleib (Ooplasma). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	float OoplasmRadiusUm = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	float ZonaInnerRadiusUm = 58.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	float ZonaOuterRadiusUm = 72.0f;

	/** Äußerer Rand des Cumulus (Corona radiata mit Gallerte) – dort werden Zellen langsamer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	float CumulusRadiusUm = 118.0f;

	/** Nach der Verschmelzung: Die Cortikalreaktion härtet die Zona und sperrt alle weiteren Zellen aus. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	bool bZonaHardened = false;

	/** Index der Zelle, die verschmolzen ist (INDEX_NONE = noch unbefruchtet). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 FertilizedByCell = INDEX_NONE;

	/** Simulationszeit seit der Verschmelzung (s). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float SecondsSinceFusion = 0.0f;

	/** 0..1 – Fortschritt der Cortikalreaktion (Zona-Verhärtung). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float CorticalReaction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 BoundCells = 0;

	bool IsFertilized() const { return FertilizedByCell != INDEX_NONE; }
};

/**
 * Stellschrauben der Befruchtung. Die Zeiten sind Simulationszeit; die Szene läuft in Zeitlupe (TimeScale),
 * damit der Geißelschlag sichtbar bleibt.
 */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisFertilizationTuning
{
	GENERATED_BODY()

	/** Reichweite der Lockwirkung (Progesteron aus dem Cumulus). Nur hyperaktivierte Zellen reagieren darauf. */
	UPROPERTY(EditAnywhere, Category = "Chemotaxis") float ChemotaxisRangeUm = 220.0f;
	UPROPERTY(EditAnywhere, Category = "Chemotaxis") float ChemotaxisTurnRate = 0.9f;

	/**
	 * Hyperaktivierung durch Progesteron (je Sekunde, am Rand des Cumulus voll wirksam): Progesteron aus
	 * den Cumuluszellen öffnet in kapazitierten Spermien den Calciumkanal CatSper – der Schlag wird
	 * asymmetrisch und kräftig. Erst so kann eine Zelle an der Zona binden und durchdringen.
	 */
	UPROPERTY(EditAnywhere, Category = "Chemotaxis", meta = (ClampMin = "0")) float ProgesteroneHyperactivationPerSecond = 0.5f;

	/** Im Cumulus bremst die Gallerte; die Zellen müssen sich hindurcharbeiten. */
	UPROPERTY(EditAnywhere, Category = "Cumulus", meta = (ClampMin = "0.05", ClampMax = "1")) float CumulusSpeedFactor = 0.55f;

	/**
	 * Bindung an die Zona: Nur kapazitierte (hyperaktivierte) Zellen binden und können die Zona durchdringen.
	 * Ohne Kapazitation fehlen der Zelle die Voraussetzungen für die Akrosomreaktion.
	 */
	UPROPERTY(EditAnywhere, Category = "Binding") float BindingDistanceUm = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (ClampMin = "0", ClampMax = "1")) float BindingChancePerSecondHyper = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (ClampMin = "0", ClampMax = "1")) float BindingChancePerSecondProgressive = 0.0f;

	/** Akrosomreaktion: Die Kappe platzt auf und gibt Enzyme frei, bevor die Zelle bohren kann (s). */
	UPROPERTY(EditAnywhere, Category = "Penetration") FFloatInterval AcrosomeReactionSeconds = FFloatInterval(2.0f, 6.0f);

	/** Bohrgeschwindigkeit durch die Zona (µm/s), skaliert mit Vitalität und Schlagkraft. */
	UPROPERTY(EditAnywhere, Category = "Penetration") FFloatInterval PenetrationSpeedUm = FFloatInterval(0.35f, 1.2f);

	/** Manche Zellen bleiben stecken und geben auf (je Sekunde). */
	UPROPERTY(EditAnywhere, Category = "Penetration", meta = (ClampMin = "0", ClampMax = "1")) float PenetrationFailureRate = 0.02f;

	/** Cortikalreaktion nach der Verschmelzung: Zeit bis die Zona vollständig verhärtet ist (s). */
	UPROPERTY(EditAnywhere, Category = "Block") float CorticalReactionSeconds = 12.0f;

	/** Ab diesem Fortschritt bindet keine Zelle mehr (die Membranblockade wirkt sofort, die Zona-Blockade verzögert). */
	UPROPERTY(EditAnywhere, Category = "Block", meta = (ClampMin = "0", ClampMax = "1")) float HardeningBlockThreshold = 0.15f;
};

/** Ergebnis einer Befruchtung – Eingang für Genetik, Körper und Seele. */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisFertilizationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 CellIndex = INDEX_NONE;

	/** Vitalität der erfolgreichen Zelle (fließt in die Startwerte des Körpers). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float Vitality = 0.0f;

	/** Wie lange die Zelle vom Start bis zur Verschmelzung gebraucht hat (Simulationssekunden). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float SecondsToFusion = 0.0f;

	/** Wie viele Zellen gleichzeitig an der Zona hingen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 CompetingCells = 0;

	/** Wie viele Zellen danach abgewiesen wurden (Cortikalreaktion). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 BlockedCells = 0;
};

/**
 * Was aus der Verschmelzung entstanden ist: die Brücke vom Mikrokosmos in die Lebenssimulation.
 * Wird in der Welt gespeichert – der Moment der Zeugung ist Teil der Biografie.
 */
USTRUCT()
struct GENESISCONCEPTION_API FGenesisConceptionRecord
{
	GENERATED_BODY()

	UPROPERTY() bool bConceived = false;

	/** Die Person, die entstanden ist (Schlüssel für Körper, Seele, Musik). */
	UPROPERTY() FGuid EntityId;

	UPROPERTY() FGuid GenomeId;
	UPROPERTY() FGuid MotherGenomeId;
	UPROPERTY() FGuid FatherGenomeId;

	/** Startwerte des ersten Körpers. */
	UPROPERTY() float Vitality = 0.0f;
	UPROPERTY() float Resilience = 0.0f;

	/** Weg der erfolgreichen Zelle (Simulationssekunden) und Wettbewerb an der Zona. */
	UPROPERTY() float SecondsToFusion = 0.0f;
	UPROPERTY() int32 CompetingCells = 0;
	UPROPERTY() int32 BlockedCells = 0;

	UPROPERTY() FGenesisTimestamp ConceptionTime;
};
