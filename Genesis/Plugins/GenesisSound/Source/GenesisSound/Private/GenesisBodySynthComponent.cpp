// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodySynthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisAudioTypes.h"
#include "GenesisBirthSubsystem.h"

UGenesisBodySynthComponent::UGenesisBodySynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	NumChannels = 1;
}

bool UGenesisBodySynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	Synth.Initialize(static_cast<float>(SampleRate));
	return true;
}

int32 UGenesisBodySynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	{
		FScopeLock Lock(&ParamsLock);
		if (bParamsDirty)
		{
			Synth.SetParams(PendingParams);
			bParamsDirty = false;
		}
	}

	Synth.Render(OutAudio, NumSamples);
	return NumSamples;
}

void UGenesisBodySynthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bFollowSimulation)
	{
		UpdateFromSimulation();
	}
}

void UGenesisBodySynthComponent::SetSoundParams(const FGenesisBodySoundParams& InParams)
{
	FScopeLock Lock(&ParamsLock);
	PendingParams = InParams;
	bParamsDirty = true;
}

FGenesisBodySoundParams UGenesisBodySynthComponent::GetSoundParams() const
{
	FScopeLock Lock(&ParamsLock);
	return PendingParams;
}

void UGenesisBodySynthComponent::UpdateFromSimulation()
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	if (!Audio)
	{
		return;
	}

	const FGenesisHearingPerception& Hearing = Audio->GetHearingPerception();
	const FGenesisBodyAudioParams& Body = Audio->GetBodyAudioParams();

	FGenesisBodySoundParams Params;
	Params.HeartRateBpm = Body.HeartRateBpm > 0.0f ? Body.HeartRateBpm : 140.0f;
	Params.MotherHeartRateBpm = Body.bMaternalSounds ? Body.MaternalHeartRateBpm : 0.0f;
	Params.BreathsPerMinute = Body.BreathRate > 0.0f ? Body.BreathRate : 14.0f;
	Params.bInWomb = Hearing.bInWomb;
	Params.LowPassCutoffHz = Hearing.LowPassCutoffHz;
	// Der eigene Körper ist im Mutterleib das Lauteste; nach der Geburt tritt er hinter die Welt zurück
	Params.BodyAudibility = FMath::Max(Hearing.BodyAudibility, Body.HeartAudibility);
	Params.ExternalAudibility = Hearing.ExternalAudibility;

	// Unter der Geburt kommen Druck und Sauerstoffmangel dazu
	if (const UGenesisBirthSubsystem* Birth = GameInstance->GetSubsystem<UGenesisBirthSubsystem>())
	{
		if (Birth->HasLabor())
		{
			const FGenesisBirthPerception Perception = Birth->GetPerception();
			Params.Pressure = Perception.Pressure;
			Params.Oxygen = Perception.Oxygen;
			Params.HeartRateBpm = Perception.HeartRateBpm;
		}
	}

	SetSoundParams(Params);
}
