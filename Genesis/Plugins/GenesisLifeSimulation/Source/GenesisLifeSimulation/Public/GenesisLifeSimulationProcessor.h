// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "GenesisKarma.h"
#include "GenesisLifeSimulationTypes.h"
#include "GenesisMemoryTrace.h"
#include "GenesisLifeSimulationProcessor.generated.h"

class UGenesisLifeActionDefinition;
struct FGenesisGenomePool;
struct FGenesisLifeAction;
struct FGenesisMemoryWorld;
struct FGenesisSimulationStep;
struct FGenesisTraitDefinition;

/** Wie eine beteiligte Person die Handlung wahrnimmt. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisObserverPerception
{
	GENERATED_BODY()

	FGuid ObserverId;
	EGenesisMemoryPerspective Role = EGenesisMemoryPerspective::Witness;
	bool bNoticed = true;
	bool bMisread = false;

	/** −1 feindselig … +1 wohlwollend. */
	float PerceivedIntent = 0.0f;
	FGameplayTagContainer PerceivedThemes;

	float CulturalDistance = 0.0f;
	float TabooOffense = 0.0f;
	float HypocrisyPerceived = 0.0f;
	float Arousal = 0.3f;

	/** Haltung der Gruppe dieses Beobachters zur Handlung −1..1. */
	float GroupStance = 0.0f;
};

/**
 * Gemeinsames Blackboard aller 14 Systeme für genau eine Handlung.
 * Jedes System liest die Ergebnisse der vorherigen und schreibt eigene – kein System rechnet isoliert.
 */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisLifeSimulationFrame
{
	GENERATED_BODY()

	FGuid EventId;
	FGenesisTimestamp Time;
	float Magnitude = 0.0f;
	float Intensity = 1.0f;

	/** Tatsächliche Absicht −1..1 (aus Motiv und Vertrauenswirkung). */
	float ActualIntent = 0.0f;

	/** Karma-Impuls – startet aus der Definition und wird von anderen Systemen moduliert. */
	FGenesisKarmaVector KarmaImpulse;

	TArray<FGenesisObserverPerception> Perceptions;

	// Ergebnisse der einzelnen Systeme
	float TabooViolation = 0.0f;
	float IndoctrinationAlignment = 0.0f;
	float BeliefConsistency = 0.0f;
	float ZeitgeistAlignment = 0.0f;
	float GroupPressure = 0.0f;
	bool bConformedAgainstConviction = false;
	float AltruismSignal = 0.0f;
	float Hypocrisy = 0.0f;
	float Dissonance = 0.0f;
	float StressLoad = 0.0f;

	TArray<FGenesisWeightedTag> EpigeneticExposure;
	TArray<FGuid> InfluenceEventIds;
	TArray<FGuid> SpawnedRumorIds;
	int32 ScheduledConsequences = 0;

	FGenesisObserverPerception* FindPerception(const FGuid& ObserverId)
	{
		return Perceptions.FindByPredicate([&ObserverId](const FGenesisObserverPerception& Perception) { return Perception.ObserverId == ObserverId; });
	}

	int32 CountNoticed() const
	{
		int32 Count = 0;
		for (const FGenesisObserverPerception& Perception : Perceptions)
		{
			Count += Perception.bNoticed ? 1 : 0;
		}
		return Count;
	}
};

/** Ausgelöste verzögerte Konsequenz. */
struct FGenesisFiredConsequence
{
	FGenesisScheduledConsequence Consequence;
	FGuid EventId;
};

/** Zugriff der Prozessoren auf Zustand und Nachbarsysteme. */
struct GENESISLIFESIMULATION_API FGenesisLifeSimulationContext
{
	FGenesisLifeSimulationContext(FGenesisLifeSimulationState& InState, const FGenesisLifeSimulationTuning& InTuning)
		: State(InState)
		, Tuning(InTuning)
	{
	}

	FGenesisLifeSimulationState& State;
	const FGenesisLifeSimulationTuning& Tuning;

	/** Optional (Tests ohne Gedächtnis möglich). */
	FGenesisMemoryWorld* Memory = nullptr;
	FGenesisGenomePool* Genomes = nullptr;
	const TArray<FGenesisTraitDefinition>* Traits = nullptr;

	FGenesisTimestamp Now;

	// Zeitsteuerung für AdvanceTime
	bool bDailyTick = false;
	double ElapsedDays = 0.0;
	bool bYearlyTick = false;
	double ElapsedYears = 0.0;

	TArray<FGenesisFiredConsequence> FiredConsequences;

	FGenesisRandomStream& Rng() { return State.Rng; }
};

/** Stufe in der Pipeline. Innerhalb einer Stufe entscheidet GetOrder(). */
UENUM()
enum class EGenesisSimulationStage : uint8
{
	/** Wie wird die Handlung wahrgenommen? */
	Perception,
	/** Was bedeutet sie für das Innere des Handelnden? */
	Inner,
	/** Was macht sie mit Beziehungen und Ruf? */
	Social,
	/** Integration in Karma, Körper und Kausalgraph. */
	Integration
};

/**
 * Basisklasse eines der 14 Systeme.
 * Prozessoren sind zustandslos – aller Zustand liegt in FGenesisLifeSimulationState.
 */
UCLASS(Abstract)
class GENESISLIFESIMULATION_API UGenesisLifeSimulationProcessor : public UObject
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetSystemTag() const PURE_VIRTUAL(UGenesisLifeSimulationProcessor::GetSystemTag, return FGameplayTag(););
	virtual EGenesisSimulationStage GetStage() const PURE_VIRTUAL(UGenesisLifeSimulationProcessor::GetStage, return EGenesisSimulationStage::Inner;);
	virtual int32 GetOrder() const { return 0; }

	/** Verarbeitet eine Handlung. */
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition,
		FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const
	{
	}

	/** Zeitbasierte Dynamik (Gerüchte, Erholung, Zeitgeist, fällige Konsequenzen …). */
	virtual void AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const
	{
	}
};
