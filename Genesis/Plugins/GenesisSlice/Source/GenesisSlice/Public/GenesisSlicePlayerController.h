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
	void MenuAccept();
	void MenuBack();
	void MenuUp();
	void MenuDown();
	void MenuLeft();
	void MenuRight();

	/** Ist gerade ein Menü offen? Dann bekommt das Kind keine Eingabe. */
	bool IsMenuOpen() const;

	void PressCry() { bCryHeld = true; }
	void ReleaseCry() { bCryHeld = false; }
	void PressRoot() { bRootHeld = true; }
	void ReleaseRoot() { bRootHeld = false; }
	void LookRight(float Value) { LookInput.X = Value; }
	void LookUp(float Value) { LookInput.Y = Value; }

	bool bCryHeld = false;
	bool bRootHeld = false;
	float CryInput = 0.0f;
	float RootInput = 0.0f;
	FVector2D LookInput = FVector2D::ZeroVector;
	FVector2D LookOffset = FVector2D::ZeroVector;
};
