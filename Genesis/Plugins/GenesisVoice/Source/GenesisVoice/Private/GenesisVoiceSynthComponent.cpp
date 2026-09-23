// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceSynthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisAudioTypes.h"
#include "GenesisVoiceSubsystem.h"

UGenesisVoiceSynthComponent::UGenesisVoiceSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	NumChannels = 1;
}

bool UGenesisVoiceSynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	SynthSampleRate = static_cast<float>(SampleRate);
	Synth.Initialize(SynthSampleRate);
	return true;
}

void UGenesisVoiceSynthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGenesisVoiceSubsystem* Voice = GameInstance->GetSubsystem<UGenesisVoiceSubsystem>())
		{
			UtteranceHandle = Voice->OnUtterance.AddUObject(this, &UGenesisVoiceSynthComponent::HandleUtterance);
		}
	}
}

void UGenesisVoiceSynthComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGenesisVoiceSubsystem* Voice = GameInstance->GetSubsystem<UGenesisVoiceSubsystem>())
		{
			Voice->OnUtterance.Remove(UtteranceHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void UGenesisVoiceSynthComponent::HandleUtterance(const FGuid& InSpeaker, const FGenesisUtterance& Utterance, const FGenesisVoiceProfile& Profile)
{
	if (InSpeaker != Speaker)
	{
		return;
	}

	FScopeLock ScopeLock(&Lock);
	PendingUtterance = Utterance;
	PendingProfile = Profile;
	bPending = true;
}

bool UGenesisVoiceSynthComponent::IsSpeaking() const
{
	FScopeLock ScopeLock(&Lock);
	return bPending || Synth.IsActive();
}

void UGenesisVoiceSynthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bFollowHearing)
	{
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	if (!Audio)
	{
		return;
	}

	// Eine Stimme ist ein Geräusch der Welt draußen: Im Mutterleib kommt sie gedämpft an,
	// nach der Geburt in voller Bandbreite. Beides steht in der Hörwahrnehmung des Körpers.
	const FGenesisHearingPerception& Perception = Audio->GetHearingPerception();
	FScopeLock ScopeLock(&Lock);
	Hearing.CutoffHz = Perception.LowPassCutoffHz;
	Hearing.HighPassHz = Perception.HighPassCutoffHz;
	// Eine Stimme läuft über den Dialogbus: Was der Mix ihr zugesteht, gilt auch für sie selbst
	Hearing.Gain = FMath::Clamp(Perception.ExternalAudibility * Loudness, 0.0f, 4.0f)
		* Audio->GetBusGainLinear(EGenesisAudioBus::Dialogue);
}

int32 UGenesisVoiceSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	{
		FScopeLock ScopeLock(&Lock);
		if (bPending)
		{
			Synth.SetProfile(PendingProfile);
			Synth.Begin(PendingUtterance);
			bPending = false;
		}
	}

	Synth.Render(OutAudio, NumSamples);

	// Der Filter gehört zum Hörer, nicht zum Sprecher: Sein Zustand läuft über die Blöcke hinweg weiter.
	{
		FScopeLock ScopeLock(&Lock);
		Hearing.Process(OutAudio, NumSamples, SynthSampleRate);
	}

	return NumSamples;
}
