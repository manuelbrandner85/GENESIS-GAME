// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GenesisFrontendTypes.h"
#include "GenesisBootFlow.h"
#include "GenesisFrontendSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UTexture;
class UMediaPlayer;
class UMediaTexture;
class UFileMediaSource;
class UMediaSoundComponent;

DECLARE_MULTICAST_DELEGATE(FGenesisOnStartRequested);
DECLARE_MULTICAST_DELEGATE(FGenesisOnReturnToMenuRequested);

/**
 * Der Rahmen des Spiels an einer Stelle: Startablauf, Menü, Einstellungen, Menümusik.
 *
 * Das Menü ist kein eigener Level und kein Widget-Asset, sondern ein Zustand: Wer ihn öffnet,
 * hält die Welt an und bekommt eine Liste. Gezeichnet wird er vom HUD, bedient von der
 * Steuerung – beides weiß nichts über den Inhalt, es fragt hier nach.
 *
 * Der Zustand lebt in der GameInstance und überdauert damit jeden Levelwechsel. Das ist wichtig:
 * Wenn ein Leben beginnt, wird der Eileiter frisch geladen, und danach darf nicht wieder das
 * Studiologo erscheinen.
 *
 * Warum kein UMG: Der gesamte Aufbau dieses Projekts entsteht kopflos aus Skripten. Ein
 * Widget-Blueprint wäre der einzige Bestandteil, der nur im Editor von Hand entstehen kann.
 */
UCLASS()
class GENESISFRONTEND_API UGenesisFrontendSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject – läuft auch in der Pause, sonst stünde die Menümusik still
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;

	// --- Einstellungen -------------------------------------------------------------------------

	const FGenesisPlayerSettings& GetSettings() const { return Settings; }

	/** Einstellungen auf das laufende Spiel anwenden (Konsolenbefehle, Fenster, Lautstärken). */
	void ApplySettings();

	/** Aus der GameUserSettings.ini lesen. Fehlt ein Wert, bleibt der Vorgabewert stehen. */
	void LoadSettings();

	/** In die GameUserSettings.ini schreiben. */
	void SaveSettings();

	// --- Startablauf ---------------------------------------------------------------------------

	/** Den Ablauf vom Studiologo an beginnen (spielbare Fassung). */
	void StartBoot();

	/** Irgendeine Taste wurde gedrückt – überspringt Karten, öffnet vom Titel das Menü. */
	void PressAnyKey();

	/** Der spielbare Abschnitt ist zu Ende: Abspann, dann zurück ins Menü. */
	void EndLife();

	const FGenesisBootState& GetBootState() const { return Boot; }
	EGenesisBootStage GetBootStage() const { return Boot.Stage; }

	/** Soll die Steuerung gerade Tastendrücke als „weiter" werten statt als Menübedienung? */
	bool WantsAnyKey() const;

	/** Das laufende Bild des Vorfilms – nur während er läuft und schon ein Bild da ist, sonst nullptr. */
	UTexture* GetFilmTexture() const;

	/** Wo die Filmdatei liegt: Content/Movies/GENESIS_Vorfilm.mp4 (entsteht mit Tools/Intro/build_vorfilm.py). */
	static FString FilmPath();

	// --- Menü ----------------------------------------------------------------------------------

	EGenesisMenuPage GetPage() const { return Page; }
	int32 GetSelection() const { return Selection; }

	/** Die Zeilen, die gerade zu zeichnen sind. */
	TArray<FString> GetMainEntries() const;
	TArray<FGenesisSettingEntry> GetSettingEntries() const;

	/** Menü öffnen (Startbildschirm oder Pause) beziehungsweise schließen. */
	void OpenMenu(EGenesisMenuPage InPage);
	void CloseMenu();
	void ToggleMenu();

	/** Auswahl bewegen. */
	void MoveSelection(int32 Delta);
	/** Wert nach links/rechts ändern – nur auf der Einstellungsseite. */
	void AdjustSelection(int32 Direction);
	/** Bestätigen. */
	void Accept();
	/** Zurück: Einstellungen → Haupt, Haupt → Spiel (wenn ein Durchlauf läuft). */
	void Back();

	/** Läuft gerade ein Durchlauf? Entscheidet über "Weiterspielen" statt "Leben beginnen". */
	bool IsRunActive() const { return bRunActive; }
	void SetRunActive(bool bActive) { bRunActive = bActive; }

	/** Ein Leben soll beginnen – das Bild ist schwarz, der Level darf frisch geladen werden. */
	FGenesisOnStartRequested OnStartRequested;
	/** Zurück zum Menü: Der Hintergrund des Menüs soll wieder der unberührte Eileiter sein. */
	FGenesisOnReturnToMenuRequested OnReturnToMenuRequested;

private:
	void ApplyWindowMode();
	void ApplyVolumes();
	void SetGamePaused(bool bPaused);
	void HandleBootEvents(const TArray<FGenesisBootEvent>& Events);
	bool MenuTakesInput() const;

	/** Ein 2D-Ton, der auch in der Pause und über einen Levelwechsel hinweg spielt. */
	UAudioComponent* CreateSound(const TCHAR* AssetName, bool bPersist);
	void PlayUiSound(const TCHAR* AssetName);
	void StartMusic();
	float MusicVolume() const;
	float VoiceVolume() const;

	/** Den Vorfilm öffnen und abspielen. */
	void StartFilm();
	/** Den Film beenden; der Ton klingt über FadeSeconds aus, erst dann wird geschlossen. */
	void StopFilm(float FadeSeconds);
	void FinishFilm();

	UFUNCTION()
	void HandleFilmEnded();
	UFUNCTION()
	void HandleFilmFailed(FString FailedUrl);

	UPROPERTY()
	TObjectPtr<UMediaPlayer> FilmPlayer;

	UPROPERTY()
	TObjectPtr<UMediaTexture> FilmTexture;

	UPROPERTY()
	TObjectPtr<UFileMediaSource> FilmSource;

	UPROPERTY()
	TObjectPtr<UMediaSoundComponent> FilmSound;

	/** Sekunden, bis ein ausklingender Film geschlossen wird (negativ: nichts zu schließen). */
	float FilmCloseIn = -1.0f;

	/** Wie weit die Menümusik für den Film zurückgenommen ist (1 = gar nicht, 0 = stumm). */
	float FilmDuck = 1.0f;

	UPROPERTY()
	FGenesisPlayerSettings Settings;

	UPROPERTY()
	FGenesisBootState Boot;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Music;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Voice;

	/** Wie weit die Musik gerade für die Stimme zurückgenommen ist (1 = gar nicht). */
	float MusicDuck = 1.0f;

	EGenesisMenuPage Page = EGenesisMenuPage::Keine;
	int32 Selection = 0;
	bool bRunActive = false;
};
