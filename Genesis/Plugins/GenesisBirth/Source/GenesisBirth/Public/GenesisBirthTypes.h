// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisBirthTypes.generated.h"

/**
 * Die Geburt in Abschnitten – aus der Sicht des Kindes, nicht der Mutter.
 *
 * Für das Kind ist eine Wehe kein Schmerz der Mutter, sondern eine Druckwelle: Die Gebärmutter presst,
 * der Mutterkuchen wird schlechter durchblutet, der Sauerstoff sinkt für eine Minute, dann kommt er zurück.
 * Dieses Auf und Ab ist der Rhythmus der letzten Stunden vor dem ersten Atemzug.
 */
UENUM(BlueprintType)
enum class EGenesisLaborStage : uint8
{
	/** Noch keine Wehen. */
	NotStarted,
	/** Eröffnungsphase, früh: der Muttermund öffnet sich bis 3 cm, Wehen alle 10–20 Minuten. */
	Latent,
	/** Eröffnungsphase, aktiv: 3 bis 7 cm, Wehen alle 3–5 Minuten. */
	Active,
	/** Übergangsphase: 7 bis 10 cm, die stärksten Wehen, alle 2–3 Minuten. */
	Transition,
	/** Austreibungsphase: Der Kopf tritt durch den Beckenboden, das Kind dreht sich. */
	Pushing,
	/** Geboren. Der erste Atemzug, der Sprung vom Wasser in die Luft. */
	Delivered
};

/** Was die Geburt erschwert. Bestimmt aus dem Zustand des Kindes, nicht willkürlich gewürfelt. */
UENUM(BlueprintType)
enum class EGenesisBirthComplication : uint8
{
	None,
	/** Die Nabelschnur wird bei jeder Wehe gedrückt – der Sauerstoff fällt tiefer und kommt langsamer zurück. */
	CordCompression,
	/** Das Kind liegt mit dem Steiß voran: Die Austreibung dauert länger. */
	Breech,
	/** Die Schulter bleibt nach dem Kopf hängen – wenige Minuten, die zählen. */
	ShoulderDystocia,
	/** Der Muttermund öffnet sich über Stunden kaum weiter. */
	StalledLabor
};

/**
 * Was das Kind gerade erlebt. Geht direkt an Kamera, Nachbearbeitung und Ton –
 * die Geburt wird nicht erzählt, sondern wahrgenommen.
 */
USTRUCT(BlueprintType)
struct GENESISBIRTH_API FGenesisBirthPerception
{
	GENERATED_BODY()

	/** 0..1 – Druck von allen Seiten. Auf dem Höhepunkt einer Wehe presst die Gebärmutter mit 40–60 mmHg. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Pressure = 0.0f;

	/** 0..1 – Sauerstoffsättigung. Im Mutterleib liegt sie ohnehin niedrig; unter Wehen sinkt sie weiter. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Oxygen = 1.0f;

	/** 0..1 – Licht. Der Geburtskanal ist dunkel; erst beim Durchtritt des Kopfes wird es hell. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Light = 0.0f;

	/** 0..1 – wie dumpf alles klingt (1 = Wasser und Gewebe, 0 = Luft). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float SoundMuffling = 1.0f;

	/** 0..1 – Kälte. Der erste Reiz außerhalb: 37 °C Fruchtwasser gegen 22 °C Raumluft. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Cold = 0.0f;

	/** 0..1 – Enge des Kanals um den Kopf herum. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Tightness = 0.0f;

	/** Herzschlag des Kindes (Schläge/min). Fällt bei jeder Wehe ab und erholt sich danach. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float HeartRateBpm = 140.0f;
};

/** Zustand der Geburt. Persistiert – eine Geburt kann über einen Spielstand hinweg laufen. */
USTRUCT(BlueprintType)
struct GENESISBIRTH_API FGenesisBirthState
{
	GENERATED_BODY()

	UPROPERTY() FGuid EntityId;
	UPROPERTY() uint64 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth")
	EGenesisLaborStage Stage = EGenesisLaborStage::NotStarted;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth")
	EGenesisBirthComplication Complication = EGenesisBirthComplication::None;

