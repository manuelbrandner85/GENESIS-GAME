// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisBootFlow.generated.h"

/**
 * Wo das Spiel zwischen Programmstart und erstem Atemzug gerade steht.
 *
 * Ein Spiel beginnt nicht mit einem Menü, sondern mit einem Auftritt: Wer es startet, wird
 * empfangen, bekommt eine Stimmung, bevor er eine Entscheidung treffen muss, und wird beim
 * Übergang ins Spiel nicht einfach in einen Level geworfen. Genau diese Abfolge steht hier.
 */
UENUM(BlueprintType)
enum class EGenesisBootStage : uint8
{
	/** Noch nicht begonnen (Editor, Messläufe). */
	Aus,
	/** „GENESIS TEAM präsentiert" */
	Studio,
	/** „Entwickelt mit Unreal Engine 5" */
	Engine,
	/** Hinweis zu Lichtwechseln und Kopfhörern */
	Hinweis,
	/**
	 * Der Vorfilm (Docs/33): vom Spermium bis zum Sterbebett und zurück, endet auf der Titelkarte.
	 * Er ersetzt den Prolog – beide sprechen dieselben Sätze. Fehlt die Filmdatei, kommt der Prolog.
	 */
	Vorfilm,
	/** Kamerafahrt durch den Eileiter mit Erzählerstimme */
	Prolog,
	/** Der Titel, groß */
	Titel,
	/** „Drücke eine beliebige Taste" */
	Taste,
	/** Hauptmenü */
	Menue,
	/** Übergang ins Spiel: Schwarzbild, Kapitelkarte, Level wird frisch geladen */
	Kapitel,
	/** Das Spiel läuft */
	Spiel,
	/** Ende des spielbaren Abschnitts, danach zurück ins Menü */
	Ende
};

/** Ein Satz des Erzählers im Prolog. */
USTRUCT(BlueprintType)
struct GENESISFRONTEND_API FGenesisPrologueBeat
{
	GENERATED_BODY()

	/** Sekunden ab Beginn des Prologs. */
	UPROPERTY() float StartSeconds = 0.0f;
	/** Wie lange die Aufnahme dauert (s) – gemessen an der Datei, nicht geschätzt. */
	UPROPERTY() float DurationSeconds = 0.0f;
	/** Asset-Name der Aufnahme unter /Game/Genesis/Frontend/Audio. */
	UPROPERTY() FName VoiceAsset;
	/** Untertitel – genau der gesprochene Text. */
	UPROPERTY() FString Subtitle;
};

/** Was beim Fortschreiben passiert ist – die Darstellung reagiert darauf, die Logik nicht. */
UENUM()
enum class EGenesisBootEventType : uint8
{
	StageEntered,
	PlayVoice,
	/** Der Bildschirm ist jetzt ganz schwarz: Hier darf der Level frisch geladen werden. */
	StartLife,
	/** Das Spiel soll zurück ins Menü – der Spielabschnitt ist zu Ende. */
	ReturnToMenu
};

struct GENESISFRONTEND_API FGenesisBootEvent
{
	EGenesisBootEventType Type = EGenesisBootEventType::StageEntered;
	EGenesisBootStage Stage = EGenesisBootStage::Aus;
	FName VoiceAsset;
};

/** Der ganze Zustand des Ablaufs. Klein, kopierbar, ohne Welt. */
USTRUCT(BlueprintType)
struct GENESISFRONTEND_API FGenesisBootState
{
	GENERATED_BODY()

	UPROPERTY() EGenesisBootStage Stage = EGenesisBootStage::Aus;
	/** Sekunden in der aktuellen Stufe. */
	UPROPERTY() float StageSeconds = 0.0f;
	/** Nächster Erzählersatz im Prolog. */
	UPROPERTY() int32 NextBeat = 0;
	/** Hat der Spieler im Prolog schon einmal gedrückt? Dann steht der Hinweis zum Überspringen da. */
	UPROPERTY() bool bSkipArmed = false;
	UPROPERTY() float SkipArmedSeconds = 0.0f;
	/** Wurde der Level für dieses Leben schon angefordert? Genau einmal pro Kapitelkarte. */
	UPROPERTY() bool bLifeStarted = false;
	/** Liegt der Vorfilm bereit? Sonst folgt auf den Hinweis der Prolog. */
	UPROPERTY() bool bFilmAvailable = false;
	/**
	 * Wo der Film gerade steht (s), gemeldet vom Abspieler. Die Untertitel folgen dem Bild, nicht der
	 * Uhr des Ablaufs: Die zählt ein hängendes Bild nur als Zehntelsekunde, der Film läuft weiter.
	 * Negativ, solange nichts gemeldet ist – dann gilt die Uhr des Ablaufs.
	 */
	UPROPERTY() float FilmSeconds = -1.0f;
};

