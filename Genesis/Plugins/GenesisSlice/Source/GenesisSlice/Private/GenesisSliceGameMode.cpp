// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceGameMode.h"
#include "GenesisSliceHud.h"
#include "GenesisSlicePlayerController.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisBootFlow.h"
#include "GenesisMicroscopeCameraRig.h"
#include "GenesisBirthCameraRig.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisMotherRig.h"
#include "GenesisSpermSwarm.h"
#include "GenesisWorldSoundActor.h"
#include "GenesisWorldSoundExport.h"
#include "GenesisBodySoundActor.h"
#include "GenesisMusicActor.h"
#include "GenesisVoiceActor.h"
#include "GenesisSceneSpeech.h"
#include "GenesisMidwifeRig.h"
#include "GenesisImplantationSite.h"
#include "GenesisEmbryoScene.h"
#include "GenesisPeopleRendering.h"
#include "CineCameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/DefaultPawn.h"
#include "GenesisLog.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "HAL/FileManager.h"
#include "AudioMixerBlueprintLibrary.h"

namespace
{
	/** Prüfhilfe: erzwingt, dass das Kind auf der Brust das Gesicht der Mutter sucht (wie ein gehaltener Blick nach oben). */
	TAutoConsoleVariable<int32> CVarSeekFace(
		TEXT("genesis.Mother.SeekFace"),
		0,
		TEXT("1 = das Kind sucht das Gesicht der Mutter, als hielte der Spieler den Blick oben."));

#if !UE_BUILD_SHIPPING
	/**
	 * Hörprüfung ohne Ohren: nimmt den fertigen Mix des Spiels auf und legt ihn als WAV ab
	 * (Saved/BouncedWavFiles/<Name>.wav). So lässt sich messen, ob und wie laut eine Szene klingt.
	 */
	FAutoConsoleCommandWithWorldAndArgs GenesisAudioRecordCommand(
		TEXT("genesis.Audio.Record"),
		TEXT("Nimmt den Spielton auf: <Sekunden> <Name>. Ergebnis in Saved/BouncedWavFiles."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World)
			{
				return;
			}
			const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.0f;
			const FString Name = Args.Num() > 1 ? Args[1] : TEXT("GENESIS_Aufnahme");
			UAudioMixerBlueprintLibrary::StartRecordingOutput(World, Seconds);
			FTimerHandle Handle;
			TWeakObjectPtr<UWorld> WeakWorld(World);
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([WeakWorld, Name]()
			{
				if (UWorld* Current = WeakWorld.Get())
				{
					// Absoluter Pfad: Der relative Projektpfad wird beim Schreiben falsch zusammengesetzt (C:/Users/Users/…)
					const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("BouncedWavFiles"));
					IFileManager::Get().MakeDirectory(*Folder, true);
					UAudioMixerBlueprintLibrary::StopRecordingOutput(Current, EAudioRecordingExportType::WavFile, Name, Folder);
					UE_LOG(LogGenesis, Display, TEXT("Tonaufnahme gespeichert: %s"), *Name);
				}
			}), FMath::Max(0.5f, Seconds), false);
		}));
#endif
}

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

	EnsureSceneSound();

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

