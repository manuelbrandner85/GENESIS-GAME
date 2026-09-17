// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisAudioTypes.generated.h"

/**
 * Mix-Busse in Prioritätsreihenfolge (höchste zuerst).
 * Entspricht den Unreal-Submixes/Sound Classes, die in GENESIS-011 angelegt werden.
 */
UENUM(BlueprintType)
enum class EGenesisAudioBus : uint8
{
	/** Wichtiger Dialog. */
	Dialogue,
	/** Lebenswichtige Gameplay-Signale (Gefahr, Warnung). */
	VitalSignal,
	/** Körpergeräusche (Herz, Atem). */
	Body,
	Foley,
	Ambient,
	Music
};

static constexpr int32 GenesisAudioBusCount = 6;

/** Wie die hörende Person die Welt gerade hört – abgeleitet aus Körper und Zustand. */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisHearingPerception
{
	GENERATED_BODY()

	/** Tiefpass auf allem Äußeren (Mutterleib ~150–800 Hz, gesund 20 kHz). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float LowPassCutoffHz = 20000.0f;

	/** Hochton-Absenkung in dB (Altersschwerhörigkeit), ≤ 0. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float HighShelfGainDb = 0.0f;

	/** 0..1 – Lautheit äußerer Klänge. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float ExternalAudibility = 1.0f;

	/** 0..1 – wie deutlich der eigene Körper (Herz, Atem, Blutfluss) zu hören ist. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float BodyAudibility = 0.05f;

	/** 0..1 – Tunnel-Hören bei Angst/Schock: Umgebung tritt zurück, Körper tritt hervor. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float FocusNarrowing = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float TinnitusLevel = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	bool bInWomb = false;

	/** true, solange ein Umgebungswechsel (z. B. Geburt: Mutterleib → Luft) noch hörbar abläuft. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	bool bInEnvironmentTransition = false;
};

/** Parameter für die prozeduralen Körperklänge (MetaSounds lesen diese Werte). */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisBodyAudioParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float HeartRateBpm = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float HeartStrength = 0.0f;

	/** 0..1 – unregelmäßiger Rhythmus (Verschleiß, Herzerkrankung). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float HeartIrregularity = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float HeartAudibility = 0.0f;

	/** Atemzüge pro Minute (0 vor der Geburt). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float BreathRate = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float BreathDepth = 0.0f;

	/** 0..1 – Atemnot, Keuchen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float BreathStrain = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float BreathAudibility = 0.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float Tremor = 0.0f;

	/** Vor der Geburt: Herzschlag und Blutfluss der Mutter dominieren. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	bool bMaternalSounds = false;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Audio")
	float MaternalHeartRateBpm = 0.0f;
};

/** Eine aktive Klangquelle, die Platz im Mix beansprucht. */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisMixRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Audio")
	EGenesisAudioBus Bus = EGenesisAudioBus::Dialogue;

	/** 0..1 – wie wichtig (ein beiläufiger Satz duckt weniger als ein entscheidender). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Audio")
	float Importance = 1.0f;
};

/** Ducking-Regel: Wenn Trigger aktiv ist, tritt Target zurück. */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisDuckingRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mix")
	EGenesisAudioBus Trigger = EGenesisAudioBus::Dialogue;

	UPROPERTY(EditAnywhere, Category = "Mix")
	EGenesisAudioBus Target = EGenesisAudioBus::Music;

	/** Absenkung bei voller Wichtigkeit (negativ). */
	UPROPERTY(EditAnywhere, Category = "Mix")
	float DuckDb = -8.0f;

	/** Zeitkonstante beim Absenken. */
	UPROPERTY(EditAnywhere, Category = "Mix", meta = (ClampMin = "0.01"))
	float AttackSeconds = 0.35f;

	/** Zeitkonstante beim Zurückkehren – bewusst langsam, damit es nicht "pumpt". */
	UPROPERTY(EditAnywhere, Category = "Mix", meta = (ClampMin = "0.01"))
	float ReleaseSeconds = 1.2f;
};

/** Mix-Einstellungen. */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisAudioMixTuning
{
	GENERATED_BODY()

	FGenesisAudioMixTuning();

	UPROPERTY(EditAnywhere, Category = "Mix")
	TArray<FGenesisDuckingRule> DuckingRules;

	/** Tiefste gemeinsame Absenkung eines Busses. */
	UPROPERTY(EditAnywhere, Category = "Mix")
	float MaxDuckDb = -18.0f;
};

/** Aktueller Mix-Zustand (Gain je Bus in dB). */
USTRUCT()
struct GENESISAUDIOCORE_API FGenesisMixState
{
	GENERATED_BODY()

	FGenesisMixState()
	{
		CurrentGainDb.Init(0.0f, GenesisAudioBusCount);
		TargetGainDb.Init(0.0f, GenesisAudioBusCount);
	}

	UPROPERTY()
	TArray<float> CurrentGainDb;

	UPROPERTY()
	TArray<float> TargetGainDb;

	float GetGainDb(EGenesisAudioBus Bus) const { return CurrentGainDb[static_cast<int32>(Bus)]; }
};

/** Stellschrauben der Hörwahrnehmung. */
USTRUCT(BlueprintType)
struct GENESISAUDIOCORE_API FGenesisHearingTuning
{
	GENERATED_BODY()

	/** Tiefpass im Mutterleib bei unreifem bzw. voll ausgebildetem Gehör. */
	UPROPERTY(EditAnywhere, Category = "Womb") float WombCutoffMinHz = 150.0f;
	UPROPERTY(EditAnywhere, Category = "Womb") float WombCutoffMaxHz = 800.0f;

	/** Höchste Hochton-Absenkung im Alter – bewusst begrenzt, damit das Spiel nie unangenehm klingt. */
	UPROPERTY(EditAnywhere, Category = "Aging") float MaxAgeHighShelfLossDb = -12.0f;
	UPROPERTY(EditAnywhere, Category = "Aging") float AgeHighShelfLossPerYearDb = -0.4f;

	/** Zeitkonstante für Wahrnehmungswechsel (die Geburt nutzt den schnellen Übergang). */
	UPROPERTY(EditAnywhere, Category = "Transition") float PerceptionSmoothingSeconds = 1.5f;
	UPROPERTY(EditAnywhere, Category = "Transition") float BirthTransitionSeconds = 0.25f;
};
