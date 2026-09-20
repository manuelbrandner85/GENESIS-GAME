// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisMusicActor.generated.h"

class UGenesisMusicSynthComponent;

/** Die Musik einer Szene. Wie der Körperklang ohne Ort im Raum – sie kommt von innen. */
UCLASS()
class GENESISSOUND_API AGenesisMusicActor : public AActor
{
	GENERATED_BODY()

public:
	AGenesisMusicActor();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UGenesisMusicSynthComponent> Music;
};
