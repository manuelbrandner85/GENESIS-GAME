// GENESIS: Der Kreislauf des Lebens

#include "GenesisFrontendSubsystem.h"
#include "GenesisFrontendLogic.h"
#include "GenesisAudioSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GenesisLog.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/App.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "FileMediaSource.h"
#include "MediaPlayer.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"

namespace
{
	const TCHAR* SettingsSection = TEXT("/Script/Genesis.PlayerSettings");

	FAutoConsoleCommandWithWorldAndArgs GenesisMenuCommand(
		TEXT("genesis.Menu"),
		TEXT("Menü öffnen (ohne Argument), schließen (0) oder direkt die Einstellungen (settings)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
			if (!Frontend)
			{
				return;
			}
			if (Args.Num() > 0 && Args[0] == TEXT("0"))
			{
				Frontend->CloseMenu();
			}
			else if (Args.Num() > 0 && Args[0].StartsWith(TEXT("s")))
			{
				Frontend->OpenMenu(EGenesisMenuPage::Einstellungen);
			}
			else
			{
				Frontend->OpenMenu(EGenesisMenuPage::Haupt);
			}
		}));

	/**
	 * Menü bedienen, ohne eine Taste zu drücken.
	 *
	 * Das ist kein Komfort, sondern die einzige Möglichkeit, die Kette Menü → Regie in dieser
	 * Umgebung überhaupt zu prüfen: Ein Tastendruck lässt sich hier nicht erzeugen, ein
	 * Konsolenbefehl schon. Beide laufen durch dieselben Funktionen.
	 */
	UGenesisFrontendSubsystem* FindFrontend(UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisMenuMoveCommand(
		TEXT("genesis.Menu.Move"),
		TEXT("Auswahl im Menü bewegen: -1 hoch, 1 runter."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisFrontendSubsystem* Frontend = FindFrontend(World))
			{
				Frontend->MoveSelection(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisMenuAdjustCommand(
		TEXT("genesis.Menu.Adjust"),
		TEXT("Ausgewählte Einstellung ändern: -1 nach links, 1 nach rechts."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisFrontendSubsystem* Frontend = FindFrontend(World))
			{
				Frontend->AdjustSelection(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBootPressCommand(
		TEXT("genesis.Boot.Press"),
		TEXT("Wie ein beliebiger Tastendruck im Startablauf (Karte überspringen, Titel → Menü)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisFrontendSubsystem* Frontend = FindFrontend(World))
			{
				Frontend->PressAnyKey();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisBootEndCommand(
		TEXT("genesis.Boot.End"),
		TEXT("Den spielbaren Abschnitt beenden: Abspann, dann Menü. Zum Prüfen, ohne ein ganzes Leben zu spielen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisFrontendSubsystem* Frontend = FindFrontend(World))
			{
				Frontend->EndLife();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisMenuAcceptCommand(
		TEXT("genesis.Menu.Accept"),
		TEXT("Den ausgewählten Menüeintrag bestätigen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisFrontendSubsystem* Frontend = FindFrontend(World))
			{
				Frontend->Accept();
			}
		}));
}

void UGenesisFrontendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
	// Noch nicht anwenden: Beim Start steht die Welt teilweise nicht. ApplySettings kommt vom
	// Spielmodus, sobald ein Spieler da ist.
}

void UGenesisFrontendSubsystem::Deinitialize()
{
	FinishFilm();
	SaveSettings();
	Super::Deinitialize();
}

void UGenesisFrontendSubsystem::LoadSettings()
{
	if (!GConfig)
	{
		return;
	}
	const FString& Ini = GGameUserSettingsIni;

	int32 IntValue = 0;
	float FloatValue = 0.0f;
	bool bValue = false;

	if (GConfig->GetInt(SettingsSection, TEXT("WindowMode"), IntValue, Ini)) { Settings.WindowMode = static_cast<EGenesisWindowMode>(FMath::Clamp(IntValue, 0, 2)); }
	if (GConfig->GetInt(SettingsSection, TEXT("ResolutionScalePercent"), IntValue, Ini)) { Settings.ResolutionScalePercent = IntValue; }
	if (GConfig->GetInt(SettingsSection, TEXT("FrameRateLimit"), IntValue, Ini)) { Settings.FrameRateLimit = IntValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("VSync"), bValue, Ini)) { Settings.bVSync = bValue; }
	if (GConfig->GetInt(SettingsSection, TEXT("Quality"), IntValue, Ini)) { Settings.Quality = static_cast<EGenesisQuality>(FMath::Clamp(IntValue, 0, 4)); }
	if (GConfig->GetBool(SettingsSection, TEXT("HardwareRayTracing"), bValue, Ini)) { Settings.bHardwareRayTracing = bValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("MotionBlur"), bValue, Ini)) { Settings.bMotionBlur = bValue; }
	if (GConfig->GetFloat(SettingsSection, TEXT("MasterVolume"), FloatValue, Ini)) { Settings.MasterVolume = FloatValue; }
	if (GConfig->GetFloat(SettingsSection, TEXT("VoiceVolume"), FloatValue, Ini)) { Settings.VoiceVolume = FloatValue; }
	if (GConfig->GetFloat(SettingsSection, TEXT("MusicVolume"), FloatValue, Ini)) { Settings.MusicVolume = FloatValue; }
	if (GConfig->GetFloat(SettingsSection, TEXT("WorldVolume"), FloatValue, Ini)) { Settings.WorldVolume = FloatValue; }
	if (GConfig->GetFloat(SettingsSection, TEXT("LookSensitivity"), FloatValue, Ini)) { Settings.LookSensitivity = FloatValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("InvertLookY"), bValue, Ini)) { Settings.bInvertLookY = bValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("Vibration"), bValue, Ini)) { Settings.bVibration = bValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("Subtitles"), bValue, Ini)) { Settings.bSubtitles = bValue; }
	if (GConfig->GetInt(SettingsSection, TEXT("TextScalePercent"), IntValue, Ini)) { Settings.TextScalePercent = IntValue; }
	if (GConfig->GetBool(SettingsSection, TEXT("ReduceFlashing"), bValue, Ini)) { Settings.bReduceFlashing = bValue; }

	// Eine von Hand verstellte Datei darf das Spiel nicht in einen unmöglichen Zustand bringen
	GenesisFrontendLogic::Clamp(Settings);
}

void UGenesisFrontendSubsystem::SaveSettings()
{
	if (!GConfig)
	{
		return;
	}
	const FString& Ini = GGameUserSettingsIni;

	GConfig->SetInt(SettingsSection, TEXT("WindowMode"), static_cast<int32>(Settings.WindowMode), Ini);
	GConfig->SetInt(SettingsSection, TEXT("ResolutionScalePercent"), Settings.ResolutionScalePercent, Ini);
	GConfig->SetInt(SettingsSection, TEXT("FrameRateLimit"), Settings.FrameRateLimit, Ini);
	GConfig->SetBool(SettingsSection, TEXT("VSync"), Settings.bVSync, Ini);
	GConfig->SetInt(SettingsSection, TEXT("Quality"), static_cast<int32>(Settings.Quality), Ini);
	GConfig->SetBool(SettingsSection, TEXT("HardwareRayTracing"), Settings.bHardwareRayTracing, Ini);
	GConfig->SetBool(SettingsSection, TEXT("MotionBlur"), Settings.bMotionBlur, Ini);
	GConfig->SetFloat(SettingsSection, TEXT("MasterVolume"), Settings.MasterVolume, Ini);
	GConfig->SetFloat(SettingsSection, TEXT("VoiceVolume"), Settings.VoiceVolume, Ini);
	GConfig->SetFloat(SettingsSection, TEXT("MusicVolume"), Settings.MusicVolume, Ini);
	GConfig->SetFloat(SettingsSection, TEXT("WorldVolume"), Settings.WorldVolume, Ini);
	GConfig->SetFloat(SettingsSection, TEXT("LookSensitivity"), Settings.LookSensitivity, Ini);
	GConfig->SetBool(SettingsSection, TEXT("InvertLookY"), Settings.bInvertLookY, Ini);
	GConfig->SetBool(SettingsSection, TEXT("Vibration"), Settings.bVibration, Ini);
	GConfig->SetBool(SettingsSection, TEXT("Subtitles"), Settings.bSubtitles, Ini);
	GConfig->SetInt(SettingsSection, TEXT("TextScalePercent"), Settings.TextScalePercent, Ini);
	GConfig->SetBool(SettingsSection, TEXT("ReduceFlashing"), Settings.bReduceFlashing, Ini);
	GConfig->Flush(false, Ini);
}

void UGenesisFrontendSubsystem::ApplySettings()
{
	GenesisFrontendLogic::Clamp(Settings);

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const TArray<FString> Commands = GenesisFrontendLogic::RenderCommands(Settings);
	for (const FString& Command : Commands)
	{
		if (GEngine)
		{
			GEngine->Exec(World, *Command);
		}
	}

	ApplyWindowMode();
	ApplyVolumes();

	UE_LOG(LogGenesis, Display, TEXT("Einstellungen angewendet: Grafik %s, Auflösung %d %%, Bildrate %s, Strahlen %s, Ton %d %%, Empfindlichkeit %.1f, Schrift %d %%"),
		*GenesisFrontendLogic::QualityName(Settings.Quality),
		Settings.ResolutionScalePercent,
		Settings.FrameRateLimit == 0 ? TEXT("unbegrenzt") : *FString::Printf(TEXT("%d"), Settings.FrameRateLimit),
		Settings.bHardwareRayTracing ? TEXT("an") : TEXT("aus"),
		FMath::RoundToInt(Settings.MasterVolume * 100.0f),
		Settings.LookSensitivity,
		Settings.TextScalePercent);
}

void UGenesisFrontendSubsystem::ApplyWindowMode()
{
	UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!User)
	{
		return;
	}
	EWindowMode::Type Mode = EWindowMode::WindowedFullscreen;
	switch (Settings.WindowMode)
	{
	case EGenesisWindowMode::Vollbild: Mode = EWindowMode::Fullscreen; break;
	case EGenesisWindowMode::Fenster:  Mode = EWindowMode::Windowed; break;
	default: Mode = EWindowMode::WindowedFullscreen; break;
	}
	if (User->GetFullscreenMode() != Mode)
	{
		User->SetFullscreenMode(Mode);
		User->ApplyResolutionSettings(false);
	}
	User->SetVSyncEnabled(Settings.bVSync);
	User->SetFrameRateLimit(static_cast<float>(Settings.FrameRateLimit));
	User->ApplyNonResolutionSettings();
}

void UGenesisFrontendSubsystem::ApplyVolumes()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	if (Audio)
	{
		Audio->SetUserVolumes(Settings.MasterVolume, Settings.VoiceVolume, Settings.MusicVolume, Settings.WorldVolume);
	}
}

TArray<FString> UGenesisFrontendSubsystem::GetMainEntries() const
{
	return GenesisFrontendLogic::MainMenuEntries(bRunActive);
}

TArray<FGenesisSettingEntry> UGenesisFrontendSubsystem::GetSettingEntries() const
{
	return GenesisFrontendLogic::BuildEntries(Settings);
}

// --- Startablauf ------------------------------------------------------------------------------

namespace
{
	/** Alle Töne des Rahmens liegen hier – der Ordner wird immer mitgekocht (DefaultGame.ini). */
	FString FrontendSoundPath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("/Game/Genesis/Frontend/Audio/%s.%s"), AssetName, AssetName);
	}
}

TStatId UGenesisFrontendSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGenesisFrontendSubsystem, STATGROUP_Tickables);
}

