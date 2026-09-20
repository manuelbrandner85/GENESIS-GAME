// GENESIS: Der Kreislauf des Lebens

#include "GenesisMusicSynthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisBodySubsystem.h"
#include "GenesisSoulMusicSubsystem.h"

UGenesisMusicSynthComponent::UGenesisMusicSynthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	NumChannels = 1;
}

bool UGenesisMusicSynthComponent::Init(int32& SampleRate)
{
	NumChannels = 1;
	Synth.Initialize(static_cast<float>(SampleRate));
	Synth.MasterGain = MusicGain;
	return true;
}

int32 UGenesisMusicSynthComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	{
		FScopeLock Lock(&PhraseLock);
		if (bPhraseDirty)
		{
			Synth.SetPhrase(PendingPhrase, bPendingLoop);
			bPhraseDirty = false;
		}
		// Der Mix entscheidet, wie laut die Musik gerade sein darf – beim Sprechen tritt sie zurück
		Synth.MasterGain = MusicGain * MixGain;
	}

	Synth.Render(OutAudio, NumSamples);
	return NumSamples;
}

void UGenesisMusicSynthComponent::PlayPhrase(const FGenesisMusicPhrase& Phrase, bool bLoop)
{
	FScopeLock Lock(&PhraseLock);
	PendingPhrase = Phrase;
	bPendingLoop = bLoop;
	bPhraseDirty = true;
}

void UGenesisMusicSynthComponent::PlayLeitmotifOfListener()
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGenesisSoulMusicSubsystem* Music = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>() : nullptr;
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	if (!Music || !Body)
	{
		return;
	}

	// Wer hört, ist der erste lebende Körper der Welt – im Vertical Slice der Spieler
	FGuid Listener;
	for (int32 Index = 0; Index < Body->GetBodyCount(); ++Index)
	{
		const FGenesisBodyState* Candidate = Body->GetBodyByIndex(Index);
		if (Candidate && Candidate->bAlive)
		{
			Listener = Candidate->EntityId;
			break;
		}
	}
	if (!Listener.IsValid())
	{
		return;
	}

	LastPhase = Music->GetLifePhase(Listener);
	PlayPhrase(Music->RenderLeitmotif(Listener), false);
	SilenceSeconds = 0.0f;
}

void UGenesisMusicSynthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (const UGameInstance* MixInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (const UGenesisAudioSubsystem* MixAudio = MixInstance->GetSubsystem<UGenesisAudioSubsystem>())
		{
			FScopeLock Lock(&PhraseLock);
			MixGain = MixAudio->GetBusGainLinear(EGenesisAudioBus::Music);
		}
	}

	if (!bFollowLifePhase)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UGenesisSoulMusicSubsystem* Music = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>() : nullptr;
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	if (!Music || !Body)
	{
		return;
	}

	FGuid Listener;
	for (int32 Index = 0; Index < Body->GetBodyCount(); ++Index)
	{
		const FGenesisBodyState* Candidate = Body->GetBodyByIndex(Index);
		if (Candidate && Candidate->bAlive)
		{
			Listener = Candidate->EntityId;
			break;
		}
	}
	if (!Listener.IsValid())
	{
		return;
	}

	// Neue Lebensphase: Das Motiv klingt ab jetzt anders – sofort umstellen
	const FGameplayTag Phase = Music->GetLifePhase(Listener);
	if (Phase != LastPhase)
	{
		PlayLeitmotifOfListener();
		return;
	}

	// Sonst: nach dem Ausklingen eine Pause, dann noch einmal
	if (Synth.IsFinished())
	{
		SilenceSeconds += DeltaTime;
		if (SilenceSeconds >= BreathSeconds)
		{
			PlayLeitmotifOfListener();
		}
	}
}
