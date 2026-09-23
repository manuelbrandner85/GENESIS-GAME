// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"

/**
 * Befruchtung: der Weg der letzten Mikrometer.
 *
 * Ablauf wie in der Reproduktionsbiologie (Zeiten biologisch, Belege in Docs/38):
 * 1. **Lockwirkung** – aus dem Cumulus tritt Progesteron aus; nur kapazitierte Zellen folgen ihm und hyperaktivieren.
 * 2. **Cumulus** – in der Gallerte kommen die Zellen langsamer voran; die Akrosomreaktion kann schon hier beginnen.
 * 3. **Bindung** – an der Zona pellucida binden nur kapazitierte, hyperaktivierte Zellen.
 * 4. **Akrosomreaktion** – falls noch nicht geschehen, jetzt an der Zona.
 * 5. **Durchdringung** – die Zelle schiebt sich schräg und vor allem mechanisch durch die 17 µm dicke Zona,
 *    rund 13 Minuten lang; manche bleiben stecken und lösen sich wieder.
 * 6. **Perivitelliner Spalt** – der Kopf legt sich flach an die Eizellmembran; nach 16 ± 6 Minuten verschmelzen
 *    die Membranen. Oft liegen mehrere Zellen hier – verschmelzen kann nur eine.
 * 7. **Cortikalreaktion** – über Minuten verändert sich die Zona: Gebundene lösen sich, wer in ihr steckt, bleibt
 *    stecken, Überzählige im Spalt bleiben liegen (Polyspermie-Block).
 */
namespace GenesisFertilizationLogic
{
	GENESISCONCEPTION_API EGenesisSpermPhase GetPhase(const FGenesisSpermCell& Cell);
	GENESISCONCEPTION_API void SetPhase(FGenesisSpermCell& Cell, EGenesisSpermPhase Phase);

	/** Abstand der Kopfspitze zur Außenfläche der Zona (negativ = innerhalb). */
	GENESISCONCEPTION_API float DistanceToZona(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte);

	/**
	 * Ein Simulationsschritt für alle Zellen und die Eizelle.
	 * Liefert true, wenn in diesem Schritt eine Verschmelzung stattgefunden hat (dann steht das Ergebnis in OutResult).
	 */
	GENESISCONCEPTION_API bool Step(TArray<FGenesisSpermCell>& Cells, FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& SwimTuning, const FGenesisFertilizationTuning& Tuning, float DeltaSeconds,
		FGenesisFertilizationResult& OutResult);

	/**
	 * Baut das Zellfeld des äußeren Cumulus (deterministisch aus dem Seed): zwischen der Corona und
	 * CumulusRadiusUm, nach außen dünner, mit Mindestabstand. Bild (AGenesisOocyte) und Physik nutzen dasselbe Feld.
	 */
	GENESISCONCEPTION_API TSharedPtr<const FGenesisCumulusField> BuildCumulus(const FGenesisOocyteState& Oocyte, const FGenesisCumulusTuning& Tuning);

	/**
	 * Schiebt eine schwimmende Zelle aus den Cumuluszellen hinaus und lässt sie an ihnen entlanggleiten.
	 * Liefert true, wenn sie eine Zelle berührt hat.
	 */
	GENESISCONCEPTION_API bool ResolveCumulusContact(FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte);

	/** Hängt die Zelle an oder in der Eizelle (gebunden, in der Zona, im Spalt, verschmolzen, in der Zona steckengeblieben)? */
	GENESISCONCEPTION_API bool IsAttached(const FGenesisSpermCell& Cell);

	/**
	 * Darstellung einer anhaftenden Zelle: Kopf an bzw. in der Zona, Achse entlang ihrer Schwimmrichtung
	 * (schräg beim Eindringen, fast flach im Spalt).
	 */
	GENESISCONCEPTION_API FTransform ComputeAttachedTransform(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte);
}