void UGenesisFrontendSubsystem::Tick(float DeltaTime)
{
	if (Boot.Stage == EGenesisBootStage::Aus)
	{
		return;
	}

	const float Delta = static_cast<float>(FApp::GetDeltaTime());

	// Die Untertitel folgen dem Bild: Der Abspieler sagt, wo der Film steht
	if (FilmPlayer && Boot.Stage == EGenesisBootStage::Vorfilm)
	{
		GenesisBootFlow::SetFilmTime(Boot, static_cast<float>(FilmPlayer->GetTime().GetTotalSeconds()));
	}
	if (FilmSound)
	{
		// Der Film trägt Stimme und Musik in einer Spur – er folgt der Gesamtlautstärke
		FilmSound->SetVolumeMultiplier(FMath::Clamp(Settings.MasterVolume, 0.0f, 1.0f));
	}
	if (FilmCloseIn >= 0.0f)
	{
		FilmCloseIn -= Delta;
		if (FilmCloseIn < 0.0f)
		{
			FinishFilm();
		}
	}

	// Echte Zeit, nicht Spielzeit: In der Pause steht die Spielzeit still, der Ablauf nicht
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Advance(Boot, Delta, Events);
	HandleBootEvents(Events);

	// Die Musik weicht der Stimme, wie im Film: Während der Erzähler spricht, geht sie um 7 dB
	// zurück – schnell hinunter, langsam wieder hinauf, damit man das Atmen der Mischung nicht hört.
	// Im Vorspann liegt sie zudem grundsätzlich tiefer als im Menü: Dort trägt die Stimme, hier die Musik.
	// Anlass war die Rückmeldung des Game Directors: „die stimme ist zu leise im vergleich zur musik".
	const bool bSpeaking = Voice && Voice->IsPlaying();
	const float DuckTarget = bSpeaking ? 0.45f : 1.0f;
	const float Rate = DuckTarget < MusicDuck ? 1.0f / 0.25f : 1.0f / 1.4f;
	MusicDuck = FMath::FInterpConstantTo(MusicDuck, DuckTarget, static_cast<float>(FApp::GetDeltaTime()), Rate);
	const bool bIntro = Boot.Stage == EGenesisBootStage::Studio || Boot.Stage == EGenesisBootStage::Engine
		|| Boot.Stage == EGenesisBootStage::Hinweis || Boot.Stage == EGenesisBootStage::Prolog;
	const float StageLevel = bIntro ? 0.6f : 1.0f;

	// Der Film bringt seine eigene Musik mit: Das Menüthema weicht ganz, in 1,5 s, und kehrt danach in
	// 4 s zurück – unter dem aufblendenden Startbildschirm, nicht als Sprung.
	const float FilmTarget = Boot.Stage == EGenesisBootStage::Vorfilm ? 0.0f : 1.0f;
	FilmDuck = FMath::FInterpConstantTo(FilmDuck, FilmTarget, Delta, FilmTarget < FilmDuck ? 1.0f / 1.5f : 1.0f / 4.0f);
	// Dasselbe für die Szene dahinter: Der Eileiter rauscht nicht unter den Film
	if (UGenesisAudioSubsystem* Audio = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisAudioSubsystem>() : nullptr)
	{
		Audio->SetSceneGain(FilmDuck);
	}

	// Lautstärke folgt den Reglern sofort – auch mitten in einem Satz
	if (Music)
	{
		Music->SetVolumeMultiplier(MusicVolume() * StageLevel * MusicDuck * FilmDuck);
	}
	if (Voice)
	{
		Voice->SetVolumeMultiplier(VoiceVolume());
	}
}

