// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisTypes.h"
#include "GenesisFertilizationTypes.generated.h"

/**
 * Was eine Zelle gerade tut. Der Weg ist einbahnig: schwimmen → gebunden → durch die Zona → im Spalt unter
 * der Zona → verschmolzen. Wer zu spät kommt, wird abgewiesen – oder bleibt im Spalt liegen (Docs/38).
 */
UENUM(BlueprintType)
enum class EGenesisSpermPhase : uint8
{
	Swimming,
	/** An der Zona pellucida gebunden; die Akrosomreaktion läuft (falls sie nicht schon im Cumulus geschah). */
	Bound,
	/** Schiebt sich schräg durch die Zona – vor allem mechanisch, mit kräftigen Geißelschlägen (Drobnis 1988, Bedford 1998). */
	Penetrating,
	/** Mit der Eizelle verschmolzen – nur eine einzige Zelle erreicht das. */
	Fused,
	/**
	 * Abgewiesen: Die Zona ist nach der Verschmelzung verändert (ZP2 gespalten). Gebundene lösen sich und
	 * schwimmen weiter, ohne erneut zu binden; wer in der Zona steckt, bleibt dort stecken.
	 */
	Blocked,
	/**
	 * Durch die Zona, im perivitellinen Spalt: Der Kopf liegt flach an der Eizellmembran, bis die Membranen
	 * verschmelzen (Maus: 16 ± 6 min, Dubois 2025). Nach der Verschmelzung einer anderen Zelle bleibt sie hier liegen.
	 */
	Perivitelline
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

