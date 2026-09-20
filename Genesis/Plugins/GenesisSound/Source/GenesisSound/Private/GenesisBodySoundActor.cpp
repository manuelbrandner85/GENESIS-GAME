// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodySoundActor.h"
#include "GenesisBodySynthComponent.h"

AGenesisBodySoundActor::AGenesisBodySoundActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Synth = CreateDefaultSubobject<UGenesisBodySynthComponent>(TEXT("BodySynth"));
	SetRootComponent(Synth);
	// Körpergeräusche haben keinen Ort: Sie sind überall, wo das Kind ist
	Synth->bAllowSpatialization = false;
	Synth->bAutoActivate = true;
}