void UGenesisFrontendSubsystem::StartBoot()
{
	// Der Vorfilm liegt nicht im Repository (300 MB); fehlt er, erzählt der Prolog dieselben Sätze.
	// -genesisnofilm lässt ihn beim Prüfen weg, ohne die Datei anzufassen.
	const bool bFilm = FPaths::FileExists(FilmPath()) && !FParse::Param(FCommandLine::Get(), TEXT("genesisnofilm"));
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Start(Boot, Events, bFilm);
	HandleBootEvents(Events);
	UE_LOG(LogGenesis, Display, TEXT("Startablauf: beginnt (%s)."), bFilm ? TEXT("mit Vorfilm") : TEXT("mit Prolog, Vorfilm fehlt"));
}

FString UGenesisFrontendSubsystem::FilmPath()
{
	return FPaths::ProjectContentDir() / TEXT("Movies/GENESIS_Vorfilm.mp4");
}

UTexture* UGenesisFrontendSubsystem::GetFilmTexture() const
{
	if (Boot.Stage != EGenesisBootStage::Vorfilm || !FilmTexture || !FilmPlayer || FilmTexture->GetWidth() <= 0)
	{
		return nullptr;
	}
	return FilmTexture;
}

void UGenesisFrontendSubsystem::StartFilm()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	AWorldSettings* WorldSettings = World ? World->GetWorldSettings() : nullptr;
	if (!WorldSettings)
	{
		HandleFilmFailed(TEXT("keine Welt"));
		return;
	}

	FilmPlayer = NewObject<UMediaPlayer>(this);
	FilmPlayer->PlayOnOpen = true;
	FilmPlayer->SetLooping(false);
	FilmPlayer->OnEndReached.AddDynamic(this, &UGenesisFrontendSubsystem::HandleFilmEnded);
	FilmPlayer->OnMediaOpenFailed.AddDynamic(this, &UGenesisFrontendSubsystem::HandleFilmFailed);

	FilmTexture = NewObject<UMediaTexture>(this);
	FilmTexture->AutoClear = true;
	FilmTexture->ClearColor = FLinearColor::Black;
	FilmTexture->SetMediaPlayer(FilmPlayer);
	FilmTexture->UpdateResource();

	// Der Ton hängt an einem Actor, der sicher da ist und tickt – sonst holt niemand die Tonproben ab
	FilmSound = NewObject<UMediaSoundComponent>(WorldSettings);
	FilmSound->bIsUISound = true;
	FilmSound->SetMediaPlayer(FilmPlayer);
	FilmSound->RegisterComponent();
	FilmSound->SetVolumeMultiplier(FMath::Clamp(Settings.MasterVolume, 0.0f, 1.0f));
	FilmSound->Activate(true);

	FilmSource = NewObject<UFileMediaSource>(this);
	const FString FullPath = FPaths::ConvertRelativePathToFull(FilmPath());
	FilmSource->SetFilePath(FullPath);
	FilmCloseIn = -1.0f;

	if (!FilmPlayer->OpenSource(FilmSource))
	{
		HandleFilmFailed(FullPath);
		return;
	}
	UE_LOG(LogGenesis, Display, TEXT("Vorfilm: %s"), *FullPath);
}

