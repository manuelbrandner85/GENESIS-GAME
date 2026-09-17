// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisTypes.h"
#include "GenesisMemoryTrace.generated.h"

struct FGenesisCausalEvent;
struct FGenesisRandomStream;

/** Aus welcher Perspektive eine Erinnerung entstand. */
UENUM(BlueprintType)
enum class EGenesisMemoryPerspective : uint8
{
	Actor,
	Target,
	Participant,
	Witness,
	/** Nur davon gehört (Gerücht, Erzählung). */
	Hearsay,
	/** Familienerzählung über Generationen. */
	Inherited
};

/**
 * Subjektive Erinnerungsspur einer Person an ein Ereignis.
 * Keine perfekten Daten: Intensität, Genauigkeit und Gefühl verändern sich; Wahrnehmung kann falsch sein.
 */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisMemoryTrace
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid TraceId;

	/** Objektives Ereignis im Kausalgraph. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGenesisTimestamp EncodedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGenesisTimestamp LastRecalledAt;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	EGenesisMemoryPerspective Perspective = EGenesisMemoryPerspective::Witness;

	/** 0..1 – Lebendigkeit und Abrufbarkeit. */
	UPROPERTY()
	float Intensity = 0.0f;

	/** 0..1 – Nähe zur tatsächlichen Ereignisversion. */
	UPROPERTY()
	float Accuracy = 1.0f;

	/** −1..1 – subjektives Gefühl (kann sich beim Erinnern verschieben). */
	UPROPERTY()
	float Valence = 0.0f;

	/** 0..1 – emotionale Aktivierung beim Einprägen. Hoch = langlebig. */
	UPROPERTY()
	float Arousal = 0.0f;

	/** 0..1 – Verdrängung. Verdrängte Spuren wirken im Unterbewusstsein weiter. */
	UPROPERTY()
	float Repression = 0.0f;

	UPROPERTY()
	int32 RecallCount = 0;

	/** Wahrgenommene Themen – können von der Wahrheit abweichen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTagContainer PerceivedThemes;

	/** Wer es aus Sicht dieser Person getan hat – kann falsch sein. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid PerceivedActorId;

	/** Sinnesreize, die diese Erinnerung später wecken können (Genesis.Sense.Smell.Rain …). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTagContainer SensoryCues;

	/** Konkreter Erinnerungssatz ("Er blieb bei mir, als meine Mutter starb."). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FText Narrative;

	UPROPERTY()
	bool bDistorted = false;
};

/** Wie eine Person ein Ereignis in diesem Moment erlebt. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisEncodingContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	EGenesisMemoryPerspective Perspective = EGenesisMemoryPerspective::Witness;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	float Attention = 1.0f;

	/** 0..1 – mittlerer Stress schärft, hoher Stress verzerrt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	float Stress = 0.0f;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	float Arousal = 0.3f;

	/** −1..1 – aktuelle Stimmung färbt die Erinnerung. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	float Mood = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGameplayTagContainer SensoryCues;

	/** Vorgegebene Fehlwahrnehmung (z. B. aus dem Missverständnis-System). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	bool bHasPerceptionOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGameplayTagContainer PerceivedThemesOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGuid PerceivedActorOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FText Narrative;
};

/** Zerfalls- und Abrufregeln. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisMemoryDynamicsParams
{
	GENERATED_BODY()

	/** Halbwertszeit einer beiläufigen Erinnerung in Jahren. */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0.01"))
	double BaseHalfLifeYears = 1.5;

	/** Zusätzliche Halbwertszeit bei maximaler emotionaler Aktivierung (quadratisch gewichtet). */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0"))
	double EmotionalHalfLifeBonusYears = 40.0;

	/** Zusätzliche Halbwertszeit pro Verdopplung der Abrufe. */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0"))
	double RecallHalfLifeBonusYears = 2.0;

	/** Genauigkeitsverlust pro Jahr (Rekonstruktion statt Abspielen). */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0", ClampMax = "1"))
	float AccuracyDriftPerYear = 0.02f;

	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0", ClampMax = "1"))
	float MinimumAccuracy = 0.1f;

	/** Unter dieser Intensität gilt eine Erinnerung als vergessen. */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0", ClampMax = "1"))
	float ForgetThreshold = 0.02f;

	/** Ab dieser Aktivierung wird nie ganz vergessen – die Spur ruht nur (Trauma, große Liebe). */
	UPROPERTY(EditAnywhere, Category = "Decay", meta = (ClampMin = "0", ClampMax = "1"))
	float DormantArousalThreshold = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Recall", meta = (ClampMin = "0", ClampMax = "1"))
	float RecallIntensityGain = 0.2f;

	/** Jeder Abruf rekonstruiert – und verfälscht minimal. */
	UPROPERTY(EditAnywhere, Category = "Recall", meta = (ClampMin = "0", ClampMax = "1"))
	float RecallAccuracyCost = 0.01f;

	/** Wie stark die aktuelle Stimmung das erinnerte Gefühl umfärbt. */
	UPROPERTY(EditAnywhere, Category = "Recall", meta = (ClampMin = "0", ClampMax = "1"))
	float RecallMoodPull = 0.08f;

	/** Gewicht von Geruchsreizen beim Wecken von Erinnerungen (Proust-Effekt). */
	UPROPERTY(EditAnywhere, Category = "Recall", meta = (ClampMin = "1"))
	float SmellCueWeight = 2.0f;
};