namespace GenesisBootFlow
{
	/** Wie lange eine Stufe von selbst dauert. Negativ: bis der Spieler etwas tut. */
	GENESISFRONTEND_API float StageDuration(EGenesisBootStage Stage);

	/** Die Sätze des Prologs, zeitlich geordnet. */
	GENESISFRONTEND_API const TArray<FGenesisPrologueBeat>& PrologueBeats();

	/** Länge des Vorfilms (s): 2916 Bilder bei 24 Bildern je Sekunde. */
	GENESISFRONTEND_API float FilmLength();

	/** Die Untertitel des Vorfilms – an der Sprachspur gemessen, zeitlich geordnet. Ohne Tonspur-Namen. */
	GENESISFRONTEND_API const TArray<FGenesisPrologueBeat>& FilmSubtitles();

	/** Der Abspieler meldet, wo der Film steht. */
	GENESISFRONTEND_API void SetFilmTime(FGenesisBootState& State, float Seconds);

	/** Der Film ist zu Ende – oder ließ sich nicht öffnen. Beides führt zum Startbildschirm. */
	GENESISFRONTEND_API void FilmFinished(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents);

	/** Abstand der Kamera zur Eizelle im Prolog (µm) – eine langsame Fahrt auf sie zu. */
	GENESISFRONTEND_API float PrologueCameraDistance(float Seconds);

	/** Wie schwarz das Bild gerade ist (0 = klar, 1 = schwarz). */
	GENESISFRONTEND_API float FadeAlpha(const FGenesisBootState& State);

	/** Der Untertitel, der jetzt zu sehen ist – leer, wenn gerade niemand spricht. */
	GENESISFRONTEND_API FString CurrentSubtitle(const FGenesisBootState& State);

	/** Die Textzeilen einer Karte (Studio, Engine, Hinweis, Titel, Kapitel, Ende). */
	GENESISFRONTEND_API TArray<FString> CardLines(EGenesisBootStage Stage);

	/** Wie sichtbar der Text einer Karte gerade ist (weiches Ein- und Ausblenden). */
	GENESISFRONTEND_API float CardAlpha(const FGenesisBootState& State);

	/** Den Ablauf beginnen. Mit Film folgt auf den Hinweis der Vorfilm, sonst der Prolog. */
	GENESISFRONTEND_API void Start(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents, bool bFilmAvailable = false);

	/** Zeit fortschreiben. */
	GENESISFRONTEND_API void Advance(FGenesisBootState& State, float DeltaSeconds, TArray<FGenesisBootEvent>& OutEvents);

	/**
	 * Der Spieler hat eine Taste gedrückt.
	 *
	 * Karten lassen sich mit einem Druck überspringen. Vorfilm und Prolog brauchen zwei: Der erste zeigt
	 * „Nochmal drücken zum Überspringen" – sonst ist die Erzählung mit einem versehentlichen Druck
	 * weg, und man bekommt sie nie wieder zu hören. So machen es die meisten großen Spiele.
	 */
	GENESISFRONTEND_API void Press(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents);

	/** Aus dem Menü heraus ein Leben beginnen (auch „von vorn"). */
	GENESISFRONTEND_API void BeginLife(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents);

	/** Der spielbare Abschnitt ist zu Ende. */
	GENESISFRONTEND_API void EndLife(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents);

	/** Nimmt das Menü gerade Eingaben an? Nicht im selben Augenblick, in dem es erscheint. */
	GENESISFRONTEND_API bool AcceptsMenuInput(const FGenesisBootState& State);

	GENESISFRONTEND_API void EnterStage(FGenesisBootState& State, EGenesisBootStage Stage, TArray<FGenesisBootEvent>& OutEvents);
}