void UGenesisFrontendSubsystem::StopFilm(float FadeSeconds)
{
	if (!FilmPlayer)
	{
		return;
	}
	// Wer überspringt, hört den Film ausklingen statt abreißen; das Bild ist sofort weg
	if (FilmSound && FadeSeconds > 0.0f)
	{
		FilmSound->FadeOut(FadeSeconds, 0.0f);
		FilmCloseIn = FadeSeconds;
	}
	else
	{
		FinishFilm();
	}
}

void UGenesisFrontendSubsystem::FinishFilm()
{
	FilmCloseIn = -1.0f;
	if (FilmPlayer)
	{
		FilmPlayer->OnEndReached.RemoveAll(this);
		FilmPlayer->OnMediaOpenFailed.RemoveAll(this);
		FilmPlayer->Close();
	}
	if (FilmSound)
	{
		FilmSound->Stop();
		FilmSound->DestroyComponent();
	}
	FilmPlayer = nullptr;
	FilmTexture = nullptr;
	FilmSource = nullptr;
	FilmSound = nullptr;
}

void UGenesisFrontendSubsystem::HandleFilmEnded()
{
	UE_LOG(LogGenesis, Display, TEXT("Vorfilm: zu Ende."));
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::FilmFinished(Boot, Events);
	HandleBootEvents(Events);
}