/** Kandidat eines durch Reize geweckten Erinnerns. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisRecallCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid TraceId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	float Score = 0.0f;

	/** true = Reiz war stark genug für einen Flashback. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	bool bFlashback = false;
};

/** Alle Erinnerungen einer Person. */
USTRUCT()
struct GENESISMEMORY_API FGenesisMemoryStore
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	TArray<FGenesisMemoryTrace> Traces;

	FGenesisMemoryTrace* FindTrace(const FGuid& TraceId);
	const FGenesisMemoryTrace* FindTrace(const FGuid& TraceId) const;
	const FGenesisMemoryTrace* FindTraceForEvent(const FGuid& EventId) const;
};

/** Zustandslose Erinnerungslogik. */
namespace GenesisMemoryLogic
{
	/**
	 * Prägt ein Ereignis ein. Existiert bereits eine Spur zum selben Ereignis, wird sie verstärkt statt dupliziert.
	 * Verzerrung ist zufällig, aber deterministisch über Rng.
	 */
	GENESISMEMORY_API FGenesisMemoryTrace& Encode(FGenesisMemoryStore& Store, const FGenesisCausalEvent& Event, const FGenesisEncodingContext& Context,
		const FGenesisTimestamp& Now, FGenesisRandomStream& Rng);

	/** Lässt alle Spuren über die vergangene Zeit altern. @return Anzahl vergessener Spuren */
	GENESISMEMORY_API int32 Decay(FGenesisMemoryStore& Store, double ElapsedYears, const FGenesisMemoryDynamicsParams& Params);

	/** Bewusstes Erinnern (Foto, Gespräch, Ort). Verstärkt, verfälscht minimal, färbt mit der Stimmung. */
	GENESISMEMORY_API bool Recall(FGenesisMemoryStore& Store, const FGuid& TraceId, const FGenesisTimestamp& Now, float CurrentMood, const FGenesisMemoryDynamicsParams& Params);

	/** Reize (Geruch, Musik, Ort, Thema) wecken Erinnerungen. Verdrängte Spuren werden gedämpft. */
	GENESISMEMORY_API TArray<FGenesisRecallCandidate> FindByCues(const FGenesisMemoryStore& Store, const FGameplayTagContainer& SensoryCues,
		const FGameplayTagContainer& Themes, int32 MaxResults, float MinScore, const FGenesisMemoryDynamicsParams& Params);

	/** Verdrängung (+) oder Aufarbeitung durch Therapie, Meditation, Krise, Wahrheit (−). */
	GENESISMEMORY_API bool AdjustRepression(FGenesisMemoryStore& Store, const FGuid& TraceId, float Delta);

	/** 0..1 – wie nah die Erinnerung an der Wahrheit liegt (Jenseits: tatsächliche Version sichtbar machen). */
	GENESISMEMORY_API float CompareWithTruth(const FGenesisMemoryTrace& Trace, const FGenesisCausalEvent& Event);
}