	/** Minuten seit der ersten Wehe. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") double MinutesInLabor = 0.0;

	/** Muttermund in cm (0–10). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float DilationCm = 0.0f;

	/** 0..1 – Tiefertreten des Kopfes durch das Becken. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Descent = 0.0f;

	/** 0..1 – Drehung des Kindes im Kanal (es dreht sich um etwa 90°, um durchzupassen). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Rotation = 0.0f;

	/** Sekunden im laufenden Wehenzyklus (Wehe + Pause). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float CycleSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float ContractionIntensity = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") int32 ContractionCount = 0;

	/** 0..1 – Sauerstoff des Kindes. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Oxygen = 1.0f;

	/** 0..1 – aufgelaufene Belastung. Zu viel davon kostet Punkte beim ersten Zustandsbild (Apgar). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float Stress = 0.0f;

	/** Minuten, die das Kind unter der kritischen Sauerstoffschwelle verbracht hat. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float HypoxiaMinutes = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float HeartRateBpm = 140.0f;

	/** Schwangerschaftswochen bei der Geburt und Lungenreife – beides kommt aus der Körpersimulation. */
	UPROPERTY() float GestationalWeeks = 40.0f;
	UPROPERTY() float LungMaturity = 1.0f;

	/** Erstes Zustandsbild nach der Geburt (0–10), erst nach der Geburt gesetzt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") int32 ApgarScore = 0;

	/** Sekunden seit der Geburt – der erste Atemzug braucht ein paar davon. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") float SecondsSinceBirth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Birth") bool bFirstBreath = false;

	UPROPERTY() FGenesisTimestamp BirthTime;

	bool IsInLabor() const { return Stage > EGenesisLaborStage::NotStarted && Stage < EGenesisLaborStage::Delivered; }
	bool IsBorn() const { return Stage == EGenesisLaborStage::Delivered; }
};

/**
 * Stellschrauben der Geburt. Die Zeiten folgen einer ersten Geburt am Termin:
 * Eröffnungsphase 8–12 Stunden, Austreibungsphase 30–60 Minuten.
 */
USTRUCT(BlueprintType)
struct GENESISBIRTH_API FGenesisBirthTuning
{
	GENERATED_BODY()

	/** Öffnung des Muttermunds in cm je Stunde. */
	UPROPERTY(EditAnywhere, Category = "Timing") float LatentDilationPerHour = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Timing") float ActiveDilationPerHour = 1.3f;
	UPROPERTY(EditAnywhere, Category = "Timing") float TransitionDilationPerHour = 2.0f;

	/** Abstand der Wehen (Sekunden, von Beginn zu Beginn) je Abschnitt. */
	UPROPERTY(EditAnywhere, Category = "Timing") float LatentIntervalSeconds = 900.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float ActiveIntervalSeconds = 240.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float TransitionIntervalSeconds = 150.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float PushingIntervalSeconds = 150.0f;

	/** Dauer einer Wehe (Sekunden). */
	UPROPERTY(EditAnywhere, Category = "Timing") float LatentDurationSeconds = 35.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float ActiveDurationSeconds = 55.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float TransitionDurationSeconds = 80.0f;
	UPROPERTY(EditAnywhere, Category = "Timing") float PushingDurationSeconds = 70.0f;

	/** Stärke einer Wehe je Abschnitt (0..1). */
	UPROPERTY(EditAnywhere, Category = "Force") float LatentIntensity = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Force") float ActiveIntensity = 0.65f;
	UPROPERTY(EditAnywhere, Category = "Force") float TransitionIntensity = 0.95f;
	UPROPERTY(EditAnywhere, Category = "Force") float PushingIntensity = 1.0f;

	/** Tiefertreten je Presswehe (0..1). */
	UPROPERTY(EditAnywhere, Category = "Force") float DescentPerContraction = 0.09f;

	/** Sauerstoffabfall auf dem Höhepunkt einer Wehe (Anteil je Minute). */
	UPROPERTY(EditAnywhere, Category = "Oxygen") float OxygenDipPerMinute = 0.95f;

	/** Erholung zwischen den Wehen (Anteil je Minute). */
	UPROPERTY(EditAnywhere, Category = "Oxygen") float OxygenRecoveryPerMinute = 0.55f;

	/** Unter diesem Wert zählt die Zeit als Sauerstoffmangel. */
	UPROPERTY(EditAnywhere, Category = "Oxygen", meta = (ClampMin = "0", ClampMax = "1")) float HypoxiaThreshold = 0.62f;

	/** Ab so vielen Minuten unter der Schwelle sinkt das erste Zustandsbild deutlich. */
	UPROPERTY(EditAnywhere, Category = "Oxygen") float HypoxiaBudgetMinutes = 12.0f;

	/** Sekunden nach der Geburt bis zum ersten Atemzug. */
	UPROPERTY(EditAnywhere, Category = "Birth") float FirstBreathSeconds = 8.0f;

	/** Wahrscheinlichkeit einer Erschwernis (0..1), bei durchschnittlicher Verfassung. */
	UPROPERTY(EditAnywhere, Category = "Risk", meta = (ClampMin = "0", ClampMax = "1")) float ComplicationChance = 0.22f;
};
