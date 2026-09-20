// GENESIS: Der Kreislauf des Lebens

#include "GenesisMusicActor.h"
#include "GenesisMusicSynthComponent.h"

AGenesisMusicActor::AGenesisMusicActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Music = CreateDefaultSubobject<UGenesisMusicSynthComponent>(TEXT("Music"));
	SetRootComponent(Music);
	Music->bAllowSpatialization = false;
	Music->bAutoActivate = true;
}
