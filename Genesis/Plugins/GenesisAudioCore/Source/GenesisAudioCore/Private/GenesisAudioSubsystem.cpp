// GENESIS: Der Kreislauf des Lebens

#include "GenesisAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisAudioCoreLogic.h"
#include "GenesisBodySubsystem.h"
#include "GenesisDebug.h"
#include "GenesisWorldClockSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisAudioDialogueCommand(
		TEXT("genesis.Audio.SimulateDialogue"),
		TEXT("Entwickler: meldet für N Sekunden einen wichtigen Dialog im Mix (Ducking prüfen). Optional: Sekunden, Wichtigkeit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			if (UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr)
			{
				const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 5.0f;
				const float Importance = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 1.0f;
				Audio->PushMixRequest(EGenesisAudioBus::Dialogue, Importance, Seconds);
			}
		}));
}
#endif

void UGenesisAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGenesisWorldClockSubsystem>();
	Collection.InitializeDependency<UGenesisBodySubsystem>();
	bInitialized = true;
	RegisterDebugPage();
}

void UGenesisAudioSubsystem::Deinitialize()
{
	bInitialized = false;
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Audio"));
#endif
	OnHearingEnvironmentChanged.Clear();
	Super::Deinitialize();
}

ETickableTickType UGenesisAudioSubsystem::GetTickableTickType() const
{
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

UWorld* UGenesisAudioSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

TStatId UGenesisAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGenesisAudioSubsystem, STATGROUP_Tickables);
}

void UGenesisAudioSubsystem::PushMixRequest(EGenesisAudioBus Bus, float Importance, float HoldSeconds)
{
	FTimedMixRequest& Timed = TimedRequests.AddDefaulted_GetRef();
	Timed.Request.Bus = Bus;
	Timed.Request.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
	Timed.RemainingSeconds = FMath::Max(0.0f, HoldSeconds);
}

float UGenesisAudioSubsystem::GetBusGainLinear(EGenesisAudioBus Bus) const
{
	// Mischung mal Regler: Dass die Mischung etwas leiser macht, weil gerade etwas Wichtigeres
	// läuft, und dass der Spieler es leiser gestellt hat, sind zwei verschiedene Dinge – beide wirken.
	return GenesisAudioCoreLogic::DbToLinear(Mix.GetGainDb(Bus)) * GetUserVolume(Bus) * SceneGain;
}

void UGenesisAudioSubsystem::SetUserVolumes(float Master, float Dialogue, float Music, float World)
{
	UserMasterVolume = FMath::Clamp(Master, 0.0f, 1.0f);
	UserDialogueVolume = FMath::Clamp(Dialogue, 0.0f, 1.0f);
	UserMusicVolume = FMath::Clamp(Music, 0.0f, 1.0f);
	UserWorldVolume = FMath::Clamp(World, 0.0f, 1.0f);
}

float UGenesisAudioSubsystem::GetUserVolume(EGenesisAudioBus Bus) const
{
	switch (Bus)
	{
	case EGenesisAudioBus::Dialogue:
		return UserMasterVolume * UserDialogueVolume;
	case EGenesisAudioBus::Music:
		return UserMasterVolume * UserMusicVolume;
	case EGenesisAudioBus::VitalSignal:
		// Lebenswichtige Signale hängen am Gesamtregler, bekommen aber keinen eigenen: Wer sie
		// wegdrehen könnte, könnte sich das Spiel unspielbar einstellen.
		return UserMasterVolume;
	default:
		return UserMasterVolume * UserWorldVolume;
	}
}

FGuid UGenesisAudioSubsystem::ResolveListener() const
{
	if (ListenerEntityId.IsValid())
	{
		return ListenerEntityId;
	}

	// Entwicklerkomfort: ohne expliziten Hörer der erste voll simulierte Körper
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBodySubsystem* Bodies = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	if (Bodies)
	{
		for (int32 Index = 0; ; ++Index)
		{
			const FGenesisBodyState* Candidate = Bodies->GetBodyByIndex(Index);
			if (!Candidate)
			{
				break;
			}
			if (Candidate->SimulationLevel == EGenesisSimulationLevel::Full)
			{
				return Candidate->EntityId;
			}
		}
	}
	return FGuid();
}

