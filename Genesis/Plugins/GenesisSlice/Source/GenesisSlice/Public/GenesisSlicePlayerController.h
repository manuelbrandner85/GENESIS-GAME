// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenesisSlicePlayerController.generated.h"

/**
 * Was der Spieler tun kann – und in der ersten Stunde ist das genau drei Dinge.
 *
 * Ein Neugeborenes kann nicht greifen, nicht sprechen, nicht weggehen. Es kann **rufen**,
 * es kann **suchen**, und es kann **hinsehen**. Mehr Werkzeuge hat ein Mensch an seinem ersten Tag
 * nicht, und deshalb bekommt der Spieler auch keine mehr. Alles andere entscheidet die Welt.
 *
 * Die Tasten liegen in `Config/DefaultInput.ini`. Absichtlich klassische Action-Mappings statt
 * Enhanced-Input-Assets: Der ganze Aufbau dieses Projekts läuft kopflos über Skripte, und
 * ein Eingabe-Asset, das nur im Editor entsteht, wäre der einzige Schritt, der das nicht tut.
 */
UCLASS()
class GENESISSLICE_API AGenesisSlicePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGenesisSlicePlayerController();

	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Wie schnell das Schreien an- und abschwillt (Anteil je Sekunde). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Input")
	float CryRampPerSecond = 2.5f;

	/** Wie schnell das Suchen an- und abschwillt. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Input")
	float RootRampPerSecond = 1.6f;

	/** Wie weit das Kind den Kopf drehen kann (Grad). Viel ist das nicht. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Input")
	float MaxLookDegrees = 22.0f;

	/** Blickrichtung relativ zur Ruhelage (Grad) – die Kamera liest sie aus. */
	FVector2D GetLookOffsetDegrees() const { return LookOffset; }

	/**
	 * Sucht das Kind das Gesicht der Mutter? Wer auf ihrer Brust eine Weile nach oben sieht, sucht
	 * sie – sie antwortet darauf und holt es zu sich hoch. Erst ein deutlicher Blick nach unten
	 * lässt es wieder los. So bleibt der Blickkontakt, auch wenn der Spieler den Stick loslässt.
	 */
	bool IsSeekingFace() const { return bSeekingFace; }

	/** Hat der Spieler das Mikroskop schon einmal geschwenkt? Für den Hinweis im Bild. */
	bool HasMovedMicroscope() const { return bMicroscopeMoved; }

	/** Wie lange der Blick oben bleiben muss, bis er als Suchen gilt (s). */
	UPROPERTY(EditAnywhere, Category = "Genesis|Input")
	float SeekFaceSeconds = 0.6f;

	/** 0..1 – was der Spieler gerade tut. Für die Anzeige. */
	float GetCryInput() const { return CryInput; }
	float GetRootInput() const { return RootInput; }

	/**
	 * Wie eine Handlung dem Spieler genannt wird, z. B. "Leertaste / A".
	 *
	 * Gelesen aus den Einstellungen, nicht in die Anzeige geschrieben: Ein Hinweis auf eine Taste,
	 * die gar nicht belegt ist, wäre eine Lüge – und genau das passiert, wenn beides getrennt gepflegt wird.
	 */
	static FString DescribeAction(FName Action);

private:
	/** Menü-Eingaben. Sie laufen auch, während das Spiel pausiert ist. */
	void MenuToggle();
	/** Irgendeine Taste – im Startablauf heißt das „weiter". */
	void AnyKeyPressed();
	void MenuAccept();
	void MenuBack();
	void MenuUp();
	void MenuDown();
	void MenuLeft();
	void MenuRight();

	/** Ist gerade ein Menü offen? Dann bekommt das Kind keine Eingabe. */
	bool IsMenuOpen() const;

	void PressCry() { bCryHeld = true; ++StrokePresses; bStrokeUsed = true; }
	void SteerRight(float Value) { SteerInput.X = Value; }
	void SteerUp(float Value) { SteerInput.Y = Value; }
	void ReleaseCry() { bCryHeld = false; }
	void PressRoot() { bRootHeld = true; }
	void ReleaseRoot() { bRootHeld = false; }
	void LookRight(float Value) { LookInput.X = Value; }
	void LookUp(float Value) { LookInput.Y = Value; }
	/** Im Mutterleib (GENESIS-044 Teil 2a, Docs/37): Hand halten, treten, Mund, strecken, greifen, Augen. */
	void PressWombHand() { bWombHand = true; }
	void ReleaseWombHand() { bWombHand = false; }
	void PressKick() { ++KickPresses; }
	void PressMouth() { bMouthHeld = true; MouthHeldSeconds = 0.0f; }
	void ReleaseMouth() { bMouthHeld = false; if (MouthHeldSeconds < 0.25f) { ++MouthTaps; } }
	void PressStretch() { bStretchHeld = true; StretchHeldSeconds = 0.0f; }
	void ReleaseStretch() { bStretchHeld = false; if (StretchHeldSeconds < 0.5f) { ++StretchTaps; } }
	void PressGrasp() { bGraspHeld = true; }
	void ReleaseGrasp() { bGraspHeld = false; }
	void PressEyes() { ++EyeToggles; }
	bool bWombHand = false;
	bool bMouthHeld = false;
	bool bStretchHeld = false;
	bool bGraspHeld = false;
	float MouthHeldSeconds = 0.0f;
	float StretchHeldSeconds = 0.0f;
	int32 KickPresses = 0;
	int32 MouthTaps = 0;
	int32 StretchTaps = 0;
	int32 EyeToggles = 0;

	/** Im Rennen: halbe Ich-Perspektive ↔ Verfolgeransicht. */
	void ToggleView();

	bool bCryHeld = false;
	bool bRootHeld = false;
	float CryInput = 0.0f;
	float RootInput = 0.0f;
	FVector2D LookInput = FVector2D::ZeroVector;
	FVector2D LookOffset = FVector2D::ZeroVector;
	float SeekUpSeconds = 0.0f;
	bool bSeekingFace = false;
	bool bMicroscopeMoved = false;
	/** Das Wettrennen: Lenken und Schlagen (GENESIS-037). */
	FVector2D SteerInput = FVector2D::ZeroVector;
	int32 StrokePresses = 0;
	bool bSteerUsed = false;
	bool bStrokeUsed = false;

public:
	/** Für die Hinweise im Bild: Hat der Spieler schon gelenkt / geschlagen? */
	bool HasSteered() const { return bSteerUsed; }
	bool HasStroked() const { return bStrokeUsed; }
	bool HasToggledView() const { return bViewToggled; }

private:
	bool bViewToggled = false;
};
