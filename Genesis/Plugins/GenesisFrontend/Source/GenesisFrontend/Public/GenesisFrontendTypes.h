// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFrontendTypes.generated.h"

/**
 * Grafikstufe. Die Namen sind die der Engine-Skalierbarkeit (sg.*), damit jeder, der die
 * Unreal-Werte kennt, sofort weiß, was er einstellt – und damit wir keine eigene Zahlenwelt
 * erfinden, die niemand nachprüfen kann.
 */
UENUM(BlueprintType)
enum class EGenesisQuality : uint8
{
	Niedrig = 0,
	Mittel = 1,
	Hoch = 2,
	Episch = 3,
	Ultra = 4
};

UENUM(BlueprintType)
enum class EGenesisWindowMode : uint8
{
	/** Echtes Vollbild: die Grafikkarte gehört dem Spiel allein. */
	Vollbild = 0,
	/** Randloses Fenster in Bildschirmgröße: schnelles Umschalten, minimal weniger Leistung. */
	FensterVollbild = 1,
	Fenster = 2
};

/** Welche Seite des Menüs gerade offen ist. */
UENUM(BlueprintType)
enum class EGenesisMenuPage : uint8
{
	/** Kein Menü – das Spiel läuft. */
	Keine,
	/** Startbildschirm beziehungsweise Pausenmenü. */
	Haupt,
	Einstellungen
};

/** Art eines Einstellungseintrags – davon hängt ab, was links/rechts bewirkt. */
UENUM(BlueprintType)
enum class EGenesisSettingKind : uint8
{
	/** An oder aus. */
	Schalter,
	/** Eine Auswahl aus benannten Stufen. */
	Auswahl,
	/** Ein Wert mit Schrittweite. */
	Regler
};

/**
 * Alles, was der Spieler einstellen kann.
 *
 * Bewusst ein einfaches Struct ohne Engine-Abhängigkeit: So lässt sich jede Regel dazu
 * (Grenzen, Schrittweiten, welche Konsolenbefehle daraus folgen) ohne laufende Welt prüfen.
 * Gespeichert wird in die GameUserSettings.ini, nicht in einen Spielstand – Einstellungen
 * gehören zum Gerät, nicht zum Leben, das man spielt.
 */
USTRUCT(BlueprintType)
struct GENESISFRONTEND_API FGenesisPlayerSettings
{
	GENERATED_BODY()

	// --- Bild ---------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bild") EGenesisWindowMode WindowMode = EGenesisWindowMode::FensterVollbild;
	/** Auflösungsskala in Prozent. Unter 100 rendert das Spiel kleiner und skaliert hoch. */
	UPROPERTY(EditAnywhere, Category = "Bild") int32 ResolutionScalePercent = 100;
	/** 0 = unbegrenzt. */
	UPROPERTY(EditAnywhere, Category = "Bild") int32 FrameRateLimit = 0;
	UPROPERTY(EditAnywhere, Category = "Bild") bool bVSync = false;

	// --- Grafik -------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Grafik") EGenesisQuality Quality = EGenesisQuality::Episch;
	/** Lumen mit Hardware-Strahlen. Kostet Leistung, bringt echte Spiegelungen und Schatten. */
	UPROPERTY(EditAnywhere, Category = "Grafik") bool bHardwareRayTracing = true;
	UPROPERTY(EditAnywhere, Category = "Grafik") bool bMotionBlur = true;

	// --- Ton ----------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Ton") float MasterVolume = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Ton") float VoiceVolume = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Ton") float MusicVolume = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Ton") float WorldVolume = 1.0f;

	// --- Steuerung ----------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Steuerung") float LookSensitivity = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Steuerung") bool bInvertLookY = false;
	/** Rumpeln im Controller. Wer es nicht mag oder braucht, schaltet es ab. */
	UPROPERTY(EditAnywhere, Category = "Steuerung") bool bVibration = true;

	// --- Barrierefreiheit ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Barrierefreiheit") bool bSubtitles = true;
	/** Schriftgröße in Prozent. Die Anzeige wächst mit, nicht nur der Text. */
	UPROPERTY(EditAnywhere, Category = "Barrierefreiheit") int32 TextScalePercent = 100;
	/** Blitze, harte Schnitte und schnelle Helligkeitswechsel dämpfen. */
	UPROPERTY(EditAnywhere, Category = "Barrierefreiheit") bool bReduceFlashing = false;
};

/** Ein Eintrag, wie das Menü ihn zeichnet. Die Anzeige entscheidet nichts – sie malt nur. */
USTRUCT(BlueprintType)
struct GENESISFRONTEND_API FGenesisSettingEntry
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	/** Überschrift, unter der dieser Eintrag steht ("Bild", "Ton", …). Leer heißt: gleiche wie oben. */
	UPROPERTY() FString Section;
	UPROPERTY() FString Label;
	UPROPERTY() FString Value;
	/** Ein Satz, der erklärt, was die Einstellung kostet oder bringt. */
	UPROPERTY() FString Hint;
	UPROPERTY() EGenesisSettingKind Kind = EGenesisSettingKind::Schalter;
};
