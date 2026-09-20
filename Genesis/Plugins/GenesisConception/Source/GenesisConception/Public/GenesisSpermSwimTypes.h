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
};

/** Stellschrauben des Schwimmmodells. Bandbreiten aus CASA-Referenzwerten menschlicher Spermien. */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisSpermSwimTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveSpeedUm = FFloatInterval(30.0f, 55.0f);
	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveBeatHz = FFloatInterval(12.0f, 18.0f);
	UPROPERTY(EditAnywhere, Category = "Progressive") FFloatInterval ProgressiveHeadAmplitudeUm = FFloatInterval(2.5f, 5.0f);
	UPROPERTY(EditAnywhere, Category = "Progressive") float ProgressiveWavelengthUm = 30.0f;
	/** Rotationsdiffusion (rad²/s). */
	UPROPERTY(EditAnywhere, Category = "Progressive") float ProgressiveRotationalDiffusion = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperSpeedUm = FFloatInterval(8.0f, 20.0f);
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperBeatHz = FFloatInterval(7.0f, 10.0f);
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") FFloatInterval HyperHeadAmplitudeUm = FFloatInterval(9.0f, 14.0f);
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") float HyperWavelengthUm = 45.0f;
	UPROPERTY(EditAnywhere, Category = "Hyperactivated") float HyperRotationalDiffusion = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Sluggish") FFloatInterval SluggishSpeedUm = FFloatInterval(3.0f, 12.0f);
	UPROPERTY(EditAnywhere, Category = "Sluggish") FFloatInterval SluggishBeatHz = FFloatInterval(3.0f, 7.0f);
	/** Unter dieser Vitalität schwimmt eine Zelle träge. */
	UPROPERTY(EditAnywhere, Category = "Sluggish") float SluggishVitalityThreshold = 0.3f;

	/** Wechselraten zwischen progressiv und hyperaktiviert (je Sekunde). */
	UPROPERTY(EditAnywhere, Category = "Transitions") float HyperactivationRate = 0.02f;
	UPROPERTY(EditAnywhere, Category = "Transitions") float DeactivationRate = 0.05f;

	/** Gegen die Strömung ausrichten (rad/s bei voller Wandscherung). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float RheotaxisTurnRate = 0.8f;
	/** Ab diesem Wandabstand wirkt die hydrodynamische Wandbindung (µm). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallAttractionDistanceUm = 25.0f;
	/** 0..1 – wie stark eine Zelle an der Wand gehalten wird (hyperaktivierte Zellen lösen sich leichter). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallTrapStrength = 0.85f;
	/** An Oberflächen schwimmen Spermien leicht zur Wand geneigt – dadurch bleiben sie lange dort (Grad). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallTiltDegrees = 3.5f;
	/** Mindestabstand Kopfspitze–Wand (µm). */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float WallMarginUm = 3.0f;
	/** Rollen um die Längsachse relativ zur Schlagfrequenz. */
	UPROPERTY(EditAnywhere, Category = "Behaviour") float RollPerBeat = 0.02f;

	/** Feste Simulationsschrittweite (s Simulationszeit). */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.001")) float FixedStepSeconds = 1.0f / 240.0f;
};