	/** Zona 17 µm dick: gemessen 16,7–17,7 µm an menschlichen Eizellen (Valeri 2011). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Conception")
	float ZonaOuterRadiusUm = 75.0f;

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

	/** Zellen im perivitellinen Spalt – vor der Verschmelzung Mitbewerberinnen, danach Überzählige, die liegen bleiben. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	int32 PerivitellineCells = 0;

	bool IsFertilized() const { return FertilizedByCell != INDEX_NONE; }
};

/**
 * Stellschrauben der Befruchtung. Alle Zeiten sind biologische Zeit (Simulationssekunden = echte Sekunden
 * im Eileiter). Wie schnell sie abläuft, bestimmt die Szene: Zeitlupe beim Schwimmen, damit der Geißelschlag
 * sichtbar bleibt, sichtbarer Zeitraffer mit Uhr an der Zona (AGenesisSpermSwarm::TimeLapseScale).
 */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisFertilizationTuning
{
	GENERATED_BODY()

	/**
	 * Reichweite der Lockwirkung (Progesteron aus dem Cumulus). Nur kapazitierte Zellen folgen ihr
	 * (Cohen-Dayag 1995) – hyperaktivierte am stärksten, progressive träger, nicht kapazitierte gar nicht.
	 */
	UPROPERTY(EditAnywhere, Category = "Chemotaxis") float ChemotaxisRangeUm = 220.0f;
	UPROPERTY(EditAnywhere, Category = "Chemotaxis") float ChemotaxisTurnRate = 0.9f;

	/**
	 * Hyperaktivierung durch Progesteron (je Sekunde, am Rand des Cumulus voll wirksam): Progesteron aus
	 * den Cumuluszellen öffnet in kapazitierten Spermien den Calciumkanal CatSper – der Schlag wird
	 * asymmetrisch und kräftig. Erst so kann eine Zelle an der Zona binden und durchdringen.
	 */
	UPROPERTY(EditAnywhere, Category = "Chemotaxis", meta = (ClampMin = "0")) float ProgesteroneHyperactivationPerSecond = 0.5f;

	/**
	 * Akrosomreaktion schon im Cumulus (je Sekunde, nur kapazitierte Zellen): Bei der Maus hatten 12 von 13
	 * erfolgreichen Spermien sie schon vor der Zona hinter sich (Jin 2011). Beim Menschen ist das umstritten –
	 * deshalb bleibt die Reaktion an der Zona möglich.
	 */
	UPROPERTY(EditAnywhere, Category = "Chemotaxis", meta = (ClampMin = "0")) float AcrosomeInCumulusPerSecond = 0.08f;

	/** Im Cumulus bremst die Gallerte; die Zellen müssen sich hindurcharbeiten. */
	UPROPERTY(EditAnywhere, Category = "Cumulus", meta = (ClampMin = "0.05", ClampMax = "1")) float CumulusSpeedFactor = 0.55f;

	/**
	 * Bindung an die Zona: Nur kapazitierte (hyperaktivierte) Zellen binden und können die Zona durchdringen.
	 * Ohne Kapazitation fehlen der Zelle die Voraussetzungen für die Akrosomreaktion.
	 */
	UPROPERTY(EditAnywhere, Category = "Binding") float BindingDistanceUm = 2.0f;
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (ClampMin = "0", ClampMax = "1")) float BindingChancePerSecondHyper = 0.85f;
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (ClampMin = "0", ClampMax = "1")) float BindingChancePerSecondProgressive = 0.0f;

	/** Akrosomreaktion: Die Kappe öffnet sich, erst danach kann die Zelle in die Zona eindringen (s). */
	UPROPERTY(EditAnywhere, Category = "Penetration") FFloatInterval AcrosomeReactionSeconds = FFloatInterval(2.0f, 6.0f);

	/**
	 * Vortrieb durch die Zona (µm/s, radial), skaliert mit Vitalität und Schlagkraft. Lebendaufnahmen der
	 * Maus: rund 13 Minuten für die Zona (Jin 2011) – bei 17 µm im Mittel gut 0,02 µm/s. Das Band reicht von
	 * gut 8 Minuten (volle Kraft) bis 19 Minuten (schwacher Schlag).
	 */
	UPROPERTY(EditAnywhere, Category = "Penetration") FFloatInterval PenetrationSpeedUm = FFloatInterval(0.015f, 0.035f);

	/**
	 * Winkel des Eindringens gegen die Senkrechte (Grad): Spermien legen sich flach an und schieben sich
	 * schräg durch die Zona, sie bohren nicht senkrecht (Drobnis 1988).
	 */
	UPROPERTY(EditAnywhere, Category = "Penetration", meta = (ClampMin = "0", ClampMax = "70")) float EntryAngleDegrees = 40.0f;

	/**
	 * Manche Zellen bleiben stecken und lösen sich wieder (je Sekunde, bei schwachem Schlag; ein kräftiger
	 * senkt die Rate auf ein Drittel). Bei 13 Minuten in der Zona: rund die Hälfte der schwach schlagenden
	 * Zellen gibt auf, von den kräftig schlagenden jede siebte.
	 */
	UPROPERTY(EditAnywhere, Category = "Penetration", meta = (ClampMin = "0", ClampMax = "1")) float PenetrationFailureRate = 0.001f;

	/**
	 * Zeit im perivitellinen Spalt bis zur Verschmelzung (s): Maus, Lebendaufnahme 15,8 ± 5,7 min (Dubois 2025).
	 * Die Membranen müssen sich finden (Izumo1 an Juno), verschmolzen wird seitlich am Kopf (Äquatorialsegment).
	 */
	UPROPERTY(EditAnywhere, Category = "Fusion") float PerivitellineMeanSeconds = 948.0f;
	UPROPERTY(EditAnywhere, Category = "Fusion") float PerivitellineSigmaSeconds = 342.0f;
	UPROPERTY(EditAnywhere, Category = "Fusion") FFloatInterval PerivitellineClampSeconds = FFloatInterval(300.0f, 1800.0f);

	/** Spalt zwischen Eizellmembran und Zona, in dem der Kopf flach liegt: Neigung gegen die Senkrechte (Grad). */
	UPROPERTY(EditAnywhere, Category = "Fusion", meta = (ClampMin = "0", ClampMax = "90")) float PerivitellineTiltDegrees = 70.0f;

	/**
	 * Cortikalreaktion nach der Verschmelzung bis zum Ende (s). Die Granula geben Ovastacin frei, das ZP2
	 * spaltet (Burkart 2012); die Membran verliert Juno innerhalb von rund 40 Minuten (Bianchi 2014).
	 */
	UPROPERTY(EditAnywhere, Category = "Block") float CorticalReactionSeconds = 1200.0f;

	/**
	 * Ab diesem Fortschritt bindet keine Zelle mehr an der Zona (bei 20 min: nach 5 Minuten). Die zweite
	 * Verschmelzung verhindert die Membran schon vorher – im Modell verschmilzt nur die erste Zelle.
	 */
	UPROPERTY(EditAnywhere, Category = "Block", meta = (ClampMin = "0", ClampMax = "1")) float HardeningBlockThreshold = 0.25f;
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
