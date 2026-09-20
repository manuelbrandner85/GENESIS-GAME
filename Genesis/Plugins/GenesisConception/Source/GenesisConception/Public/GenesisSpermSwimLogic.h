// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSpermSwimTypes.h"

/**
 * Zustandsloses, deterministisches Schwimmmodell menschlicher Spermien im Eileiter.
 *
 * Verhalten (belegt in der Reproduktionsbiologie):
 * - Rotationsdiffusion: Schwimmbahnen sind nie gerade, hyperaktivierte deutlich unruhiger.
 * - Rheotaxis: Spermien richten sich gegen die Strömung aus – der Zilienstrom zur Gebärmutter weist ihnen den Weg zum Eierstock.
 * - Wandbindung (Thigmotaxis/Hydrodynamik): Zellen sammeln sich an Oberflächen und schwimmen an ihnen entlang.
 * - Hyperaktivierung: Wechsel zu kräftigem, asymmetrischem Schlag mit wenig Vortrieb; erleichtert das Lösen von der Wand.
 */
namespace GenesisSpermSwimLogic
{
	/** Neue Zelle an einer zufälligen Stelle im Kanal. */
	GENESISCONCEPTION_API FGenesisSpermCell CreateCell(uint64 Seed, float Vitality, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning);

	/** Setzt Geschwindigkeit und Schlagparameter passend zur Bewegungsart (innerhalb der Bandbreiten, nach Individualität). */
	GENESISCONCEPTION_API void ApplyMotility(FGenesisSpermCell& Cell, EGenesisSpermMotility Motility, const FGenesisSpermSwimTuning& Tuning);

	/** Zilienströmung an einer Stelle (µm/s). */
	GENESISCONCEPTION_API FVector FlowAt(const FVector& Position, const FGenesisOviductChannel& Channel);

	/** 0..1 – Nähe zur Wand innerhalb der Anziehungsdistanz. */
	GENESISCONCEPTION_API float WallProximity(const FVector& Position, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning);

	/** Ein fester Simulationsschritt. */
	GENESISCONCEPTION_API void Step(FGenesisSpermCell& Cell, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning, float DeltaSeconds);

	/** Rückt um beliebige Zeit in festen Schritten vor. Liefert die Anzahl der Schritte. */
	GENESISCONCEPTION_API int32 Advance(FGenesisSpermCell& Cell, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning, float DeltaSeconds);

	/**
	 * Darstellung: Kopfspitze mit seitlicher Kopfauslenkung (ALH), Gier-Pendeln im Schlagtakt, Rollen um die Längsachse.
	 * Lokale X-Achse = Schwimmrichtung (Kopf vorn), Z = Normale der flachen Kopfseite. Einheit µm (= Unreal-Einheiten).
	 */
	GENESISCONCEPTION_API FTransform ComputeVisualTransform(const FGenesisSpermCell& Cell);

	/** Kopfposition inklusive seitlicher Auslenkung (für Kurvengeschwindigkeit VCL und Kamera). */
	GENESISCONCEPTION_API FVector ComputeHeadPosition(const FGenesisSpermCell& Cell);

	/**
	 * Schlagebene der Zelle: OutSide liegt in der Ebene (Richtung der Auslenkung), OutNormal steht senkrecht darauf.
	 * Die Geißelwelle ist nur sichtbar, wenn man entlang OutNormal blickt – die Kamera richtet sich danach aus.
	 */
	GENESISCONCEPTION_API void ComputeBeatFrame(const FGenesisSpermCell& Cell, FVector& OutSide, FVector& OutNormal);

	/** Per-Instance-Daten für das Material: [0] Schlagphase (Zyklen), [1] Amplitude an der Geißelspitze (µm), [2] Asymmetrie, [3] Wellenlänge (µm). */
	GENESISCONCEPTION_API void ComputeMaterialData(const FGenesisSpermCell& Cell, float OutData[4]);
}
