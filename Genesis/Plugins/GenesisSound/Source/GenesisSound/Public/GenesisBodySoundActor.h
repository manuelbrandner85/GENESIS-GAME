// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisBodySoundActor.generated.h"

class UGenesisBodySynthComponent;

/**
 * Der Körperklang in einer Szene.
 *
 * Er kommt nicht von einem Ort im Raum, sondern von innen – deshalb ohne Ortung im Stereobild.
 * Ein Herzschlag, den man im eigenen Kopf hört, wandert nicht, wenn man den Kopf dreht.
 */
UCLASS()
class GENESISSOUND_API AGenesisBodySoundActor : public AActor
{
	GENERATED_BODY()

public:
	AGenesisBodySoundActor();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGenesisBodySynthComponent> Synth;
};