void UGenesisFrontendSubsystem::HandleFilmFailed(FString FailedUrl)
{
	// Kein Film heißt nicht: zwei Minuten Schwarz. Weiter zum Startbildschirm.
	UE_LOG(LogGenesis, Warning, TEXT("Vorfilm ließ sich nicht öffnen (%s) – weiter zum Startbildschirm."), *FailedUrl);
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::FilmFinished(Boot, Events);
	HandleBootEvents(Events);
}

bool UGenesisFrontendSubsystem::WantsAnyKey() const
{
	switch (Boot.Stage)
	{
	case EGenesisBootStage::Studio:
	case EGenesisBootStage::Engine:
	case EGenesisBootStage::Hinweis:
	case EGenesisBootStage::Vorfilm:
	case EGenesisBootStage::Prolog:
	case EGenesisBootStage::Titel:
	case EGenesisBootStage::Taste:
		return true;
	default:
		return false;
	}
}

void UGenesisFrontendSubsystem::PressAnyKey()
{
	if (!WantsAnyKey())
	{
		return;
	}
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Press(Boot, Events);
	HandleBootEvents(Events);
}

void UGenesisFrontendSubsystem::EndLife()
{
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::EndLife(Boot, Events);
	HandleBootEvents(Events);
}

void UGenesisFrontendSubsystem::HandleBootEvents(const TArray<FGenesisBootEvent>& Events)
{
	for (const FGenesisBootEvent& Event : Events)
	{
		switch (Event.Type)
		{
		case EGenesisBootEventType::StageEntered:
			UE_LOG(LogGenesis, Display, TEXT("Startablauf: %s"), *StaticEnum<EGenesisBootStage>()->GetNameStringByValue(static_cast<int64>(Event.Stage)));
			// Wer den Prolog überspringt, soll den Erzähler nicht mitten im Satz weiterreden hören
			if (Event.Stage != EGenesisBootStage::Prolog && Voice)
			{
				Voice->FadeOut(0.4f, 0.0f);
				Voice = nullptr;
			}
			// Dasselbe für den Film: Jede andere Stufe beendet ihn
			if (Event.Stage != EGenesisBootStage::Vorfilm && FilmPlayer)
			{
				StopFilm(0.6f);
			}
			switch (Event.Stage)
			{
			case EGenesisBootStage::Vorfilm:
				StartFilm();
				break;
			case EGenesisBootStage::Studio:
				StartMusic();
				break;
			case EGenesisBootStage::Menue:
				bRunActive = false;
				StartMusic();
				Page = EGenesisMenuPage::Haupt;
				Selection = 0;
				break;
			case EGenesisBootStage::Kapitel:
				// Die Musik geht mit dem Bild: Sie klingt aus, während es schwarz wird
				Page = EGenesisMenuPage::Keine;
				if (Music)
				{
					Music->FadeOut(2.5f, 0.0f);
					Music = nullptr;
				}
				break;
			case EGenesisBootStage::Ende:
				Page = EGenesisMenuPage::Keine;
				break;
			default:
				break;
			}
			break;

		case EGenesisBootEventType::PlayVoice:
			if (Voice)
			{
				Voice->Stop();
			}
			Voice = CreateSound(*Event.VoiceAsset.ToString(), false);
			if (Voice)
			{
				Voice->SetVolumeMultiplier(VoiceVolume());
				Voice->Play();
			}
			break;

		case EGenesisBootEventType::StartLife:
			bRunActive = true;
			SaveSettings();
			UE_LOG(LogGenesis, Display, TEXT("Startablauf: Ein Leben beginnt."));
			OnStartRequested.Broadcast();
			break;

		case EGenesisBootEventType::ReturnToMenu:
			bRunActive = false;
			UE_LOG(LogGenesis, Display, TEXT("Startablauf: zurück ins Menü."));
			OnReturnToMenuRequested.Broadcast();
			break;
		}
	}
}

