// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisMotherTypes.h"

/**
 * Die Mutter als reine Rechenlogik – ohne Welt, ohne Skelett.
 * Die Pose entsteht daraus erst in der Animationsinstanz.
 */
namespace GenesisMotherLogic
{
	/** Ein neuer Zustand mit eigenem Zufall (Seed ≠ 0). */
	GENESISPEOPLE_API FGenesisMotherState Begin(uint32 Seed);

	/** Führt die Mutter um DeltaSeconds weiter. */
	GENESISPEOPLE_API void Advance(FGenesisMotherState& State, const FGenesisMotherTuning& Tuning, const FGenesisMotherInputs& Inputs, float DeltaSeconds);

	/**
	 * 0..1 – wie weit die Brust gerade gehoben ist. Einatmen ist kürzer als Ausatmen,
	 * beide weich: Am Umkehrpunkt steht der Brustkorb einen Moment still.
	 */
	GENESISPEOPLE_API float GetBreathLift(const FGenesisMotherState& State, const FGenesisMotherTuning& Tuning);

	/** 0..1 – wie weit das Lid geschlossen ist. Es schließt in einem Drittel der Zeit und öffnet in zwei. */
	GENESISPEOPLE_API float GetBlinkClosure(const FGenesisMotherState& State, const FGenesisMotherTuning& Tuning);

	/** 0..1 – geglättete En-face-Haltung (das Heben beginnt und endet sanft). */
	GENESISPEOPLE_API float GetEnFaceBlend(const FGenesisMotherState& State);

	/** Ist Blickkontakt möglich? Erst wenn das Kind vor ihrem Gesicht ist. */
	GENESISPEOPLE_API bool IsFaceToFace(const FGenesisMotherState& State);
}