void AGenesisSliceGameMode::EnsureSceneSound()
{
	// Der Klang einer Szene wird hier zur Laufzeit angelegt, nicht im Level gespeichert. Früher standen die
	// Klang-Actors in den Karten – und jedes Skript, das eine Szene neu aufbaute und die Karte speicherte,
	// verlor sie wieder. Ab dem Menü war das ganze Spiel still (Prüfung 2026-09-21). Jetzt entscheidet der
	// Ort, was zu hören ist, und kein Neuaufbau einer Szene kann ihn stumm machen.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	auto Has = [World](UClass* Class)
	{
		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			return true;
		}
		return false;
	};
	auto Place = [World](const TCHAR* /*Label*/, EGenesisPlace Where, float Loudness, float Activity, float Digestion, bool bFollowBirth, const FVector& Location)
	{
		for (TActorIterator<AGenesisWorldSoundActor> It(World); It; ++It)
		{
			if (It->Params.Place == Where)
			{
				return;
			}
		}
		if (AGenesisWorldSoundActor* Actor = World->SpawnActorDeferred<AGenesisWorldSoundActor>(AGenesisWorldSoundActor::StaticClass(), FTransform(Location)))
		{
			Actor->Params = GenesisWorldSoundExport::GetPresetParams(Where);
			Actor->Params.Place = Where;
			Actor->Params.Loudness = Loudness;
			Actor->Params.Activity = Activity;
			Actor->Params.Digestion = Digestion;
			Actor->bFollowBirth = bFollowBirth;
			Actor->FinishSpawning(FTransform(Location));
			UE_LOG(LogGenesis, Display, TEXT("Klang der Szene: %s"), *GenesisWorldSoundExport::GetPlaceName(Where));
		}
	};

	const bool bOviduct = Has(AGenesisSpermSwarm::StaticClass());
	const bool bBirth = Has(AGenesisBirthCameraRig::StaticClass());
	// Gebärmutter und Fruchthöhle: drinnen im Mutterleib. Die Gebärmutter war bis GENESIS-041 stumm.
	const bool bInsideWomb = Has(AGenesisImplantationSite::StaticClass()) || Has(AGenesisEmbryoScene::StaticClass());

	if (bOviduct)
	{
		// Im Eileiter: der Strom der Zilien, das ferne Pochen der mütterlichen Gefäße
		Place(TEXT("OviductTone"), EGenesisPlace::OviductAmpulla, 0.9f, 0.1f, 0.5f, false, FVector::ZeroVector);
	}
	if (bInsideWomb)
	{
		// Das Rauschen der mütterlichen Gefäße, Darmgeräusche, die Welt draußen nur als Dröhnen. Leiser als bei der
		// Geburt: Der Embryo hört noch nichts (Hören erst ab Woche 24) – das hier ist für den, der zusieht.
		Place(TEXT("WombTone"), EGenesisPlace::Womb, 0.6f, 0.2f, 0.5f, false, FVector::ZeroVector);
	}
	if (bBirth)
	{
		// Zwei Orte zugleich: der Mutterleib von innen und der Kreißsaal von außen – vor der Geburt durch
		// Bauchdecke und Fruchtwasser gedämpft, nach dem ersten Atemzug klar
		Place(TEXT("WombTone"), EGenesisPlace::Womb, 0.9f, 0.3f, 0.6f, true, FVector::ZeroVector);
		// Der Kreißsaal klingt aus einer echten Aufnahme (AGenesisSceneSpeech). Die Synthese des Raums ist hier aus:
		// Ihr Monitor piepte als reiner 980-Hz-Ton bei jedem Herzschlag der Mutter, dazu Metallklänge aus reinen
		// Sinustönen – das komische Klingeln, das der Game Director gehört hat. Echte Kreißsäle piepen so nicht.

		// Der eigene Körper (Herzschlag, Atem) – ab der Geburt der des Kindes
		if (!Has(AGenesisBodySoundActor::StaticClass()))
		{
			World->SpawnActor<AGenesisBodySoundActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		}
		// Stimmen: das Kind (Schreien) und die Mutter
		bool bChild = false;
		bool bMother = false;
		for (TActorIterator<AGenesisVoiceActor> It(World); It; ++It)
		{
			bChild |= It->VoiceRole == EGenesisVoiceRole::Newborn;
			bMother |= It->VoiceRole == EGenesisVoiceRole::Mother;
		}
		auto SpawnVoice = [World](EGenesisVoiceRole VoiceRole, const FVector& Location)
		{
			if (AGenesisVoiceActor* Voice = World->SpawnActorDeferred<AGenesisVoiceActor>(AGenesisVoiceActor::StaticClass(), FTransform(Location)))
			{
				Voice->VoiceRole = VoiceRole;
				Voice->FinishSpawning(FTransform(Location));
			}
		};
		// Das Kind schreit jetzt mit echten Aufnahmen (AGenesisSceneSpeech, geprüft: Grundton 440–470 Hz wie bei
		// Neugeborenen). Die Synthese liefe sonst gleichzeitig – zwei Kinder in einem Raum.
		(void)bChild;
		(void)SpawnVoice;
		// Die Mutter spricht jetzt echte Sätze (AGenesisSceneSpeech). Ihre bisherige Stimme aus Silben ohne Worte
		// liefe sonst gleichzeitig darüber – zwei Mütter in einem Raum.
		(void)bMother;
		// Die Hebamme: MetaHuman-Figur (BP_Midwife, wie die Mutter außerhalb von Git) und ihr Rig – zur Laufzeit,
		// damit kein Neuaufbau der Karte sie verliert
		if (!Has(AGenesisMidwifeRig::StaticClass()))
		{
			UClass* MidwifeClass = LoadClass<AActor>(nullptr, TEXT("/Game/Genesis/Characters/Midwife/Built/Midwife/BP_Midwife.BP_Midwife_C"));
			AGenesisMotherRig* MotherRig = nullptr;
			for (TActorIterator<AGenesisMotherRig> It(World); It; ++It)
			{
				MotherRig = *It;
				break;
			}
			if (MidwifeClass)
			{
				AGenesisMidwifeRig* Rig = World->SpawnActorDeferred<AGenesisMidwifeRig>(AGenesisMidwifeRig::StaticClass(), FTransform::Identity);
				const FTransform Start(FRotator::ZeroRotator, Rig->FootOfBed, FVector(10.0));
				// Die Figur ist in cm gebaut, die Szene in mm – Faktor 10 wie bei der Mutter
				Rig->MidwifeActor = World->SpawnActor<AActor>(MidwifeClass, Start);
				if (Rig->MidwifeActor)
				{
					Rig->MidwifeActor->SetActorScale3D(FVector(10.0));
				}
				Rig->Mother = MotherRig;
				Rig->FinishSpawning(FTransform::Identity);
				UE_LOG(LogGenesis, Display, TEXT("Hebamme im Kreißsaal: %s"), Rig->MidwifeActor ? TEXT("ja") : TEXT("Figur fehlt"));
			}
			else
			{
				UE_LOG(LogGenesis, Warning, TEXT("Hebamme: BP_Midwife nicht gefunden (Tools/Unreal/Birth/midwife_create.py im Editor ausführen)."));
			}
		}
		if (!Has(AGenesisSceneSpeech::StaticClass()))
		{
			World->SpawnActor<AGenesisSceneSpeech>(FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}
	if ((bOviduct || bBirth) && !Has(AGenesisMusicActor::StaticClass()))
	{
		// Die Seelenmusik des Lebens – sie folgt dem, was die Simulation erzählt
		World->SpawnActor<AGenesisMusicActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	}
}

void AGenesisSliceGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Nach der Geburt: Die Kamera des Kindes folgt dem, was mit ihm geschieht. Liegt es auf der Haut
	// der Mutter, liegt die Kamera auf ihrer Brust.
	if (UGenesisEarlyLifeSubsystem* EarlyLife = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr)
	{
		const bool bOnChest = EarlyLife->HasNewborn() && EarlyLife->GetState().bSkinToSkin;
		const AGenesisSlicePlayerController* Player = Cast<AGenesisSlicePlayerController>(GetWorld()->GetFirstPlayerController());
		const bool bSeeksFace = bOnChest && ((Player && Player->IsSeekingFace()) || CVarSeekFace.GetValueOnGameThread() > 0);

		AGenesisMotherRig* Mother = nullptr;
		for (TActorIterator<AGenesisMotherRig> It(GetWorld()); It; ++It)
		{
			Mother = *It;
			break;
		}

		bool bEyeContact = false;
		for (TActorIterator<AGenesisBirthCameraRig> It(GetWorld()); It; ++It)
		{
			It->bOnMothersChest = bOnChest;
			// Die Mutter sieht, wo das Kind ist, und das Kind liegt auf ihrem Atem. Hebt es den Blick,
			// holt sie es vor ihr Gesicht – und die Kamera geht mit ihren Händen.
			if (Mother && It->Camera)
			{
				Mother->SetChild(bOnChest, bSeeksFace, It->Camera->GetComponentLocation(), It->Camera->GetComponentQuat());
				// Nach Stunden Wehen: Schweiß und Röte, die in der ersten Stunde abklingen
				Mother->SetExertion(EarlyLife->HasNewborn()
					? GenesisPeopleRendering::ExertionAfterBirth(static_cast<float>(EarlyLife->GetState().MinutesSinceBirth)) : 1.0f);
				// Das Kind liegt auf ihrem Körper, nicht auf festen Koordinaten
				It->ChestEyeLocation = It->GetActorTransform().InverseTransformPosition(Mother->GetChestChildLocation());
				It->MotherBreathLift = Mother->GetBreathLift();
				It->EnFaceBlend = Mother->GetEnFaceBlend();
				It->EnFaceView = Mother->GetEnFaceChildTransform();
				bEyeContact = Mother->HasEyeContact();
			}
			// Die Hebamme: fängt das Kind auf, hält es vor sich, legt es der Mutter auf die Brust
			if (It->Camera)
			{
				for (TActorIterator<AGenesisMidwifeRig> Midwife(GetWorld()); Midwife; ++Midwife)
				{
					Midwife->SetChild(EarlyLife->HasNewborn(), bOnChest, It->Camera->GetComponentLocation(), It->Camera->GetComponentQuat());
					It->MidwifeHoldBlend = Midwife->GetHoldBlend();
					It->MidwifeHeldView = Midwife->GetHeldView();
					// Auf der Brust: abrubbeln, dann zudecken. Das Kind spürt es, sieht den Rand des Tuchs – und es
					// hält warm: Abgetrocknet fällt die Verdunstung weg, zugedeckt liegt der Rücken nicht mehr frei.
					const EGenesisMidwifeTask Care = Midwife->GetTask();
					const float Step = Midwife->GetTaskProgress();
					It->DryingRub = Care == EGenesisMidwifeTask::Drying
						? FMath::SmoothStep(0.0f, 0.08f, Step) * (1.0f - FMath::SmoothStep(0.92f, 1.0f, Step)) : 0.0f;
					It->TowelCover = Care == EGenesisMidwifeTask::Covering ? Step : (Care == EGenesisMidwifeTask::Watching && bOnChest ? 1.0f : 0.0f);
					if (bOnChest && (Care == EGenesisMidwifeTask::Covering || Care == EGenesisMidwifeTask::Watching))
					{
						EarlyLife->SetDried(true);
					}
					if (bOnChest && Care == EGenesisMidwifeTask::Watching)
					{
						EarlyLife->SetCovered(true);
					}
					// Ihre Stimme kommt von dort, wo sie ist
					for (TActorIterator<AGenesisSceneSpeech> Speech(GetWorld()); Speech; ++Speech)
					{
						Speech->SetMidwifeLocation(Midwife->GetEyeLocation() - FVector(0.0, 0.0, 70.0));
					}
				}
			}
		}
		if (EarlyLife->HasNewborn())
		{
			EarlyLife->SetEyeContact(bEyeContact);
		}
	}

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
	// Hinter dem Vorfilm steht die Kamera schon dort, wo der Startbildschirm aufblendet
	case EGenesisBootStage::Vorfilm:
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
