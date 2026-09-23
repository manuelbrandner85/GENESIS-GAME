// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSliceTypes.h"
#include "GenesisEmbryoTypes.h"

/**
 * Die Regeln des Durchlaufs – zustandslos, ohne Welt, ohne Karten.
 *
 * Damit lässt sich prüfen, was sonst nur im Spiel sichtbar wäre: dass keine Phase übersprungen wird,
 * dass ein abgestorbener Keim den Durchlauf ehrlich beendet und dass die erste Stunde ein Ende hat.
 */
namespace GenesisSliceLogic
{
	/** Die Phase, die auf die aktuelle folgt. Gleich, wenn noch gewartet wird. */
	GENESISSLICE_API EGenesisSlicePhase NextPhase(EGenesisSlicePhase Current, const FGenesisSliceSignals& Signals,
		const FGenesisSliceTuning& Tuning, EGenesisSliceEnding& OutEnding);

	GENESISSLICE_API FString GetPhaseName(EGenesisSlicePhase Phase);
	GENESISSLICE_API FString GetEndingName(EGenesisSliceEnding Ending);

	/** Was in dieser Phase zu sehen ist – ein Satz für Anzeige und Log. */
	GENESISSLICE_API FString GetPhaseDescription(EGenesisSlicePhase Phase);

	/** Zeitraffer der ersten Woche: Stunden Keimzeit je Sekunde Echtzeit für diese Stufe. */
	GENESISSLICE_API float EmbryoHoursPerSecond(EGenesisEmbryoStage Stage, const FGenesisSliceTuning& Tuning);

	/**
	 * Was gerade im Keim geschieht – ein Satz wie die Beschriftung eines Zeitraffers aus dem Labor,
	 * mit den Zeiten, die man dort misst.
	 */
	GENESISSLICE_API FString DescribeEmbryoStage(EGenesisEmbryoStage Stage);

	/** Wo der Keim zu sehen ist: im Eileiter bis zum Schlüpfen, danach in der Gebärmutter (GENESIS-040). */
	GENESISSLICE_API FName MapForEmbryoStage(EGenesisEmbryoStage Stage, const FGenesisSliceTuning& Tuning);

	/** Wie MapForEmbryoStage – ab Tag EmbryoSceneFromDay aber die Fruchthöhle, in der der Embryo selbst zu sehen ist. */
	GENESISSLICE_API FName MapForEmbryo(EGenesisEmbryoStage Stage, float DayPostFertilization, const FGenesisSliceTuning& Tuning);

	/** Die Momente der Schwangerschaft nach der Wochentafel (Docs/34). */
	GENESISSLICE_API TArray<FGenesisGestationMoment> DefaultGestationMoments();

	/**
	 * Wo der Ablauf der Schwangerschaft nach ElapsedSeconds Echtzeit steht: Zeitraffer zum nächsten Moment (weich
	 * beschleunigt und abgebremst), im Moment die Weltzeit mit GestationMomentTimeScale, am Ende zur Geburt.
	 * StartHours: Stunden seit der Befruchtung beim Eintritt in die Phase.
	 */
	GENESISSLICE_API FGenesisGestationPlanPoint EvaluateGestationPlan(const FGenesisSliceTuning& Tuning, const TArray<double>& MomentStarts,
		double StartHours, float ElapsedSeconds);

	/** Die Startzeiten aller Momente (Stunden seit der Befruchtung) – einmal je Durchlauf berechnen, die Suche ist teuer. */
	GENESISSLICE_API TArray<double> ResolveGestationMoments(const FGenesisSliceTuning& Tuning, int32 MotherSeed);

	/** Stunden seit der Befruchtung, zu denen ein Moment beginnt – bei einer Situation im wirklichen Tag der Mutter gesucht. */
	GENESISSLICE_API double MomentStartHours(const FGenesisGestationMoment& Moment, int32 MotherSeed);

	/** Gesamtdauer des Ablaufs in Echtzeit (s). */
	GENESISSLICE_API float GestationPlanSeconds(const FGenesisSliceTuning& Tuning);
}
