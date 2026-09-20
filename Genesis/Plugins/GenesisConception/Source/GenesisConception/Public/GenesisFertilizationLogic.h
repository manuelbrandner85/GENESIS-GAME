// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"

/**
 * Befruchtung: der Weg der letzten Mikrometer.
 *
 * Ablauf wie in der Reproduktionsbiologie:
 * 1. **Lockwirkung** – aus dem Cumulus tritt Progesteron aus; hyperaktivierte (kapazitierte) Zellen richten sich danach aus.
 * 2. **Cumulus** – in der Gallerte kommen die Zellen langsamer voran.
 * 3. **Bindung** – an der Zona pellucida binden nur kapazitierte Zellen zuverlässig.
 * 4. **Akrosomreaktion** – die Kappe platzt auf und gibt Enzyme frei.
 * 5. **Durchdringung** – die Zelle bohrt sich mit Enzymen und Schlagkraft durch die 14 µm dicke Zona; manche bleiben stecken.
 * 6. **Verschmelzung** – die erste Zelle, die durchkommt, verschmilzt mit der Eizelle.
 * 7. **Cortikalreaktion** – die Zona verhärtet; alle anderen bleiben draußen (Polyspermie-Block).
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

	/** Darstellung einer gebundenen oder bohrenden Zelle: Kopf an der Zona, Achse zur Eizelle. */
	GENESISCONCEPTION_API FTransform ComputeAttachedTransform(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte);
}
