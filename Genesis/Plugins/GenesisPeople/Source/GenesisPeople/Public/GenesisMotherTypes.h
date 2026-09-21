// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisMotherTypes.generated.h"

/**
 * Die Mutter in der ersten Stunde.
 *
 * Sie ist keine Kulisse: Sie atmet, und das Kind liegt auf diesem Atem. Sie blinzelt, und sie sieht
 * das Kind an, fast ununterbrochen. Hebt das Kind den Blick, holt sie es zu sich hoch, bis ihre
 * Gesichter sich auf eine Unterarmlänge gegenüber sind – in der Entwicklungspsychologie heißt das
 * „en face“, und die Entfernung ist kein Zufall: 20 bis 30 cm ist genau der Abstand, auf den ein
 * Neugeborenes scharf sieht.
 */

/** Stellschrauben. Alle Werte mit Herkunft. */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherTuning
{
	GENERATED_BODY()

	/**
	 * Atemzüge je Minute. In Ruhe atmen Erwachsene 12–20-mal; nach der Geburt, erschöpft und
	 * entspannt, eher am unteren Rand. Muss zum Atem passen, auf dem die Kamera des Kindes liegt.
	 */
	UPROPERTY(EditAnywhere, Category = "Breath", meta = (ClampMin = "6", ClampMax = "30"))
	float BreathsPerMinute = 14.0f;

	/** Anteil des Einatmens am Atemzug. Einatmen ist aktiv und kürzer, Ausatmen passiv und länger (etwa 1 : 1,5). */
	UPROPERTY(EditAnywhere, Category = "Breath", meta = (ClampMin = "0.2", ClampMax = "0.6"))
	float InhaleFraction = 0.4f;

	/**
	 * Lidschläge je Minute. In Ruhe 15–20, beim aufmerksamen Hinsehen deutlich weniger – die Rate
	 * sinkt, sobald die Augen etwas festhalten wollen. Eine Mutter, die ihr Kind ansieht, liegt dazwischen.
	 */
	UPROPERTY(EditAnywhere, Category = "Eyes", meta = (ClampMin = "2", ClampMax = "40"))
	float BlinksPerMinute = 12.0f;

	/** Beim Blickkontakt sinkt die Rate weiter (Anteil der normalen Rate). */
	UPROPERTY(EditAnywhere, Category = "Eyes", meta = (ClampMin = "0.2", ClampMax = "1"))
	float EyeContactBlinkFactor = 0.6f;

	/** Dauer eines Lidschlags (s). Gemessen 0,1–0,4 s; das Lid schließt schnell und öffnet langsamer. */
	UPROPERTY(EditAnywhere, Category = "Eyes", meta = (ClampMin = "0.08", ClampMax = "0.5"))
	float BlinkSeconds = 0.25f;

	/** Kürzester Abstand zwischen zwei Lidschlägen (s). */
	UPROPERTY(EditAnywhere, Category = "Eyes", meta = (ClampMin = "0.2"))
	float MinBlinkIntervalSeconds = 0.9f;

	/** Wie lange es dauert, das Kind von der Brust vor das Gesicht zu heben (s). Ruhig, beide Hände am Kind. */
	UPROPERTY(EditAnywhere, Category = "Hold", meta = (ClampMin = "0.5"))
	float EnFaceSeconds = 2.8f;

	/**
	 * Abstand Gesicht zu Gesicht in der En-face-Haltung (mm). Beobachtet werden 20–30 cm – und ein
	 * Neugeborenes sieht auf etwa 25 cm scharf. Mütter treffen diesen Abstand, ohne ihn zu kennen.
	 */
	UPROPERTY(EditAnywhere, Category = "Hold", meta = (ClampMin = "150", ClampMax = "450"))
	float EnFaceDistanceMm = 250.0f;

	/** Ein weiches, zufriedenes Grundgesicht – kein Dauerlächeln. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0", ClampMax = "1"))
	float RestingSmile = 0.1f;

	/** Lächeln beim Blickkontakt. Echt ist es nie breit, sondern warm – Mundwinkel und Wangen. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0", ClampMax = "1"))
	float EyeContactSmile = 0.45f;

	/** Wie schnell das Lächeln kommt und geht (je Sekunde). Es kommt schneller, als es geht. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0.05"))
	float SmileRisePerSecond = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0.05"))
	float SmileFallPerSecond = 0.2f;
};

/** Was die Welt der Mutter in diesem Moment sagt. */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherInputs
{
	GENERATED_BODY()

	/** Das Kind liegt auf ihr (und nicht mehr in den Händen der Hebamme). */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Mother") bool bChildOnChest = false;

	/** Das Kind sucht ihr Gesicht – sie holt es zu sich hoch. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Mother") bool bChildSeeksFace = false;

	/** Der Blick des Kindes liegt auf ihren Augen. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Mother") bool bChildLooksAtEyes = false;
};

/** Zustand der Mutter. Läuft nur im Spiel mit, wird nicht gespeichert – er entsteht in jedem Moment neu. */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherState
{
	GENERATED_BODY()

	/** Atemphase in Zügen (0..1 je Zug, läuft weiter). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float BreathPhase = 0.0f;

	/** Sekunden bis zum nächsten Lidschlag. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float NextBlinkIn = 2.0f;

	/** Sekunden seit Beginn des laufenden Lidschlags, negativ = kein Lidschlag. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float BlinkElapsed = -1.0f;

	/** 0 = Kind auf der Brust, 1 = Kind vor ihrem Gesicht (linear, geglättet wird beim Auslesen). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float EnFace = 0.0f;

	/** 0..1 – Lächeln. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float Smile = 0.1f;

	/** Sekunden ununterbrochenen Blickkontakts. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float EyeContactSeconds = 0.0f;

	/** Gezählte Lidschläge (für Tests und Anzeige). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") int32 Blinks = 0;

	/** Zufall für die Abstände der Lidschläge. Deterministisch, damit Tests reproduzierbar sind. */
	uint32 RandomState = 0x9E3779B9u;
};
