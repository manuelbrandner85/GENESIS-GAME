// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisCausalGraph.h"
#include "GenesisMemoryTrace.h"
#include "GenesisRandom.h"
#include "GenesisMemoryWorld.generated.h"

/**
 * Gesamter Erinnerungszustand einer Welt: objektiver Kausalgraph + subjektive Erinnerungen aller simulierten Personen.
 * Reiner Zustand + Logik – ohne Subsystem testbar und von GenesisLifeSimulation direkt nutzbar.
 */
USTRUCT()
struct GENESISMEMORY_API FGenesisMemoryWorld
{
	GENERATED_BODY()

	void Reset(uint64 WorldSeed);

	/** Muss nach dem Laden aufgerufen werden. */
	void RebuildIndices();

	/** Deterministische neue Ereignis-ID. */
	FGuid NewEventId() { return Rng.NewGuid(); }

	/** Nimmt ein Ereignis auf; vergibt eine ID, falls keine gesetzt ist. */
	const FGenesisCausalEvent* RecordEvent(FGenesisCausalEvent Event);

	bool AddLink(const FGuid& CauseId, const FGuid& EffectId, EGenesisCausalLinkType Type, float Strength)
	{
		return Graph.AddLink(CauseId, EffectId, Type, Strength);
	}

	FGenesisMemoryStore& GetOrCreateStore(const FGuid& OwnerId);
	const FGenesisMemoryStore* FindStore(const FGuid& OwnerId) const;
	FGenesisMemoryStore* FindStoreMutable(const FGuid& OwnerId);

	/** Prägt ein Ereignis in das Gedächtnis einer Person ein. @return TraceId oder ungültig */
	FGuid EncodeMemory(const FGuid& OwnerId, const FGuid& EventId, const FGenesisEncodingContext& Context, const FGenesisTimestamp& Now);

	/** Lässt alle Erinnerungen altern. @return Anzahl vergessener Spuren */
	int32 DecayAll(double ElapsedYears, const FGenesisMemoryDynamicsParams& Params);

	/**
	 * Verdichtet den Graphen: abgeschlossene, unbedeutende Ereignisse ohne lebende Erinnerung werden entfernt,
	 * ihre Kausalketten bleiben über Brückenkanten erhalten.
	 */
	int32 Compact(float MaxMagnitude, float MinBridgedStrength);

	FGenesisCausalGraph& GetGraph() { return Graph; }
	const FGenesisCausalGraph& GetGraph() const { return Graph; }
	const TArray<FGenesisMemoryStore>& GetStores() const { return Stores; }
	FGenesisRandomStream& GetRng() { return Rng; }

	/** Zeitpunkt des letzten Zerfallsschritts (vom Subsystem geführt). */
	UPROPERTY()
	FGenesisTimestamp LastDecayTime;

private:
	UPROPERTY()
	FGenesisCausalGraph Graph;

	UPROPERTY()
	TArray<FGenesisMemoryStore> Stores;

	UPROPERTY()
	FGenesisRandomStream Rng;

	TMap<FGuid, int32> StoreIndex;
};
