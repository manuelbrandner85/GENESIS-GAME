// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisSpermRace.generated.h"

/**
 * Das Wettrennen zur Eizelle (GENESIS-037).
 *
 * Der Spieler führt eine einzige Zelle unter den rund 80, die es bis vor die Eizelle geschafft haben. Er lenkt
 * sie – innerhalb dessen, was ein Spermium kann – und er gibt beim Weg durch die Zona seine Kraft. Alles andere
 * entscheidet dieselbe Biologie, die für alle Zellen gilt: Strömung, Wand, Reifezustand, Lockstoff, Zufall.
 * Mehrere kommen an, nur eine verschmilzt (GENESIS-047, Docs/38). Ist es eine andere, beginnt dieses Leben
 * nicht – das Rennen fängt von vorn an.
 */
UENUM(BlueprintType)
enum class EGenesisRaceOutcome : uint8
{
	/** Kein Rennen (Zuschauen, Editor, Prolog). */
	None,
	Running,
	/** Die eigene Zelle ist verschmolzen – dieses Leben beginnt. */
	Won,
	/** Eine andere Zelle ist verschmolzen – nicht die schnellste gewinnt, sondern die, die zur richtigen Zeit bereit ist (Docs/38). */
	Lost
};

/** Stellschrauben des Rennens. */
USTRUCT(BlueprintType)
struct GENESISCONCEPTION_API FGenesisRaceTuning
{
	GENERATED_BODY()

	/**
	 * Das Feld: Die vordersten Zellen stehen so weit vor der Eizelle (µm), dahinter reicht es mit dieser
	 * Streuung weit zurück – die Zellen treffen nach und nach ein, nicht als Pulk (Wilcox 1995, Docs/38).
	 * Liegen beim Start schon Zellen an der Eizelle, kann kein Spieler gewinnen (GENESIS-037: 0 von 5).
	 */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "150")) float FieldDistanceUm = 950.0f;
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "10")) float FieldSpreadUm = 700.0f;

	/** Startabstand der eigenen Zelle vor der Eizellmitte (µm): vor dem Feld. */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "130")) float StartDistanceUm = 820.0f;

	/** Vitalität der eigenen Zelle: eine der stärksten im Feld (Mittel 0,75 ± 0,18) – die Seele wählt keine schwache. */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "0", ClampMax = "1")) float Vitality = 0.97f;

	/** Größter Lenkwinkel, den ein voll ausgelenkter Stick verlangt (Grad). Gedreht wird mit der Drehrate der Zelle. */
	UPROPERTY(EditAnywhere, Category = "Race") float SteerYawDegrees = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Race") float SteerPitchDegrees = 50.0f;

	/** Anstrengung in der Zona: je Tastendruck hinzu, je Sekunde (Echtzeit) wieder ab. */
	UPROPERTY(EditAnywhere, Category = "Race") float VigorPerPress = 0.22f;
	UPROPERTY(EditAnywhere, Category = "Race") float VigorDecayPerSecond = 0.7f;
};

namespace GenesisSpermRace
{
	/** Anstrengung nach Tastendrücken in diesem Bild; sie fällt von selbst ab. Echtzeit, nicht Simulationszeit. */
	GENESISCONCEPTION_API float UpdateVigor(float Vigor, int32 Presses, float RealDeltaSeconds, const FGenesisRaceTuning& Tuning);

	/**
	 * Gewünschte Richtung aus der Eingabe (X = rechts/links, Y = hoch/runter, je −1..1), bezogen auf die
	 * aktuelle Schwimmrichtung. Null ohne Eingabe – dann schwimmt die Zelle, wie Physik und Lockstoff sie treiben.
	 */
	GENESISCONCEPTION_API FVector SteerFromInput(const FVector& Heading, const FVector2D& Input, const FGenesisRaceTuning& Tuning);

	/** Platz im Feld: wie viele Zellen näher an der Zona sind (gebundene und bohrende zählen als vorn). */
	GENESISCONCEPTION_API int32 CountCellsAhead(const TArray<FGenesisSpermCell>& Cells, int32 PlayerIndex, const FGenesisOocyteState& Oocyte);

	/** Ergebnis nach einer Verschmelzung. */
	GENESISCONCEPTION_API EGenesisRaceOutcome OutcomeAfterFusion(int32 PlayerIndex, int32 FusedIndex);
}