UAudioComponent* UGenesisFrontendSubsystem::CreateSound(const TCHAR* AssetName, bool bPersist)
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	USoundBase* Sound = Cast<USoundBase>(FSoftObjectPath(FrontendSoundPath(AssetName)).TryLoad());
	if (!World || !Sound)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Startablauf: Ton %s fehlt."), AssetName);
		return nullptr;
	}
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(World, Sound, 1.0f, 1.0f, 0.0f, nullptr, bPersist, true);
	if (Component)
	{
		// Menü und Rahmen klingen auch in der Pause – sonst verstummt die Musik, sobald man pausiert
		Component->bIsUISound = true;
	}
	return Component;
}

void UGenesisFrontendSubsystem::PlayUiSound(const TCHAR* AssetName)
{
	if (UAudioComponent* Component = CreateSound(AssetName, false))
	{
		Component->SetVolumeMultiplier(FMath::Clamp(Settings.MasterVolume, 0.0f, 1.0f) * 0.55f);
		Component->Play();
	}
}

void UGenesisFrontendSubsystem::StartMusic()
{
	if (Music && Music->IsPlaying())
	{
		return;
	}
	// Über Levelwechsel hinweg: Die Rückkehr ins Menü lädt den Eileiter neu, die Musik läuft weiter
	Music = CreateSound(TEXT("MX_Frontend_Theme"), true);
	if (Music)
	{
		// Die Lautstärke trägt der Multiplikator (Regler, Vorspann, Absenken) – das Einblenden nur die Form
		Music->SetVolumeMultiplier(MusicVolume() * 0.6f);
		Music->FadeIn(4.0f, 1.0f);
	}
}

