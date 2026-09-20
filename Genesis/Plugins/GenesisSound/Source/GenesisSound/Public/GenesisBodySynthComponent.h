// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "GenesisBodySynth.h"
#include "GenesisBodySynthComponent.generated.h"

/**
 * Macht den Körperklang hörbar.
 *
 * Die Werte holt die Komponente jeden Tick aus der Simulation (Hörwahrnehmung, Körperklang-Parameter,
 * Geburt) und gibt sie an die Synthese weiter. Der Ton wird im Audio-Thread erzeugt, die Werte kommen
 * aus dem Spiel-Thread – deshalb liegt zwischen beiden eine Sperre und keine Vermutung.
 */
UCLASS(ClassGroup = "Genesis", meta = (BlueprintSpawnableComponent))
class GENESISSOUND_API UGenesisBodySynthComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UGenesisBodySynthComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Übernimmt Werte aus der Simulation. Ohne laufende Simulation bleiben die eingestellten Werte stehen. */
	void UpdateFromSimulation();

	/** Parameter von Hand setzen (Entwicklerbefehle, Tests). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Sound")
	void SetSoundParams(const FGenesisBodySoundParams& InParams);

	UFUNCTION(BlueprintPure, Category = "Genesis|Sound")
	FGenesisBodySoundParams GetSoundParams() const;

	/** Werte aus der Simulation ziehen (aus, wenn von Hand gesteuert wird). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Sound")
	bool bFollowSimulation = true;

private:
	FGenesisBodySynth Synth;
	FGenesisBodySoundParams PendingParams;
	mutable FCriticalSection ParamsLock;
	bool bParamsDirty = true;
};