void UGenesisAudioSubsystem::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisAudio_Tick);
	const double Start = FPlatformTime::Seconds();

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBodySubsystem* Bodies = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisTimestamp Now = Clock ? Clock->GetNow() : FGenesisTimestamp();
	const FGenesisBodyState* Body = Bodies ? Bodies->FindBody(ResolveListener()) : nullptr;

	const UGenesisAudioSettings* Settings = GetDefault<UGenesisAudioSettings>();
	const FGenesisHearingPerception Target = GenesisAudioCoreLogic::ComputeHearing(Body, Now, Settings->Hearing);

	const bool bWasInWomb = Perception.bInWomb;
	Perception = bHasPerception ? GenesisAudioCoreLogic::SmoothPerception(Perception, Target, DeltaTime, Settings->Hearing) : Target;
	if (bHasPerception && bWasInWomb != Perception.bInWomb)
	{
		OnHearingEnvironmentChanged.Broadcast(Perception.bInWomb);
	}
	bHasPerception = true;

	BodyAudio = GenesisAudioCoreLogic::ComputeBodyAudio(Body, Now, Perception);

	// Mix: abgelaufene Anforderungen entfernen, Körper meldet sich selbst, wenn er deutlich hörbar ist
	TArray<FGenesisMixRequest> Active;
	for (int32 Index = TimedRequests.Num() - 1; Index >= 0; --Index)
	{
		TimedRequests[Index].RemainingSeconds -= DeltaTime;
		if (TimedRequests[Index].RemainingSeconds <= 0.0f)
		{
			TimedRequests.RemoveAtSwap(Index);
			continue;
		}
		Active.Add(TimedRequests[Index].Request);
	}
	if (Perception.BodyAudibility > 0.4f)
	{
		FGenesisMixRequest BodyRequest;
		BodyRequest.Bus = EGenesisAudioBus::Body;
		BodyRequest.Importance = Perception.BodyAudibility;
		Active.Add(BodyRequest);
	}
	GenesisAudioCoreLogic::UpdateMix(Mix, Active, Settings->Mix, DeltaTime);

	LastTickSeconds = static_cast<float>(FPlatformTime::Seconds() - Start);
}

void UGenesisAudioSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisAudioSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Audio"),
		TEXT("Audio Core"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisAudioSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FGenesisHearingPerception& Hearing = Self->Perception;
			OutLines.Add(FString::Printf(TEXT("Hörer %s | %s | Tiefpass %.0f Hz | Hochton %.1f dB | außen %.2f | Körper %.2f | Tunnel %.2f | Tinnitus %.2f"),
				*Self->ResolveListener().ToString(EGuidFormats::Short).Left(8), Hearing.bInWomb ? TEXT("Mutterleib") : TEXT("Luft"),
				Hearing.LowPassCutoffHz, Hearing.HighShelfGainDb, Hearing.ExternalAudibility, Hearing.BodyAudibility, Hearing.FocusNarrowing, Hearing.TinnitusLevel));

			const FGenesisBodyAudioParams& Body = Self->BodyAudio;
			OutLines.Add(FString::Printf(TEXT("Herz %.0f bpm Stärke %.2f Unregelm. %.2f hörbar %.2f | Atem %.0f/min Tiefe %.2f Not %.2f hörbar %.2f | Zittern %.2f%s"),
				Body.HeartRateBpm, Body.HeartStrength, Body.HeartIrregularity, Body.HeartAudibility,
				Body.BreathRate, Body.BreathDepth, Body.BreathStrain, Body.BreathAudibility, Body.Tremor,
				Body.bMaternalSounds ? *FString::Printf(TEXT(" | Mutter-Herz %.0f bpm"), Body.MaternalHeartRateBpm) : TEXT("")));

			const UEnum* BusEnum = StaticEnum<EGenesisAudioBus>();
			FString Buses;
			for (int32 Index = 0; Index < GenesisAudioBusCount; ++Index)
			{
				Buses += FString::Printf(TEXT("%s %.1f dB  "), *BusEnum->GetNameStringByValue(Index), Self->Mix.CurrentGainDb[Index]);
			}
			OutLines.Add(TEXT("Mix: ") + Buses);
			OutLines.Add(FString::Printf(TEXT("Mix-Anforderungen %d | CPU Audio-Core %.3f ms"), Self->TimedRequests.Num(), Self->LastTickSeconds * 1000.0f));
		}
	});
#endif
}
