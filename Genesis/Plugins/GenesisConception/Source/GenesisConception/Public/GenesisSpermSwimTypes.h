// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.h"
#include "GenesisSpermSwimTypes.generated.h"

/**
 * Maßstab der Mikrowelt: 1 µm = 1 Unreal-Einheit (cm). Alle Größen hier in µm und Sekunden.
 */
namespace GenesisMicroScale
{
	/** Unreal-Einheiten je Mikrometer. */
	constexpr float UnitsPerMicrometer = 1.0f;
}

/** Bewegungsmuster einer Spermienzelle (CASA-Kategorien, WHO-Laborhandbuch). */
UENUM(BlueprintType)
enum class EGenesisSpermMotility : uint8
{
	/** Schnell vorwärts, symmetrischer Geißelschlag. */
	Progressive,
	/** Im Eileiter: kräftiger, asymmetrischer Peitschenschlag, wenig geradlinig – löst sich leichter von der Schleimhaut. */
	Hyperactivated,
	/** Geringe Vitalität: langsam, wenig Vortrieb. */
	Sluggish
};

/** Zustand einer Zelle. */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisSpermCell
{
	GENERATED_BODY()

	/** Kopfspitze relativ zur Kanalachse (µm). X = Kanalachse, Richtung Eierstock. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	FVector Position = FVector::ZeroVector;

	/** Schwimmrichtung (Einheitsvektor). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	FVector Heading = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	EGenesisSpermMotility Motility = EGenesisSpermMotility::Progressive;

	/** 0..1 – aus dem Ejakulat/Genom; skaliert Geschwindigkeit und Schlagkraft. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float Vitality = 1.0f;

	/** Vortrieb entlang der Schwimmrichtung (µm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float Speed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float BeatFrequencyHz = 15.0f;

	/** Seitliche Kopfauslenkung (ALH, µm). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float HeadAmplitudeUm = 3.5f;

	/** Wellenlänge der Geißelwelle (µm). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float WavelengthUm = 30.0f;

	/** 0..1 – Asymmetrie des Schlags (Hyperaktivierung). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float Asymmetry = 0.0f;

	/** Phasen in Zyklen (0..1 umlaufend). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	double BeatPhase = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	double RollPhase = 0.0;

	/** Individuelle Eigenheiten (0..1), bestimmen Werte innerhalb der Bandbreiten. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float Individuality = 0.5f;

	/** Gelebte Simulationszeit dieser Zelle (s) – für die Dauer bis zur Verschmelzung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float LifeSeconds = 0.0f;

	/** Was die Zelle gerade tut (schwimmen, gebunden, durchdringen, verschmolzen, abgewiesen). Siehe EGenesisSpermPhase. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	uint8 Phase = 0;

	/** Restzeit der Akrosomreaktion (s), solange die Zelle gebunden ist. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float AcrosomeTimer = 0.0f;

	/** Eingedrungene Tiefe in der Zona pellucida (µm). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Conception")
	float PenetrationDepthUm = 0.0f;

	/**
	 * Gewünschte Schwimmrichtung des Spielers (Einheitsvektor im Kanalraum; null = keine Eingabe).
	 * Sie wirkt wie jede andere Richtungskraft im Modell – mit einer Obergrenze der Drehrate: Eine Zelle
	 * lenkt über einen asymmetrischen Geißelschlag und fährt dabei Bögen, sie kann nicht auf der Stelle wenden.
	 */
	FVector SteerDirection = FVector::ZeroVector;

	/**
	 * Anstrengung beim Bohren durch die Zona (0..1), negativ = aus der eigenen Veranlagung (Individuality).
	 * Wählt innerhalb der gemessenen Bohrgeschwindigkeit – schneller als die Physiologie erlaubt geht es nicht.
	 */
	float Vigor = -1.0f;

	/** Eigener Zufallsstrom – Zellen bleiben unabhängig von Reihenfolge und Anzahl. */
	FGenesisRandomStream Random;
};

