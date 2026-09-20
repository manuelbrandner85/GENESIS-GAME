// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisEarlyLifeTypes.generated.h"

/**
 * Die ersten Minuten eines Lebens.
 *
 * Ein Neugeborenes ist weder blind noch hilflos: Es sieht auf Armlänge, erkennt die Stimme,
 * die es neun Monate lang gehört hat, und sucht von selbst die Brust. Was es nicht kann,
 * ist seine Wärme halten – das ist in der ersten Stunde die eigentliche Gefahr.
 */
UENUM(BlueprintType)
enum class EGenesisNewbornStage : uint8
{
	/** Noch nicht geboren. */
	NotBorn,
	/** Die erste Minute: Der Atem kommt in Gang, die Haut färbt sich, alles ist zu laut und zu hell. */
	FirstBreaths,
	/** Die ruhige Wachheit: eine knappe Stunde, in der das Kind so aufmerksam ist wie danach lange nicht mehr. */
	QuietAlert,
	/** Haut an Haut: Wärme, Herzschlag, Geruch – das Kind beruhigt sich. */
	SkinContact,
	/** Das erste Anlegen. Das Kind findet die Brust von selbst. */
	FirstFeed,
	/** Der erste Schlaf. */
	FirstSleep,
	/** Unterkühlt – ohne Wärme von außen endet die erste Stunde gefährlich. */
	Hypothermic
};

/** Was das Neugeborene in diesem Moment wahrnimmt. Geht an Kamera, Nachbearbeitung und Ton. */
USTRUCT(BlueprintType)
struct GENESISEARLYLIFE_API FGenesisNewbornPerception
{
	GENERATED_BODY()

	/** Entfernung, auf die das Kind scharf sieht (mm). Anfangs etwa 25 cm – genau ein Gesicht am Arm. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float FocusDistanceMm = 250.0f;

	/** 0..1 – Sehschärfe. Ein Neugeborenes sieht mit etwa 20/400, also 0,05. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float VisualAcuity = 0.05f;

	/** 0..1 – Helligkeitsempfinden. Die erste Minute ist blendend hell. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Glare = 1.0f;

	/** 0..1 – Wärme. 0 ist auskühlen, 1 ist Haut an Haut. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Warmth = 0.2f;

	/** 0..1 – Ruhe. Steigt mit Wärme, Stimme und Sattsein. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Calm = 0.2f;

	/** 0..1 – Hunger. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Hunger = 0.0f;

	/** 0..1 – wie vertraut die gehörte Stimme ist. Die Stimme der Mutter kennt das Kind aus dem Mutterleib. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float VoiceFamiliarity = 0.0f;

	/** 0..1 – Müdigkeit. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Sleepiness = 0.0f;
};

/** Zustand der ersten Stunde. Persistiert. */
USTRUCT(BlueprintType)
struct GENESISEARLYLIFE_API FGenesisNewbornState
{
	GENERATED_BODY()

	UPROPERTY() FGuid EntityId;
	UPROPERTY() FGuid MotherId;
	UPROPERTY() uint64 Seed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife")
	EGenesisNewbornStage Stage = EGenesisNewbornStage::NotBorn;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") double MinutesSinceBirth = 0.0;

	/** Körpertemperatur in °C. Ein Neugeborenes verliert ohne Hilfe bis zu 0,3 °C je Minute. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float BodyTemperature = 37.2f;

	/** true, sobald das Kind abgetrocknet und zugedeckt auf der Haut der Mutter liegt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") bool bSkinToSkin = false;

	/** true, solange jemand mit dem Kind spricht. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") bool bMotherSpeaking = false;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Hunger = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Calm = 0.15f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Sleepiness = 0.0f;

	/** 0..1 – Bindung. Sie entsteht aus Wärme, Stimme, Geruch und Sattsein, nicht aus Zeit allein. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float Bonding = 0.0f;

	/** 0..1 – Sehschärfe, die in den ersten Minuten nur wenig zunimmt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float VisualAcuity = 0.04f;

	/** Minuten, die das Kind geschrien hat. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") float CryingMinutes = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") bool bHasFed = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|EarlyLife") bool bFirstMemoryEncoded = false;

	UPROPERTY() FGenesisTimestamp BirthTime;

	bool IsAlive() const { return Stage != EGenesisNewbornStage::NotBorn; }
	bool IsCrying() const { return Calm < 0.35f && Stage != EGenesisNewbornStage::FirstSleep; }
};

/** Stellschrauben der ersten Stunde. */
USTRUCT(BlueprintType)
struct GENESISEARLYLIFE_API FGenesisEarlyLifeTuning
{
	GENERATED_BODY()

	/** Raumtemperatur im Kreißsaal (°C). */
	UPROPERTY(EditAnywhere, Category = "Temperature") float RoomTemperature = 24.0f;

	/**
	 * Abkühlung als Anteil des Temperaturgefälles je Minute (Newtonsches Abkühlungsgesetz).
	 * Ein nasses Neugeborenes verliert anfangs etwa 0,25 °C je Minute – aber nicht gleichmäßig weiter,
	 * sondern immer langsamer, je näher es der Raumtemperatur kommt.
	 */
	UPROPERTY(EditAnywhere, Category = "Temperature") float CoolingRatePerMinute = 0.019f;

	/** Erwärmung auf der Haut der Mutter, ebenfalls als Anteil des Gefälles je Minute. */
	UPROPERTY(EditAnywhere, Category = "Temperature") float SkinContactRatePerMinute = 0.045f;

	/** Hauttemperatur der Mutter (°C) – mehr als sie kann das Kind nicht bekommen. */
	UPROPERTY(EditAnywhere, Category = "Temperature") float TargetTemperature = 37.0f;

	/** Unter dieser Temperatur gilt das Kind als unterkühlt (°C). */
	UPROPERTY(EditAnywhere, Category = "Temperature") float HypothermiaTemperature = 35.5f;

	/** Dauer der ersten Atemzüge (min). */
	UPROPERTY(EditAnywhere, Category = "Timing") float FirstBreathsMinutes = 2.0f;

	/** Dauer der ruhigen Wachheit (min). Danach wird das Kind müde. */
	UPROPERTY(EditAnywhere, Category = "Timing") float QuietAlertMinutes = 55.0f;

	/** Minuten bis zum ersten Anlegen, wenn das Kind auf der Haut liegt (der "Brustkrabbelgang"). */
	UPROPERTY(EditAnywhere, Category = "Timing") float MinutesToFirstFeed = 32.0f;

	/** Hunger je Minute (0..1). */
	UPROPERTY(EditAnywhere, Category = "Needs") float HungerPerMinute = 0.02f;

	/** Ruhe je Minute durch Hautkontakt bzw. Verlust ohne ihn. */
	UPROPERTY(EditAnywhere, Category = "Needs") float CalmGainPerMinute = 0.09f;
	UPROPERTY(EditAnywhere, Category = "Needs") float CalmLossPerMinute = 0.06f;

	/** Zusätzliche Ruhe, wenn die vertraute Stimme spricht. */
	UPROPERTY(EditAnywhere, Category = "Needs") float VoiceCalmPerMinute = 0.05f;

	/** Bindung je Minute unter guten Bedingungen – mit Sättigung, die erste Stunde kommt nicht auf 1. */
	UPROPERTY(EditAnywhere, Category = "Needs") float BondingPerMinute = 0.016f;

	/** Ab dieser Ruhe schläft das Kind ein, wenn es müde und satt ist. */
	UPROPERTY(EditAnywhere, Category = "Needs", meta = (ClampMin = "0", ClampMax = "1")) float SleepCalmThreshold = 0.7f;
};
