// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "GenesisMusicSynth.h"
#include "GenesisMusicSynthComponent.generated.h"

/**
 * Spielt das Leitmotiv der Person, aus deren Ohren gehört wird.
 *
 * Soul Music entscheidet, welche Töne das sind und in welcher Instrumentierung – die Komponente
 * fragt beim Wechsel der Lebensphase neu nach und legt die Phrase auf. Zwischen den Wiederholungen
 * liegt eine Pause: Musik, die ohne Atem durchläuft, wird zur Tapete.
 */
UCLASS(ClassGroup = "Genesis", meta = (BlueprintSpawnableComponent))
class GENESISSOUND_API UGenesisMusicSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UGenesisMusicSynthComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Legt eine Phrase auf (Entwicklerbefehle, Tests). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Music")
	void PlayPhrase(const FGenesisMusicPhrase& Phrase, bool bLoop = true);

	/** Holt das Leitmotiv der hörenden Person und legt es auf. */
	void PlayLeitmotifOfListener();

	/** Lautstärke der Musik gegenüber dem Körperklang. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Music", meta = (ClampMin = "0", ClampMax = "2"))
	float MusicGain = 0.5f;

	/** Pause zwischen zwei Wiederholungen des Motivs (s). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Music")
	float BreathSeconds = 6.0f;

	/** Beim Start und bei jedem Wechsel der Lebensphase das Motiv neu holen. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Music")
	bool bFollowLifePhase = true;

private:
	FGenesisMusicSynth Synth;
	FGenesisMusicPhrase PendingPhrase;
	mutable FCriticalSection PhraseLock;
	bool bPhraseDirty = false;
	bool bPendingLoop = true;

	FGameplayTag LastPhase;
	float SilenceSeconds = 0.0f;
};
