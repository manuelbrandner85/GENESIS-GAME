// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisWorldSoundTypes.h"
#include "GenesisWorldSoundActor.generated.h"

class UGenesisWorldSoundComponent;

/**
 * Gibt einem Ort seine Stimme.
 *
 * Der Actor liest aus der Simulation, was der Ort gerade tut: Unter der Geburt steigt der Puls der
 * Mutter, der Monitor wird schneller, im Raum geschieht mehr. Nach der Geburt beruhigt sich alles wieder.
 */
UCLASS()
class GENESISWORLDSOUND_API AGenesisWorldSoundActor : public AActor
{
	GENERATED_BODY()

public:
	AGenesisWorldSoundActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Genesis|WorldSound")
	FGenesisWorldSoundParams Params;

	/** true: Puls und Betrieb folgen der laufenden Geburt. */
	UPROPERTY(EditAnywhere, Category = "Genesis|WorldSound")
	bool bFollowBirth = true;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGenesisWorldSoundComponent> WorldSound;

private:
	void RegisterDebugPage();
};
