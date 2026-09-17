// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * GENESIS Developer HUD – Seitenregistry.
 *
 * Jedes System registriert eine Debug-Seite, die Textzeilen liefert (Karma, Soul Seed, DNA, Memory Graph ...).
 * Anzeige im Spiel über die Konsole:
 *   showdebug Genesis                → alle Seiten
 *   genesis.Debug.Page <SeitenId>    → nur diese Seite (leer = alle)
 *
 * In Shipping-Builds wird kein HUD-Hook installiert; registrierte Seiten sind dort wirkungslos.
 * Versteckte Werte (Karma, Resonanzen) erscheinen ausschließlich hier – nie in der Spiel-UI.
 */
struct GENESISCORE_API FGenesisDebugPage
{
	/** Eindeutige ID, z. B. "Soul". */
	FName Id;

	/** Überschrift im HUD. */
	FString Title;

	/** Liefert die anzuzeigenden Zeilen. World ist die Welt des HUDs (Zugriff auf GameInstance-Subsysteme). */
	TFunction<void(const UWorld* World, TArray<FString>& OutLines)> CollectLines;
};

namespace GenesisDebug
{
	/** Registriert oder ersetzt eine Seite mit gleicher Id. */
	GENESISCORE_API void RegisterPage(FGenesisDebugPage Page);

	GENESISCORE_API void UnregisterPage(FName PageId);

	/** Sammelt die Zeilen aller (bzw. der gefilterten) Seiten – auch für Tests und Konsolen-Dumps nutzbar. */
	GENESISCORE_API void CollectAllLines(const UWorld* World, TArray<FString>& OutLines);

	/** Installiert den HUD-Hook (vom Core-Modul beim Start aufgerufen). */
	void InstallHudHook();
	void RemoveHudHook();
}
