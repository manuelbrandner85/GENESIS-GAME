// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceGameMode.h"
#include "GenesisSliceHud.h"
#include "GenesisSlicePlayerController.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisBootFlow.h"
#include "GenesisMicroscopeCameraRig.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
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

	// Tickt, um im Startablauf die Kamera zu führen – auch in der Pause, wo das Menü darüber liegt
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

UGenesisFrontendSubsystem* AGenesisSliceGameMode::GetFrontend() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
}

void AGenesisSliceGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGenesisFrontendSubsystem* Frontend = GetFrontend();
	if (!Frontend)
	{
		return;
	}

	// Die Einstellungen des Spielers gelten ab jetzt – vorher stand die Welt noch nicht.
	Frontend->ApplySettings();

	// Nur die spielbare Fassung beginnt mit dem Startablauf. Im Editor und in den Messläufen wäre
	// ein Studiologo vor jedem Bild im Weg; dort öffnet `genesis.Menu` das Menü bei Bedarf.
	// Der Ablauf beginnt genau einmal: Nach einem Ortswechsel (Lebensbeginn, Rückkehr ins Menü)
	// steht er schon woanders und darf nicht wieder beim Logo anfangen.
	const bool bPlayable = FParse::Param(FCommandLine::Get(), TEXT("genesisplay"));
	if (bPlayable)
	{
		// Entwicklermeldungen der Engine (rote Zeilen oben links) gehören nicht in ein Spiel. Eine
		// Veröffentlichungsfassung zeigt sie nie; die spielbare Testfassung soll sich genauso anfühlen.
		// Im Protokoll stehen sie weiterhin.
		if (GEngine)
		{
			GEngine->bEnableOnScreenDebugMessages = false;
		}
		if (Frontend->GetBootStage() == EGenesisBootStage::Aus)
		{
			Frontend->StartBoot();
		}
	}
}

void AGenesisSliceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UGenesisFrontendSubsystem* Frontend = GetFrontend();
	if (!Frontend)
	{
		return;
	}

	// Vor dem ersten Leben gehört die Kamera dem Rahmen: eine langsame Fahrt auf die Eizelle im
	// Prolog, danach ein ruhiger Blick auf sie hinter Titel und Menü. Sobald ein Leben läuft, folgt
	// die Kamera wieder einer Zelle – so wie die Szene gebaut ist.
	const FGenesisBootState& Boot = Frontend->GetBootState();
	float Distance = -1.0f;
	switch (Boot.Stage)
	{
	case EGenesisBootStage::Studio:
	case EGenesisBootStage::Engine:
	case EGenesisBootStage::Hinweis:
		Distance = GenesisBootFlow::PrologueCameraDistance(0.0f);
		break;
	case EGenesisBootStage::Prolog:
		Distance = GenesisBootFlow::PrologueCameraDistance(Boot.StageSeconds);
		break;
	case EGenesisBootStage::Titel:
	case EGenesisBootStage::Taste:
	case EGenesisBootStage::Menue:
		Distance = GenesisBootFlow::PrologueCameraDistance(1000.0f);
		break;
	default:
		break;
	}
	if (Distance < 0.0f)
	{
		return;
	}

	for (TActorIterator<AGenesisMicroscopeCameraRig> It(GetWorld()); It; ++It)
	{
		It->bWatchOocyte = true;
		if (!FMath::IsNearlyEqual(It->OocyteDistanceUm, Distance, 0.5f))
		{
			It->OocyteDistanceUm = Distance;
			It->OrbitDistanceUm = Distance;
			It->ApplyOptics();
		}
	}
}
