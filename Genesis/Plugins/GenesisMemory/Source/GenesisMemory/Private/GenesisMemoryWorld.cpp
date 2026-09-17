// GENESIS: Der Kreislauf des Lebens

#include "GenesisMemoryWorld.h"

namespace
{
	constexpr uint64 MemoryWorldSalt = 0x3E3021ull;
}

void FGenesisMemoryWorld::Reset(uint64 WorldSeed)
{
	Graph.Reset();
	Stores.Reset();
	StoreIndex.Reset();
	LastDecayTime = FGenesisTimestamp();
	Rng = FGenesisRandomStream(WorldSeed).Derive(MemoryWorldSalt);
}

void FGenesisMemoryWorld::RebuildIndices()
{
	Graph.RebuildIndices();

	StoreIndex.Reset();
	for (int32 Index = 0; Index < Stores.Num(); ++Index)
	{
		StoreIndex.Add(Stores[Index].OwnerId, Index);
	}
}

const FGenesisCausalEvent* FGenesisMemoryWorld::RecordEvent(FGenesisCausalEvent Event)
{
	if (!Event.EventId.IsValid())
	{
		Event.EventId = Rng.NewGuid();
	}
	return Graph.RecordEvent(Event);
}

FGenesisMemoryStore& FGenesisMemoryWorld::GetOrCreateStore(const FGuid& OwnerId)
{
	if (const int32* Found = StoreIndex.Find(OwnerId))
	{
		return Stores[*Found];
	}

	const int32 NewIndex = Stores.AddDefaulted();
	Stores[NewIndex].OwnerId = OwnerId;
	StoreIndex.Add(OwnerId, NewIndex);
	return Stores[NewIndex];
}

const FGenesisMemoryStore* FGenesisMemoryWorld::FindStore(const FGuid& OwnerId) const
{
	const int32* Found = StoreIndex.Find(OwnerId);
	return Found ? &Stores[*Found] : nullptr;
}

FGenesisMemoryStore* FGenesisMemoryWorld::FindStoreMutable(const FGuid& OwnerId)
{
	const int32* Found = StoreIndex.Find(OwnerId);
	return Found ? &Stores[*Found] : nullptr;
}

FGuid FGenesisMemoryWorld::EncodeMemory(const FGuid& OwnerId, const FGuid& EventId, const FGenesisEncodingContext& Context, const FGenesisTimestamp& Now)
{
	const FGenesisCausalEvent* Event = Graph.FindEvent(EventId);
	if (!Event || !OwnerId.IsValid())
	{
		return FGuid();
	}

	FGenesisMemoryStore& Store = GetOrCreateStore(OwnerId);
	return GenesisMemoryLogic::Encode(Store, *Event, Context, Now, Rng).TraceId;
}

int32 FGenesisMemoryWorld::DecayAll(double ElapsedYears, const FGenesisMemoryDynamicsParams& Params)
{
	int32 Forgotten = 0;
	for (FGenesisMemoryStore& Store : Stores)
	{
		Forgotten += GenesisMemoryLogic::Decay(Store, ElapsedYears, Params);
	}
	return Forgotten;
}

int32 FGenesisMemoryWorld::Compact(float MaxMagnitude, float MinBridgedStrength)
{
	TSet<FGuid> RememberedEvents;
	for (const FGenesisMemoryStore& Store : Stores)
	{
		for (const FGenesisMemoryTrace& Trace : Store.Traces)
		{
			RememberedEvents.Add(Trace.EventId);
		}
	}

	return Graph.PruneEvents([&RememberedEvents, MaxMagnitude](const FGenesisCausalEvent& Event)
	{
		// Offene Fäden und erinnerte Ereignisse bleiben – GENESIS erinnert sich
		return Event.bResolved && Event.Magnitude <= MaxMagnitude && !RememberedEvents.Contains(Event.EventId);
	}, MinBridgedStrength);
}
