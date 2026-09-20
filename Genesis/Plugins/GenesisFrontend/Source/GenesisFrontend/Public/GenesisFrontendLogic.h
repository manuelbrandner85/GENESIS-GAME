// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFrontendTypes.h"

/**
 * Die Regeln des Menüs – ohne Welt, ohne Actor, ohne Engine-Zustand.
 *
 * Alles, was eine Einstellung bedeutet, steht hier: ihre Grenzen, ihre Schrittweite, ihr
 * angezeigter Text und die Konsolenbefehle, die daraus folgen. Deshalb lässt sich prüfen,
 * ob eine Einstellung wirklich etwas tut, ohne das Spiel zu starten.
 */
namespace GenesisFrontendLogic
{
	/** Jeden Wert in seinen gültigen Bereich holen. Wird nach jedem Laden und jeder Änderung gerufen. */
	GENESISFRONTEND_API void Clamp(FGenesisPlayerSettings& Settings);

	/** Die Liste, wie das Menü sie zeigt – in dieser Reihenfolge. */
	GENESISFRONTEND_API TArray<FGenesisSettingEntry> BuildEntries(const FGenesisPlayerSettings& Settings);

	/**
	 * Einen Eintrag um eine Stufe nach links (-1) oder rechts (+1) verstellen.
	 * Gibt zurück, ob sich dadurch etwas geändert hat.
	 */
	GENESISFRONTEND_API bool Adjust(FGenesisPlayerSettings& Settings, FName Id, int32 Direction);

	/**
	 * Die Konsolenbefehle, die diese Einstellungen im laufenden Spiel bedeuten.
	 *
	 * Bewusst als Text und nicht als direkte CVar-Zugriffe: So steht in einem Test schwarz auf
	 * weiß, was eine Einstellung auslöst, und im Log steht dasselbe, was ausgeführt wurde.
	 */
	GENESISFRONTEND_API TArray<FString> RenderCommands(const FGenesisPlayerSettings& Settings);

	/** Anzeigename einer Grafikstufe. */
	GENESISFRONTEND_API FString QualityName(EGenesisQuality Quality);

	/** Anzeigename eines Fenstermodus. */
	GENESISFRONTEND_API FString WindowModeName(EGenesisWindowMode Mode);

	/** Die Einträge des Startbildschirms. `bRunActive` entscheidet, ob "Fortsetzen" dabei ist. */
	GENESISFRONTEND_API TArray<FString> MainMenuEntries(bool bRunActive);

	/** Index nach oben/unten bewegen, mit Umlauf – ein Menü darf nicht in einer Ecke hängen bleiben. */
	GENESISFRONTEND_API int32 Wrap(int32 Index, int32 Delta, int32 Count);
}
