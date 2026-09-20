// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisFrontendTypes.h"
#include "GenesisFrontendSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FGenesisOnStartRequested);
DECLARE_MULTICAST_DELEGATE(FGenesisOnRestartRequested);

/**
 * Menü und Einstellungen an einer Stelle.
 *
 * Das Menü ist kein eigener Level und kein Widget-Asset, sondern ein Zustand: Wer ihn öffnet,
 * hält die Welt an und bekommt eine Liste. Gezeichnet wird er vom HUD, bedient von der
 * Steuerung – beides weiß nichts über den Inhalt, es fragt hier nach.
 *
 * Warum kein UMG: Der gesamte Aufbau dieses Projekts entsteht kopflos aus Skripten. Ein
 * Widget-Blueprint wäre der einzige Bestandteil, der nur im Editor von Hand entstehen kann.
 */
UCLASS()
class GENESISFRONTEND_API UGenesisFrontendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Einstellungen -------------------------------------------------------------------------

	const FGenesisPlayerSettings& GetSettings() const { return Settings; }

	/** Einstellungen auf das laufende Spiel anwenden (Konsolenbefehle, Fenster, Lautstärken). */
	void ApplySettings();

	/** Aus der GameUserSettings.ini lesen. Fehlt ein Wert, bleibt der Vorgabewert stehen. */
	void LoadSettings();

	/** In die GameUserSettings.ini schreiben. */
	void SaveSettings();

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

	/** Der Durchlauf soll starten. Die Regie hängt sich hier ein. */
	FGenesisOnStartRequested OnStartRequested;
	/** Der Durchlauf soll von vorn beginnen. */
	FGenesisOnRestartRequested OnRestartRequested;

private:
	void ApplyWindowMode();
	void ApplyVolumes();
	void SetGamePaused(bool bPaused);

	UPROPERTY()
	FGenesisPlayerSettings Settings;

	EGenesisMenuPage Page = EGenesisMenuPage::Keine;
	int32 Selection = 0;
	bool bRunActive = false;
};
