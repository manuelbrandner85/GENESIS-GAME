// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "GenesisHearingFilter.h"
#include "GenesisWorldSoundSynth.h"
#include "GenesisWorldSoundComponent.generated.h"

/**
 * Macht einen Ort hörbar.
 *
 * Die Komponente nimmt zwei Dinge aus der Simulation: **wie** der Hörer gerade hört (Tiefpass des
 * Mutterleibs, Lautheiten) und **wie viel Platz** der Mix der Umgebung gerade lässt. Wenn jemand
 * spricht, tritt der Raum zurück – nicht weil es hübscher klingt, sondern weil ein Mensch genau so hört.
 */
UCLASS(ClassGroup = "Genesis", meta = (BlueprintSpawnableComponent))
class GENESISWORLDSOUND_API UGenesisWorldSoundComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UGenesisWorldSoundComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Genesis|WorldSound")
	void SetWorldSoundParams(const FGenesisWorldSoundParams& InParams);

	UFUNCTION(BlueprintPure, Category = "Genesis|WorldSound")
	FGenesisWorldSoundParams GetWorldSoundParams() const;

	/**
	 * true: Der Ort wird durch die Ohren des Hörers gefiltert (im Mutterleib dumpf, an Luft klar).
	 * Für den Klang **des** Mutterleibs selbst ist das aus – er ist ja schon innen.
	 */
	UPROPERTY(EditAnywhere, Category = "Genesis|WorldSound")
	bool bHeardFromOutside = true;

	/** true: Der Mix darf diesen Ort zurücktreten lassen, wenn jemand spricht. */
	UPROPERTY(EditAnywhere, Category = "Genesis|WorldSound")
	bool bFollowMix = true;

private:
	FGenesisWorldSoundSynth Synth;
	FGenesisHearingFilter Hearing;
	float SynthSampleRate = 48000.0f;

	FGenesisWorldSoundParams PendingParams;
	bool bParamsDirty = true;
	mutable FCriticalSection Lock;
};
