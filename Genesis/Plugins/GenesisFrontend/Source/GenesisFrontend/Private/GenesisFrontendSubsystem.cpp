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
	if (Page == EGenesisMenuPage::Keine)
	{
		OpenMenu(EGenesisMenuPage::Haupt);
	}
	else
	{
		// Aus den Einstellungen führt die Pausentaste zurück ins Hauptmenü, nicht direkt ins Spiel:
		// Sonst verlässt ein Fehlgriff die Einstellungen ungewollt.
		if (Page == EGenesisMenuPage::Einstellungen)
		{
			OpenMenu(EGenesisMenuPage::Haupt);
		}
		else if (bRunActive)
		{
			CloseMenu();
		}
	}
}

void UGenesisFrontendSubsystem::MoveSelection(int32 Delta)
{
	const int32 Count = Page == EGenesisMenuPage::Einstellungen ? GetSettingEntries().Num() : GetMainEntries().Num();
	Selection = GenesisFrontendLogic::Wrap(Selection, Delta, Count);
}

void UGenesisFrontendSubsystem::AdjustSelection(int32 Direction)
{
	if (Page != EGenesisMenuPage::Einstellungen)
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
	}
}

void UGenesisFrontendSubsystem::Accept()
{
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
	const FString& Chosen = Entries[Selection];

	if (Chosen == TEXT("Leben beginnen"))
	{
		CloseMenu();
		bRunActive = true;
		OnStartRequested.Broadcast();
	}
	else if (Chosen == TEXT("Weiterspielen"))
	{
		CloseMenu();
	}
	else if (Chosen == TEXT("Von vorn beginnen"))
	{
		CloseMenu();
		OnRestartRequested.Broadcast();
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
	if (Page == EGenesisMenuPage::Einstellungen)
	{
		SaveSettings();
		OpenMenu(EGenesisMenuPage::Haupt);
	}
	else if (Page == EGenesisMenuPage::Haupt && bRunActive)
	{
		CloseMenu();
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
