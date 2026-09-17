// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GenesisAudioTypes.h"
#include "GenesisAudioSubsystem.generated.h"

/** Projekteinstellungen (Project Settings → Genesis → Audio). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Audio"))
class GENESISAUDIOCORE_API UGenesisAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Hearing")
	FGenesisHearingTuning Hearing;

	UPROPERTY(Config, EditAnywhere, Category = "Mix")
	FGenesisAudioMixTuning Mix;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnHearingEnvironmentChanged, bool /*bNowInWomb*/);

/**
 * Audio-Fundament zur Laufzeit.
 *
 * Leitet jeden Frame aus dem Körper der hörenden Person (in der Regel der Spieler) die Hörwahrnehmung und die
 * Körperklang-Parameter ab, glättet sie und führt die Mix-Engine. Die Werte sind persistenzfrei, weil sie
 * vollständig aus dem gespeicherten Körperzustand rekonstruierbar sind.
 */
UCLASS()
class GENESISAUDIOCORE_API UGenesisAudioSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override { return bInitialized; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;

	/** Wer hört (Spieler). Ohne Angabe: erster Körper mit Simulation Level 1. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Audio")
	void SetListenerEntity(const FGuid& EntityId) { ListenerEntityId = EntityId; }

	/** Eine Quelle beansprucht für HoldSeconds Platz im Mix (z. B. ein Dialogsatz). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Audio")
	void PushMixRequest(EGenesisAudioBus Bus, float Importance, float HoldSeconds);

	UFUNCTION(BlueprintPure, Category = "Genesis|Audio")
	const FGenesisHearingPerception& GetHearingPerception() const { return Perception; }

	UFUNCTION(BlueprintPure, Category = "Genesis|Audio")
	const FGenesisBodyAudioParams& GetBodyAudioParams() const { return BodyAudio; }

	UFUNCTION(BlueprintPure, Category = "Genesis|Audio")
	float GetBusGainDb(EGenesisAudioBus Bus) const { return Mix.GetGainDb(Bus); }

	UFUNCTION(BlueprintPure, Category = "Genesis|Audio")
	float GetBusGainLinear(EGenesisAudioBus Bus) const;

	/** Mutterleib ↔ Luft (Geburt) – Schlüsselmoment für Audio und Cinematics. */
	FGenesisOnHearingEnvironmentChanged OnHearingEnvironmentChanged;

private:
	FGuid ResolveListener() const;
	void RegisterDebugPage();

	struct FTimedMixRequest
	{
		FGenesisMixRequest Request;
		float RemainingSeconds = 0.0f;
	};

	FGuid ListenerEntityId;
	FGenesisHearingPerception Perception;
	FGenesisBodyAudioParams BodyAudio;
	FGenesisMixState Mix;
	TArray<FTimedMixRequest> TimedRequests;
	bool bHasPerception = false;
	bool bInitialized = false;
	float LastTickSeconds = 0.0f;
};