float UGenesisFrontendSubsystem::MusicVolume() const
{
	return FMath::Clamp(Settings.MasterVolume * Settings.MusicVolume, 0.0f, 1.0f);
}

float UGenesisFrontendSubsystem::VoiceVolume() const
{
	return FMath::Clamp(Settings.MasterVolume * Settings.VoiceVolume, 0.0f, 1.0f);
}

// --- Menü -------------------------------------------------------------------------------------

bool UGenesisFrontendSubsystem::MenuTakesInput() const
{
	return Page != EGenesisMenuPage::Keine && GenesisBootFlow::AcceptsMenuInput(Boot);
}

void UGenesisFrontendSubsystem::OpenMenu(EGenesisMenuPage InPage)
{
	Page = InPage;
	Selection = 0;
	SetGamePaused(Page != EGenesisMenuPage::Keine);
}

void UGenesisFrontendSubsystem::CloseMenu()
{
	Page = EGenesisMenuPage::Keine;
	SetGamePaused(false);
	SaveSettings();
}

void UGenesisFrontendSubsystem::ToggleMenu()
{
	// Während des Startablaufs ist die Pausentaste eine Taste wie jede andere. Das Überspringen
	// erledigt schon „beliebige Taste" – hier noch einmal zu drücken, hätte jeden Druck doppelt
	// gezählt und den Prolog mit einem einzigen Esc übersprungen.
	if (WantsAnyKey())
	{
		return;
	}
	// Kapitelkarte und Abspann lassen sich nicht pausieren – dort passiert gerade ein Übergang
	if (Boot.Stage == EGenesisBootStage::Kapitel || Boot.Stage == EGenesisBootStage::Ende)
	{
		return;
	}
	// Im Hauptmenü vor dem ersten Leben gibt es nichts, wohin man zurückkehren könnte
	if (Boot.Stage == EGenesisBootStage::Menue && Page == EGenesisMenuPage::Haupt)
	{
		return;
	}

	if (Page == EGenesisMenuPage::Keine)
	{
		OpenMenu(EGenesisMenuPage::Haupt);
		PlayUiSound(TEXT("UI_Accept"));
	}
	else if (Page == EGenesisMenuPage::Einstellungen)
	{
		// Aus den Einstellungen führt die Pausentaste zurück ins Hauptmenü, nicht direkt ins Spiel:
		// Sonst verlässt ein Fehlgriff die Einstellungen ungewollt.
		OpenMenu(EGenesisMenuPage::Haupt);
		PlayUiSound(TEXT("UI_Back"));
	}
	else if (bRunActive)
	{
		CloseMenu();
		PlayUiSound(TEXT("UI_Back"));
	}
}

