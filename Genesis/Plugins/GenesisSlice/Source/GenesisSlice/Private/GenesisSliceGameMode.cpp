// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceGameMode.h"
#include "GenesisSliceHud.h"
#include "GenesisSlicePlayerController.h"
#include "GameFramework/DefaultPawn.h"

AGenesisSliceGameMode::AGenesisSliceGameMode()
{
	PlayerControllerClass = AGenesisSlicePlayerController::StaticClass();
	HUDClass = AGenesisSliceHud::StaticClass();
	// Der Standard-Pawn bleibt: Er wird von den Kamera-Rigs versteckt und dient nur als Halter
	// für den Spielerstart. Ein eigener Pawn käme erst, wenn sich der Mensch bewegen kann.
	DefaultPawnClass = ADefaultPawn::StaticClass();
}
