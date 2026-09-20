// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "GenesisVoiceSynth.h"
#include "GenesisVoiceSynthComponent.generated.h"

/**
 * Macht eine einzelne Stimme hörbar.
 *
 * Die Komponente gehört genau einer sprechenden Person. Sie hört mit, was diese Person sagt,
 * und erzeugt den Laut – gefiltert durch die Ohren dessen, der gerade zuhört. Im Mutterleib
 * ist das derselbe Laut, nur ohne Höhen.
 */
UCLASS(ClassGroup = "Genesis", meta = (BlueprintSpawnableComponent))
class GENESISVOICE_API UGenesisVoiceSynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UGenesisVoiceSynthComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Wessen Stimme diese Komponente ist. */
	void SetSpeaker(const FGuid& InSpeaker) { Speaker = InSpeaker; }
	const FGuid& GetSpeaker() const { return Speaker; }

	/** true, solange gerade ein Laut erklingt. */
	bool IsSpeaking() const;

	/** Die Hörwahrnehmung aus dem Audio Core übernehmen (Tiefpass im Mutterleib, Lautheiten). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Voice")
	bool bFollowHearing = true;

	/** Lautstärke dieser Stimme beim Hörer (Entfernung, Dämpfung). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Voice")
	float Loudness = 1.0f;

private:
	void HandleUtterance(const FGuid& InSpeaker, const FGenesisUtterance& Utterance, const FGenesisVoiceProfile& Profile);

	FGuid Speaker;
	FGenesisVoiceSynth Synth;
	FGenesisVoiceHearing Hearing;
	float SynthSampleRate = 48000.0f;

	FGenesisUtterance PendingUtterance;
	FGenesisVoiceProfile PendingProfile;
	bool bPending = false;
	mutable FCriticalSection Lock;

	FDelegateHandle UtteranceHandle;
};
