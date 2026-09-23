// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GenesisSliceHud.generated.h"

/**
 * Die einzige Anzeige, die dieses Spiel in der Szene hat: zwei Zeilen unten im Bild.
 *
 * Kein Balken, keine Zahl, kein Symbol. Was das Kind spürt, soll man am Bild und am Ton merken –
 * Wärme an der Farbe, Ruhe an der Kamera, Hunger am Schreien. Die Anzeige sagt nur, **was möglich ist**,
 * und auch das nur so lange, bis der Spieler es getan hat.
 */
UCLASS()
class GENESISSLICE_API AGenesisSliceHud : public AHUD
{
	GENERATED_BODY()

public:
	AGenesisSliceHud();

	virtual void DrawHUD() override;

	/** Wie lange ein Hinweis stehen bleibt, nachdem der Spieler ihn zum ersten Mal befolgt hat (s). */
	/** Schriftschnitt der Titelschrift (Cinzel). Das Font-Asset dazu wird zur Laufzeit gebaut. */
	UPROPERTY(EditDefaultsOnly, Category = "Genesis|Hud")
	TObjectPtr<class UFontFace> TitleFontFace;

	UPROPERTY(Transient)
	TObjectPtr<UFont> TitleFont;

	/** Ein Verlauf von oben (dunkel) nach unten (klar) für den Schleier hinter der Schrift. */
	UPROPERTY(Transient)
	TObjectPtr<class UTexture2D> VeilGradient;

	UPROPERTY(EditAnywhere, Category = "Genesis|Hud")
	float FadeAfterUseSeconds = 4.0f;

private:
	void DrawLine(const FString& Text, float LineIndex, float Alpha);

	/** Startablauf: Karten, Prolog mit Untertiteln, Titel, Kapitelkarte, Abspann. Gibt zurück, ob er das Bild für sich hat. */
	bool DrawBoot();
	void DrawBlack(float Alpha);
	void DrawCentered(const FString& Text, float Y, float Scale, float Alpha, bool bLarge);

	/** Startbildschirm und Pausenmenü. Gibt zurück, ob gezeichnet wurde. */
	bool DrawMenu();
	void DrawMenuTitle(const FString& Text, float Y, float Scale);

	/**
	 * Titelzeile in der Schrift des Covers: Großbuchstaben mit weitem Abstand, Gold, ein weicher Schein
	 * (Docs/31_Bildsprache.md). Liefert die Breite des Schriftzugs zurück – die feine Linie darunter richtet sich danach.
	 */
	float DrawCoverTitle(const FString& Text, float Y, float Scale, const FLinearColor& Colour, float Tracking, float Glow);
	/** Im Mutterleib: was das Kind spürt (Sinneszeile) und was der Spieler in dieser Woche tun kann (Docs/37). */
	void DrawWomb(float Scale, const struct FGenesisGestationPlanPoint& Plan, const TArray<struct FGenesisGestationMoment>& Moments, bool bInRun);

	/** Die Titelschrift (Cinzel, SIL OFL) als Laufzeitschrift – das Font-Asset entsteht erst hier. */
	UFont* GetTitleFont();

	/** Legt den Verlauf einmal an (256 Stufen); mit einzelnen Streifen blieben feine Linien im Bild stehen. */
	UTexture2D* GetVeilGradient();

	/** Der Schleier über der laufenden Szene: gleichmäßig plus ein Verlauf, der nach oben dunkler wird. */
	void DrawVeil(float FlatAlpha, float GradientAlpha);
	void DrawMenuHint(const FString& Text, float Y, float Scale, float Alpha);
	void DrawMenuRow(const FString& Left, const FString& Right, float Y, bool bSelected, float Scale, bool bSection = false);

	float CryUsedSeconds = -1.0f;
	float RootUsedSeconds = -1.0f;
	float SeekFaceUsedSeconds = -1.0f;

	/** Das Wettrennen: Messwerte der eigenen Zelle und Hinweise. Gibt zurück, ob gezeichnet wurde. */
	bool DrawRace(const class UGenesisSliceDirector& Director);
	float SteerHintUsedSeconds = -1.0f;
	/** Bahn der eigenen Zelle (Weltpositionen des Kopfes, alle 0,1 s). */
	TArray<FVector> PlayerTrack;
	float TrackSampleSeconds = 0.0f;
};