/** Der Eileiterabschnitt als vereinfachter Kanal (Lumen zwischen den Schleimhautfalten). */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisOviductChannel
{
	GENERATED_BODY()

	/** Freier Radius bis zu den Faltenspitzen (µm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel", meta = (ClampMin = "50"))
	float LumenRadiusUm = 450.0f;

	/** Simulierte Länge; Zellen laufen an den Enden um (µm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel", meta = (ClampMin = "100"))
	float LengthUm = 3000.0f;

	/** Zilienstrom an der Wand Richtung Gebärmutter (−X), µm/s. Zur Mitte hin schwächer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel", meta = (ClampMin = "0"))
	float WallFlowSpeedUm = 25.0f;

	/**
	 * Anteil des Wandstroms, der auch in der Mitte des Lumens noch fließt (0..1).
	 * Ohne ihn stünde die Flüssigkeit dort still – und die Zellen hätten mitten im Kanal keinen
	 * Hinweis mehr, wohin: Sie schwämmen in alle Richtungen, die halbe Ladung rückwärts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel", meta = (ClampMin = "0", ClampMax = "1"))
	float CoreFlowFraction = 0.45f;
};

/** Stellschrauben des Schwimmmodells. Bandbreiten aus CASA-Referenzwerten menschlicher Spermien. */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisSpermSwimTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveSpeedUm = FFloatInterval(30.0f, 55.0f);
	/** Schlagfrequenz (Hz). Gemessen liegt die Grundfrequenz bei 19–20 Hz, die Kopf-Kreuzfrequenz bei 23,6 ± 5,0. */
	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveBeatHz = FFloatInterval(16.0f, 24.0f);
	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveHeadAmplitudeUm = FFloatInterval(2.5f, 5.0f);
	/** Bogenwellenlänge der Geißelwelle (µm). Gemessener Median: 31 µm. */
	UPROPERTY(EditAnywhere, Category = "Progressive") float ProgressiveWavelengthUm = 31.0f;
	/**
	 * Rotationsdiffusion (rad²/s). Eine progressive Zelle hält ihren Kurs über Sekunden –
	 * gemessen liegt die Richtungsstreuung bei wenigen Hundertstel rad² je Sekunde. Mit einem zu
	 * hohen Wert taumelt der ganze Schwarm, und aus gerichtetem Schwimmen wird ein Gewimmel.
	 */
	UPROPERTY(EditAnywhere, Category = "Progressive") float ProgressiveRotationalDiffusion = 0.035f;

	/**
	 * Höchste Drehrate beim Lenken (rad/s). Spermien lenken, indem sie den Geißelschlag asymmetrisch
	 * machen; bei chemotaktischen Wendungen liegen die Bahnradien bei 20–40 µm. Bei 30–55 µm/s
	 * Vortrieb sind das 1–2,5 rad/s. Hyperaktivierte Zellen schlagen asymmetrischer und wenden enger.
	 */
	UPROPERTY(EditAnywhere, Category = "Steering") float SteerTurnRate = 1.6f;
	UPROPERTY(EditAnywhere, Category = "Steering") float SteerTurnRateHyperFactor = 1.5f;

	/**
	 * Hyperaktivierte Zellen peitschen, aber sie stehen nicht: Gemessen liegt ihre Bahngeschwindigkeit
	 * über 150 µm/s und ihr Vortrieb bei 20–35 µm/s. Mit zu wenig Vortrieb wirkt der Schwarm wie ein
	 * Gewimmel, und genau das war er auch.
	 */
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperSpeedUm = FFloatInterval(20.0f, 35.0f);
	/**
	 * Schlagfrequenz hyperaktivierter Zellen. Sie schlagen **langsamer** als progressive, dafür weiter
	 * ausholend: gemessener Median 10 Hz gegen 19 Hz bei aktivierter Beweglichkeit, in frei
	 * schwimmenden Zellen im Mittel 14,6 Hz und nach Reizung im Median 11,7 Hz. Der Bereich 9–15 Hz
	 * deckt diese Messungen ab; sein Mittel von 12 Hz reicht zusammen mit der Auslenkung aus, um das
	 * CASA-Kriterium VCL ≥ 150 µm/s zu erreichen, ohne eine Zahl zu erfinden.
	 */
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperBeatHz = FFloatInterval(9.0f, 15.0f);
	/** Seitliche Kopfauslenkung hyperaktivierter Zellen (µm). Kriterium: ALH > 7; gemessen 5,7–11,4. */
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperHeadAmplitudeUm = FFloatInterval(7.5f, 11.5f);
	/**
	 * Bogenwellenlänge bei Hyperaktivierung (µm). Gemessener Median: **17 µm** – die Welle wird
	 * kürzer, nicht länger. Der bisherige Wert von 45 µm war genau verkehrt herum gedacht.
	 */
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") float HyperWavelengthUm = 17.0f;
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") float HyperRotationalDiffusion = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Sluggish") FFloatInterval SluggishSpeedUm = FFloatInterval(3.0f, 12.0f);
	UPROPERTY(EditAnywhere, Category = "Sluggish") FFloatInterval SluggishBeatHz = FFloatInterval(3.0f, 7.0f);
	/** Unter dieser Vitalität schwimmt eine Zelle träge. */
	UPROPERTY(EditAnywhere, Category = "Sluggish") float SluggishVitalityThreshold = 0.3f;

	/**
	 * Wechselraten zwischen progressiv und hyperaktiviert (je Sekunde).
	 * In der Ampulle zum Zeitpunkt des Eisprungs sind die Zellen längst kapazitiert – sie haben sich
	 * aus dem Reservoir im Isthmus gelöst. Deshalb liegt das Gleichgewicht hier bei gut der Hälfte.
	 */
	UPROPERTY(EditAnywhere, Category = "Transitions") float HyperactivationRate = 0.05f;
	UPROPERTY(EditAnywhere, Category = "Transitions") float DeactivationRate = 0.045f;

	/** Gegen die Strömung ausrichten (rad/s bei voller Wandscherung). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float RheotaxisTurnRate = 0.8f;
	/** Ab diesem Wandabstand wirkt die hydrodynamische Wandbindung (µm). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallAttractionDistanceUm = 25.0f;
	/** 0..1 – wie stark eine Zelle an der Wand gehalten wird (hyperaktivierte Zellen lösen sich leichter). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallTrapStrength = 0.6f;
	/** An Oberflächen schwimmen Spermien leicht zur Wand geneigt – dadurch bleiben sie lange dort (Grad). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallTiltDegrees = 3.5f;
	/** Mindestabstand Kopfspitze–Wand (µm). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallMarginUm = 3.0f;
	/** Rollen um die Längsachse relativ zur Schlagfrequenz. */
	/**
	 * Rollen um die Längsachse, als Anteil der Schlagfrequenz.
	 * Gemessen rollen menschliche Spermien mit 4–8 Hz (Mittel 6,0 ± 2,1), in zähem Medium bis 10 Hz –
	 * bei einem Schlag um 20 Hz sind das etwa 0,3. Das Rollen ist keine Zierde: Ohne Rotation gibt es
	 * keine Rheotaxis, die Zelle könnte die Strömung gar nicht abtasten.
	 */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float RollPerBeat = 0.3f;

	/** Feste Simulationsschrittweite (s Simulationszeit). */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.001")) float FixedStepSeconds = 1.0f / 240.0f;
};
