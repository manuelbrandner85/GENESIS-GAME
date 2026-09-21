// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenesisSliceGameMode.generated.h"

/**
 * Setzt Steuerung und Anzeige des Vertical Slice.
 *
 * Kein Pawn: Der Spieler bewegt keinen Körper durch die Welt, er **ist** der Körper, und der liegt.
 * Die Kamera gehört den Szenen-Rigs (Geburtskanal, Mikroskop) – sie machen sich beim Start selbst
 * zum Blickpunkt.
 */
UCLASS()
class GENESISSLICE_API AGenesisSliceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGenesisSliceGameMode();

	/** Einstellungen anwenden und – in der spielbaren Fassung – den Startablauf beginnen. */
	virtual void BeginPlay() override;

	/** Führt im Startablauf die Kamera (Prolog-Fahrt, Blick hinter dem Menü). */
	virtual void Tick(float DeltaSeconds) override;

private:
	class UGenesisFrontendSubsystem* GetFrontend() const;
};
