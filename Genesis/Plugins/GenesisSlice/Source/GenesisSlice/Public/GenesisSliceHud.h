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
	UPROPERTY(EditAnywhere, Category = "Genesis|Hud")
	float FadeAfterUseSeconds = 4.0f;

private:
	void DrawLine(const FString& Text, float LineIndex, float Alpha);

	float CryUsedSeconds = -1.0f;
	float RootUsedSeconds = -1.0f;
};
