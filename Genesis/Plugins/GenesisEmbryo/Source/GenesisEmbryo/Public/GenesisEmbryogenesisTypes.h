// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisEmbryogenesisTypes.generated.h"

/**
 * Die dritte und vierte Woche (GENESIS-041), Carnegie-Stadien 7 bis 13: Aus der zweiblättrigen Keimscheibe wird ein
 * Körper mit Bauplan. Tage nach der Befruchtung.
 *
 * Quellen: Langman „Medizinische Embryologie", Moore „The Developing Human", O'Rahilly & Müller (Carnegie-Stadien mit
 * Somitenzahlen), Ultraschall- und Embryoskopie-Daten für Herzfrequenz und Scheitel-Steiß-Länge.
 */
UENUM(BlueprintType)
enum class EGenesisEmbryogenesisStage : uint8
{
	/** Noch die zweiblättrige Scheibe der zweiten Woche. */
	None,
	/** Tag 15: Am hinteren Rand der Scheibe erscheint der Primitivstreifen – der Körper bekommt vorn und hinten. */
	PrimitiveStreak,
	/** Tag 16–17: Zellen wandern durch den Streifen ein; aus zwei Blättern werden drei (Ento-, Meso-, Ektoderm). */
	Gastrulation,
	/** Tag 17–19: Die Chorda entsteht als Achse des Körpers; über ihr verdickt sich das Ektoderm. */
	Notochord,
	/** Tag 19–20: Die Neuralplatte hebt ihre Ränder – die Neuralrinne. */
	NeuralPlate,
	/** Tag 20–21: Die ersten Somitenpaare erscheinen; aus ihnen werden Wirbel, Rippen, Muskeln, Lederhaut. */
	Somites,
	/** Tag 22: Der Herzschlauch schließt sich und beginnt zu schlagen – die erste eigene Bewegung. */
	HeartBeats,
	/** Tag 24–28: Beide Neuroporen schließen sich, das Neuralrohr ist zu. Kiemenbögen, Augenbläschen, Extremitätenknospen. */
	NeuralTubeClosed,
	/** Ende der vierten Woche: ein gekrümmter Embryo von 4–5 mm mit rund 30 Somitenpaaren. */
	Complete
};

/** Was schiefgehen kann, wenn sich das Neuralrohr nicht schließt. */
UENUM(BlueprintType)
enum class EGenesisNeuralTubeDefect : uint8
{
	None,
	/** Der vordere Neuroporus bleibt offen – das Gehirn kann sich nicht entwickeln. Nicht mit dem Leben vereinbar. */
	Anencephaly,
	/** Der hintere Neuroporus bleibt offen – offener Rücken; das Kind lebt, mit Folgen für Beine und Blase. */
	SpinaBifida
};

/**
 * Zustand der dritten und vierten Woche. Längen in mm (der Keim ist jetzt mit bloßem Auge zu sehen),
 * Anteile 0..1 wie überall.
 */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisEmbryogenesisState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") EGenesisEmbryogenesisStage Stage = EGenesisEmbryogenesisStage::None;

	/** Scheitel-Steiß-Länge (mm): Tag 15 gut 0,4 mm, Ende der vierten Woche 4–5 mm. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float LengthMm = 0.0f;

	/** 0..1 – Primitivstreifen, Einwanderung der Zellen (drei Blätter), Chorda. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float PrimitiveStreak = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float ThreeLayers = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float Notochord = 0.0f;

	/** 0..1 – Neuralplatte, aufgeworfene Ränder (Neuralrinne). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float NeuralPlate = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float NeuralFolds = 0.0f;

	/**
	 * 0..1 – wie weit das Neuralrohr geschlossen ist. Es schließt in der Mitte zuerst (Höhe der Somiten 4–6) und
	 * läuft von dort nach vorn und nach hinten wie ein Reißverschluss.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float TubeClosure = 0.0f;
	/** 0..1 – noch offener vorderer (Tag 24–26) und hinterer Neuroporus (Tag 26–28). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float RostralNeuropore = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float CaudalNeuropore = 1.0f;

	/** Somitenpaare: erstes an Tag 20, dann etwa eins alle 6–8 Stunden bis rund 30 am Ende der vierten Woche. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 Somites = 0;

	/** 0..1 – der Herzschlauch: zwei Anlagen, die verschmelzen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float HeartTube = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") bool bHeartBeating = false;
	/** Herzfrequenz (Schläge je Minute): ab Tag 22 rund 75, danach steigend. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float HeartRateBpm = 0.0f;

	/** Kiemenbögen (Tag 24 der erste), Augenbläschen, Ohrgrübchen, Extremitätenknospen (Ende der Woche). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 PharyngealArches = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float OpticVesicles = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float OticPits = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float LimbBuds = 0.0f;

	/** 0..1 – der Embryo krümmt sich zur C-Form und wird an den Seiten eingerollt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float Curvature = 0.0f;

	/** Bleibt ein Neuroporus offen? Einmal ausgewürfelt, wenn sich das Rohr zu schließen beginnt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") EGenesisNeuralTubeDefect Defect = EGenesisNeuralTubeDefect::None;
	/** Risiko, mit dem gewürfelt wurde (0..1) – es hängt an der Ernährung der Mutter (Folat). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float DefectRisk = 0.0f;
	UPROPERTY() bool bDefectRolled = false;
};

/**
 * Stellschrauben der dritten und vierten Woche. Zeiten sind Tage nach der Befruchtung.
 */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisEmbryogenesisTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Timing") float StreakDay = 15.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float GastrulationDay = 16.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float NotochordDay = 17.5f;
	UPROPERTY(EditAnywhere, Category = "Timing") float NeuralPlateDay = 19.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float FirstSomiteDay = 20.0f;
	/** Das Rohr schließt in der Mitte zuerst (Somiten 4–6). */
	UPROPERTY(EditAnywhere, Category = "Timing") float TubeClosureStartDay = 21.5f;
	UPROPERTY(EditAnywhere, Category = "Timing") float HeartBeatDay = 22.0f;
	/** Vorderer Neuroporus zu (Tag 24–26), hinterer zwei Tage später. */
	UPROPERTY(EditAnywhere, Category = "Timing") float RostralClosedDay = 25.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float CaudalClosedDay = 27.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float CompleteDay = 28.0f;

	/** Stunden je Somitenpaar (klinisch 6–8 h). */
	UPROPERTY(EditAnywhere, Category = "Timing") float HoursPerSomite = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") int32 MaxSomites = 30;

	/**
	 * Herzfrequenz: Der erste Schlag ist langsam (~75/min), sie steigt bis Woche 9 auf ~170/min
	 * (Ultraschalldaten: Woche 5 ~100, Woche 6 ~120, Woche 7 ~150).
	 */
	UPROPERTY(EditAnywhere, Category = "Heart") float FirstBeatBpm = 75.0f;
	UPROPERTY(EditAnywhere, Category = "Heart") float BpmPerDay = 6.0f;

	/**
	 * Neuralrohrdefekte: rund 1 von 1000 Schwangerschaften. Folat (Folsäure) senkt das Risiko um bis zu 70 %,
	 * ein Mangel hebt es deutlich. Deshalb hängt es an der Ernährung der Mutter.
	 */
	UPROPERTY(EditAnywhere, Category = "Risk") float BaseDefectRisk = 0.001f;
	UPROPERTY(EditAnywhere, Category = "Risk") float PoorNutritionFactor = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Risk") float GoodNutritionFactor = 0.3f;
};
