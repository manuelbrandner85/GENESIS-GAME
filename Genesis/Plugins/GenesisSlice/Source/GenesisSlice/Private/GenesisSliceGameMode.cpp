// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceGameMode.h"
#include "GenesisSliceHud.h"
#include "GenesisSlicePlayerController.h"
#include "GenesisSliceDirector.h"
#include "GenesisFrontendSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/DefaultPawn.h"
#include "GenesisLog.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

AGenesisSliceGameMode::AGenesisSliceGameMode()
{
	PlayerControllerClass = AGenesisSlicePlayerController::StaticClass();
	HUDClass = AGenesisSliceHud::StaticClass();
	// Der Standard-Pawn bleibt: Er wird von den Kamera-Rigs versteckt und dient nur als Halter
	// für den Spielerstart. Ein eigener Pawn käme erst, wenn sich der Mensch bewegen kann.
	DefaultPawnClass = ADefaultPawn::StaticClass();
}

void AGenesisSliceGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	UGenesisSliceDirector* Director = GameInstance ? GameInstance->GetSubsystem<UGenesisSliceDirector>() : nullptr;
	if (!Frontend)
	{
		return;
	}

	// Die Einstellungen des Spielers gelten ab jetzt – vorher stand die Welt noch nicht.
	Frontend->ApplySettings();

	if (Director)
	{
		Frontend->OnStartRequested.AddWeakLambda(this, [Director]()
		{
			UE_LOG(LogGenesis, Display, TEXT("Startbildschirm: Der Spieler beginnt ein Leben."));
			Director->StartRun(0);
		});
		Frontend->OnRestartRequested.AddWeakLambda(this, [Director]()
		{
			UE_LOG(LogGenesis, Display, TEXT("Startbildschirm: von vorn."));
			Director->StartRun(0);
		});
	}

	// Nur die spielbare Fassung beginnt mit dem Startbildschirm. Im Editor und in den Messläufen
	// wäre ein Menü über jedem Bild im Weg; dort öffnet `genesis.Menu` es bei Bedarf.
	if (FParse::Param(FCommandLine::Get(), TEXT("genesisplay")))
	{
		Frontend->OpenMenu(EGenesisMenuPage::Haupt);
	}
}
