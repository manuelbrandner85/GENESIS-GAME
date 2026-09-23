// GENESIS: Der Kreislauf des Lebens

#include "GenesisWorldSoundComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisAudioCoreLogic.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisAudioTypes.h"

UGenesisWorldSoundComponent::UGenesisWorldSoundComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	NumChannels = 1;
}

bool UGenesisWorldSoundComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	SynthSampleRate = static_cast<float>(SampleRate);
	Synth.Initialize(SynthSampleRate);
	return true;
}

void UGenesisWorldSoundComponent::SetWorldSoundParams(const FGenesisWorldSoundParams& InParams)
{
	FScopeLock ScopeLock(&Lock);
	PendingParams = InParams;
	bParamsDirty = true;
}

FGenesisWorldSoundParams UGenesisWorldSoundComponent::GetWorldSoundParams() const
{
	FScopeLock ScopeLock(&Lock);
	return PendingParams;
}

void UGenesisWorldSoundComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	if (!Audio)
	{
		return;
	}

	const FGenesisHearingPerception& Perception = Audio->GetHearingPerception();
	const float MixGain = bFollowMix ? Audio->GetBusGainLinear(EGenesisAudioBus::Ambient) : 1.0f;

	FScopeLock ScopeLock(&Lock);
	if (bHeardByUnborn)
	{
		Hearing.CutoffHz = Perception.LowPassCutoffHz;
		Hearing.HighPassHz = Perception.HighPassCutoffHz;
		Hearing.Gain = GenesisAudioCoreLogic::UnbornPresentationGain(Perception.BodyAudibility) * MixGain;
	}
	else if (bHeardFromOutside)
	{
		Hearing.CutoffHz = Perception.LowPassCutoffHz;
		Hearing.HighPassHz = Perception.HighPassCutoffHz;
		Hearing.Gain = FMath::Clamp(Perception.ExternalAudibility, 0.0f, 2.0f) * MixGain;
	}
	else
	{
		// Der Mutterleib von innen ist kein Geräusch von draußen – er ist der Ort selbst
		Hearing.CutoffHz = 18000.0f;
		Hearing.HighPassHz = 0.0f;
		Hearing.Gain = MixGain;
	}
}

int32 UGenesisWorldSoundComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	{
		FScopeLock ScopeLock(&Lock);
		if (bParamsDirty)
		{
			Synth.SetParams(PendingParams);
			bParamsDirty = false;
		}
	}

	Synth.Render(OutAudio, NumSamples);

	{
		FScopeLock ScopeLock(&Lock);
		Hearing.Process(OutAudio, NumSamples, SynthSampleRate);
	}

	return NumSamples;
}
