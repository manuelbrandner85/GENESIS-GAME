// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisSliceTypes.generated.h"

/**
 * Der Vertical Slice als ein Durchlauf.
 *
 * Bis hierher gab es die Teile: die Befruchtung, die erste Woche, die Geburt, die erste Stunde.
 * Jeder Teil lief für sich, gestartet über Konsolenbefehle. Was fehlte, war das, worum es geht –
 * **ein Leben am Stück**: von der Verschmelzung zweier Zellen bis zu dem Moment, in dem ein Mensch
 * satt und warm einschläft.
 *
 * Die Regie erfindet dabei nichts. Sie schaut zu, was die Systeme melden, und entscheidet nur,
 * wann gewartet, wann gesprungen und wann der Ort gewechselt wird.
 */
UENUM(BlueprintType)
enum class EGenesisSlicePhase : uint8
{
	/** Kein Durchlauf. */
	Idle,
	/** Der Schwarm im Eileiter: Die Befruchtung steht noch aus. */
	Conception,
	/** Die erste Woche: Furchung, Morula, Blastozyste, Schlüpfen, Einnistung. */
	Embryo,
	/** Schwangerschaft: neun Monate, die im Zeitraffer vergehen. */
	Gestation,
	/** Die Geburt. */
	Birth,
	/** Die erste Stunde. */
	FirstHour,
	/** Angekommen: Das Kind ist warm, satt und schläft. */
	Complete,
	/** Abgebrochen – meistens, weil das Leben nicht zustande kam. */
	Ended
};

/** Warum ein Durchlauf geendet hat. */
UENUM(BlueprintType)
enum class EGenesisSliceEnding : uint8
{
	/** Läuft noch. */
	None,
	/** Das Kind schläft: der vorgesehene Schluss. */
	Asleep,
	/** Die erste Stunde ist vorbei, das Kind aber nicht zur Ruhe gekommen. Auch das ist ein Ausgang. */
	Unsettled,
	/** Der Keim hat sich nicht weiterentwickelt. Das ist kein Fehler, das ist Biologie. */
	EmbryoArrested,
	/** Das Kind hat die Geburt nicht überlebt. */
	NotAlive,
	/** Von Hand abgebrochen. */
	Aborted
};

/** Was die Systeme gerade melden. Daraus – und nur daraus – entscheidet die Regie. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceSignals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bConceived = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bImplanted = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bEmbryoArrested = false;

	/** Schwangerschaftswoche des Kindes. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float GestationalWeeks = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bLaborRunning = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bBorn = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bAlive = true;

	/** Minuten seit der Geburt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float MinutesSinceBirth = 0.0f;

	/** true, sobald das Kind in der ersten Stunde eingeschlafen ist. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bAsleep = false;
};

/** Zustand eines Durchlaufs. Persistiert – ein Slice überlebt das Speichern. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") EGenesisSlicePhase Phase = EGenesisSlicePhase::Idle;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") EGenesisSliceEnding Ending = EGenesisSliceEnding::None;

	UPROPERTY() FGuid EntityId;
	UPROPERTY() uint64 RunSeed = 0;

	/** Zeitpunkt, an dem der Durchlauf begonnen hat. */
	UPROPERTY() FGenesisTimestamp StartTime;

	/** Wie lange der Durchlauf in echter Zeit schon läuft (s) – für Anzeige und Messung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float RealSeconds = 0.0f;

	/** Echtzeit in der aktuellen Phase (s). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float PhaseRealSeconds = 0.0f;

	bool IsRunning() const { return Phase != EGenesisSlicePhase::Idle && Phase != EGenesisSlicePhase::Complete && Phase != EGenesisSlicePhase::Ended; }
};

/** Stellschrauben der Regie. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceTuning
{
	GENERATED_BODY()

	/** Ab dieser Schwangerschaftswoche beginnt die Geburt. */
	UPROPERTY(EditAnywhere, Category = "Slice") float BirthAtWeeks = 39.0f;

	/** Wie viele Stunden die Regie je Schritt überspringt, solange die erste Woche läuft. */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "1", ClampMax = "24")) int32 EmbryoSkipHours = 6;

	/** Echtzeit zwischen zwei Schritten der ersten Woche (s). */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0", ClampMax = "5")) float EmbryoStepSeconds = 0.25f;

	/** Wie viele Tage die Regie je Schritt überspringt, solange die Schwangerschaft läuft. */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "1", ClampMax = "60")) int32 GestationSkipDays = 7;

	/** Echtzeit zwischen zwei Sprüngen (s) – so bleibt die Schwangerschaft ein Verlauf und kein Schnitt. */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0", ClampMax = "5")) float GestationStepSeconds = 0.35f;

	/** Simulationsminuten je Sekunde in der Geburt. */
	UPROPERTY(EditAnywhere, Category = "Slice") float BirthTimeScale = 90.0f;

	/** Simulationsminuten je Sekunde in der ersten Stunde. */
	UPROPERTY(EditAnywhere, Category = "Slice") float FirstHourTimeScale = 2.0f;

	/**
	 * Nach dieser Zeit legt die Hebamme das Kind auf die Haut der Mutter und spricht mit ihm (min).
	 * Das ist keine Entscheidung des Spielers, sondern das, was in einem Kreißsaal ohnehin geschieht –
	 * solange es keine Eingabe gibt, handelt die Welt. Mit `genesis.Newborn.SkinToSkin 0` lässt sich
	 * das Gegenteil erzwingen, und dann kühlt das Kind aus.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float SkinContactAfterMinutes = 2.0f;

	/**
	 * Um wie viele Minuten jede gerufene Minute die Versorgung vorzieht.
	 * Ein Kind, das schreit, wird schneller geholt – das ist der einzige Hebel, den es hat.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float CallShortensCare = 4.0f;

	/** Spätestens nach dieser Zeit endet die erste Stunde, auch wenn das Kind nicht schläft (min). */
	UPROPERTY(EditAnywhere, Category = "Slice") float FirstHourLimitMinutes = 75.0f;

	/**
	 * Weltsekunden je Echtzeitsekunde in den Szenen, die man wirklich erlebt (Geburt, erste Stunde).
	 * Dort soll nur die Phase selbst beschleunigen – sonst laufen zwei Zeitraffer übereinander.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float SceneClockTimeScale = 1.0f;

	/** Weltsekunden je Echtzeitsekunde außerhalb dieser Szenen. */
	UPROPERTY(EditAnywhere, Category = "Slice") float DefaultClockTimeScale = 60.0f;

	/** Die Karten des Durchlaufs. */
	UPROPERTY(EditAnywhere, Category = "Slice") FName ConceptionMap = TEXT("L_GEN_OviductAmpulla");
	UPROPERTY(EditAnywhere, Category = "Slice") FName BirthMap = TEXT("L_GEN_Birth");
};