void UGenesisFrontendSubsystem::MoveSelection(int32 Delta)
{
	if (!MenuTakesInput())
	{
		return;
	}
	const int32 Count = Page == EGenesisMenuPage::Einstellungen ? GetSettingEntries().Num() : GetMainEntries().Num();
	Selection = GenesisFrontendLogic::Wrap(Selection, Delta, Count);
	PlayUiSound(TEXT("UI_Move"));
}

void UGenesisFrontendSubsystem::AdjustSelection(int32 Direction)
{
	if (Page != EGenesisMenuPage::Einstellungen || !MenuTakesInput())
	{
		return;
	}
	const TArray<FGenesisSettingEntry> Entries = GetSettingEntries();
	if (!Entries.IsValidIndex(Selection))
	{
		return;
	}
	if (GenesisFrontendLogic::Adjust(Settings, Entries[Selection].Id, Direction))
	{
		// Sofort sichtbar und hörbar: Wer einen Regler bewegt, will das Ergebnis jetzt sehen,
		// nicht nach einem Bestätigen-Knopf.
		ApplySettings();
		PlayUiSound(TEXT("UI_Move"));
	}
}

void UGenesisFrontendSubsystem::Accept()
{
	if (!MenuTakesInput())
	{
		return;
	}
	if (Page == EGenesisMenuPage::Einstellungen)
	{
		// Auf einem Schalter ist Bestätigen dasselbe wie "nach rechts"
		AdjustSelection(1);
		return;
	}

	const TArray<FString> Entries = GetMainEntries();
	if (!Entries.IsValidIndex(Selection))
	{
		return;
	}
	const FString Chosen = Entries[Selection];
	PlayUiSound(TEXT("UI_Accept"));

	if (Chosen == TEXT("Leben beginnen") || Chosen == TEXT("Von vorn beginnen"))
	{
		Page = EGenesisMenuPage::Keine;
		SetGamePaused(false);
		SaveSettings();
		if (Boot.Stage == EGenesisBootStage::Aus)
		{
			// Ohne Startablauf (Editor, Messläufe): sofort, wie bisher
			bRunActive = true;
			OnStartRequested.Broadcast();
		}
		else
		{
			// Ein echtes Spiel wirft niemanden einfach in einen Level: Schwarzbild, Kapitelkarte,
			// frisch geladener Eileiter, dann erst das Bild.
			TArray<FGenesisBootEvent> Events;
			GenesisBootFlow::BeginLife(Boot, Events);
			HandleBootEvents(Events);
		}
	}
	else if (Chosen == TEXT("Weiterspielen"))
	{
		CloseMenu();
	}
	else if (Chosen == TEXT("Einstellungen"))
	{
		OpenMenu(EGenesisMenuPage::Einstellungen);
	}
	else if (Chosen == TEXT("Beenden"))
	{
		SaveSettings();
		if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		{
			UKismetSystemLibrary::QuitGame(World, World->GetFirstPlayerController(), EQuitPreference::Quit, false);
		}
	}
}

void UGenesisFrontendSubsystem::Back()
{
	if (!MenuTakesInput())
	{
		return;
	}
	if (Page == EGenesisMenuPage::Einstellungen)
	{
		SaveSettings();
		OpenMenu(EGenesisMenuPage::Haupt);
		PlayUiSound(TEXT("UI_Back"));
	}
	else if (Page == EGenesisMenuPage::Haupt && bRunActive)
	{
		CloseMenu();
		PlayUiSound(TEXT("UI_Back"));
	}
}

void UGenesisFrontendSubsystem::SetGamePaused(bool bPaused)
{
	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller)
	{
		return;
	}
	// Nur pausieren, wenn tatsächlich etwas läuft: Der Startbildschirm vor dem ersten Leben soll
	// die Szene dahinter weiterlaufen lassen – sie ist das Bild des Startbildschirms.
	if (bPaused && !bRunActive)
	{
		return;
	}
	Controller->SetPause(bPaused);
}
