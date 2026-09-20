// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisVoiceActor.generated.h"

class UGenesisVoiceSynthComponent;

/** Wessen Stimme dieser Actor in der Szene ist. */
UENUM(BlueprintType)
enum class EGenesisVoiceRole : uint8
{
	/** Das Kind selbst – in der ersten Stunde heißt das: schreien, wenn es ihm schlecht geht. */
	Newborn,
	/** Die Mutter. Sie spricht, wenn mit dem Kind gesprochen wird. */
	Mother
};

/**
 * Gibt einer Person in der Szene ihre Stimme.
 *
 * Der Actor erfindet nichts: Er schaut in die Simulation, ob das Kind gerade schreit oder ob jemand
 * mit ihm spricht, und löst dann den passenden Laut aus. Ein zufriedenes Kind schweigt –
 * und zwar nicht, weil es gerade keine Klangdatei gibt, sondern weil es zufrieden ist.
 */
UCLASS()
class GENESISVOICE_API AGenesisVoiceActor : public AActor
{
	GENERATED_BODY()

public:
	AGenesisVoiceActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Genesis|Voice")
	EGenesisVoiceRole VoiceRole = EGenesisVoiceRole::Newborn;

	/** Pause zwischen zwei Lauten (s). Ein Kind schreit in Wellen, es schreit nicht ununterbrochen. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Voice")
	float PauseSeconds = 1.1f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGenesisVoiceSynthComponent> Voice;

private:
	/** Sucht die Person, zu der diese Stimme gehört. Beim Kind gelingt das erst nach der Geburt. */
	bool ResolveSpeaker();

	FGuid SpeakerId;
	float Cooldown = 0.0f;
	int32 UtteranceCount = 0;
};
