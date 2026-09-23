// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisSpermRace.generated.h"

/**
 * Das Wettrennen zur Eizelle (GENESIS-037).
 *
 * Der Spieler führt eine einzige Zelle aus Tausenden. Er lenkt sie – innerhalb dessen, was ein Spermium
 * kann – und er gibt beim Bohren durch die Zona seine Kraft. Alles andere entscheidet dieselbe Physik,
 * die für alle Zellen gilt: Strömung, Wand, Lockstoff, Zufall. Nur eine Zelle verschmilzt. Ist es eine
 * andere, beginnt dieses Leben nicht – das Rennen fängt von vorn an.
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
	 * Das Feld rückt gemeinsam an: Mitte des Pulks vor der Eizelle und seine Streuung (µm).
	 * Beim Zuschauen (GENESIS-030) lag der Pulk bei 320 ± 220 µm – dann liegen beim Start schon Zellen
	 * an der Eizelle, und kein Spieler kann gewinnen (gemessen: 0 von 5 Siegen, auch perfekt gelenkt).
	 */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "150")) float FieldDistanceUm = 420.0f;
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "10")) float FieldSpreadUm = 90.0f;

	/**
	 * Startabstand der eigenen Zelle vor der Eizellmitte (µm): an der Spitze der ersten Reihe
	 * (das Feld steht bei 420 ± 90 µm, seine vordersten Zellen um 240 µm). Aus 260 µm gewann ein guter
	 * Spieler gegen 6000 Zellen nur 2 von 5 Rennen, zwei davon um weniger als einen Mikrometer verloren.
	 */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "130")) float StartDistanceUm = 235.0f;

	/** Vitalität der eigenen Zelle: eine der stärksten im Feld (Mittel 0,75 ± 0,18) – die Seele wählt keine schwache. */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "0", ClampMax = "1")) float Vitality = 0.97f;

	/** Größter Lenkwinkel, den ein voll ausgelenkter Stick verlangt (Grad). Gedreht wird mit der Drehrate der Zelle. */
	UPROPERTY(EditAnywhere, Category = "Race") float SteerYawDegrees = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Race") float SteerPitchDegrees = 50.0f;

	/** Anstrengung beim Bohren: je Tastendruck hinzu, je Sekunde wieder ab. */
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
