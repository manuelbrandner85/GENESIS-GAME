// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisVoiceLogic.h"
#include "GenesisVoiceSubsystem.h"
#include "GenesisVoiceSynthComponent.h"

AGenesisVoiceActor::AGenesisVoiceActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Voice = CreateDefaultSubobject<UGenesisVoiceSynthComponent>(TEXT("Voice"));
	Voice->SetupAttachment(Root);
	// Die Stimme kommt in der ersten Stunde aus nächster Nähe – Ortung braucht es dafür nicht,
	// und Mono hält den Klang so, wie er gemessen wurde.
	Voice->bAllowSpatialization = false;
}

void AGenesisVoiceActor::BeginPlay()
{
	Super::BeginPlay();
	ResolveSpeaker();
}

bool AGenesisVoiceActor::ResolveSpeaker()
{
	if (SpeakerId.IsValid())
	{
		return true;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	if (VoiceRole == EGenesisVoiceRole::Mother)
	{
		SpeakerId = UGenesisVoiceSubsystem::GetMotherId();
	}
	else if (const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>())
	{
		// Vor der Geburt gibt es kein Kind und also auch keine Stimme
		SpeakerId = EarlyLife->GetState().EntityId;
	}

	if (SpeakerId.IsValid() && Voice)
	{
		Voice->SetSpeaker(SpeakerId);
	}
	return SpeakerId.IsValid();
}

void AGenesisVoiceActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ResolveSpeaker())
	{
		return;
	}

	Cooldown = FMath::Max(0.0f, Cooldown - DeltaSeconds);
	if (Cooldown > 0.0f || (Voice && Voice->IsSpeaking()))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisVoiceSubsystem* VoiceSubsystem = GameInstance ? GameInstance->GetSubsystem<UGenesisVoiceSubsystem>() : nullptr;
	const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr;
	if (!VoiceSubsystem || !EarlyLife || !EarlyLife->HasNewborn())
	{
		return;
	}

	const FGenesisNewbornState& State = EarlyLife->GetState();

	FGenesisUtterance Utterance;
	Utterance.Seed = GetTypeHash(SpeakerId) + static_cast<uint64>(UtteranceCount) * 7919ull;

	if (VoiceRole == EGenesisVoiceRole::Newborn)
	{
		if (!State.IsCrying())
		{
			// Ein ruhiges Kind ist still. Das ist kein fehlender Ton, das ist der Zustand.
			Cooldown = 0.5f;
			return;
		}
		// Wie dringend geschrien wird, hängt daran, wie schlecht es dem Kind geht
		Utterance.Type = State.Calm < 0.2f ? EGenesisUtterance::Cry : EGenesisUtterance::Fuss;
		Utterance.Intensity = FMath::Clamp(1.0f - State.Calm, 0.25f, 1.0f);
	}
	else
	{
		if (!State.bMotherSpeaking)
		{
			Cooldown = 0.5f;
			return;
		}
		// Wer ein schreiendes Kind beruhigt, spricht anders als jemand, der ein zufriedenes betrachtet
		if (State.IsCrying())
		{
			Utterance.Type = (UtteranceCount % 2 == 0) ? EGenesisUtterance::Soothe : EGenesisUtterance::Hum;
			Utterance.Intensity = 0.55f;
		}
		else
		{
			Utterance.Type = (UtteranceCount % 3 == 0) ? EGenesisUtterance::Hum : EGenesisUtterance::Speak;
			Utterance.Intensity = 0.4f;
		}
	}

	VoiceSubsystem->Say(SpeakerId, Utterance);
	++UtteranceCount;
	Cooldown = PauseSeconds;
}
