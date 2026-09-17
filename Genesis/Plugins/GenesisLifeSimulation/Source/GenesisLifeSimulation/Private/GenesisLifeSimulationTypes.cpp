// GENESIS: Der Kreislauf des Lebens

#include "GenesisLifeSimulationTypes.h"

namespace
{
	constexpr uint64 LifeSimulationSalt = 0x11FE51ull;
}

void FGenesisLifeSimulationState::Reset(uint64 WorldSeed)
{
	*this = FGenesisLifeSimulationState();
	Rng = FGenesisRandomStream(WorldSeed).Derive(LifeSimulationSalt);
}

void FGenesisLifeSimulationState::RebuildIndices()
{
	ProfileIndex.Reset();
	TrustByFrom.Reset();
	ReputationBySubject.Reset();

	for (int32 Index = 0; Index < Profiles.Num(); ++Index)
	{
		ProfileIndex.Add(Profiles[Index].EntityId, Index);
	}
	for (int32 Index = 0; Index < TrustEdges.Num(); ++Index)
	{
		TrustByFrom.FindOrAdd(TrustEdges[Index].FromId).Add(Index);
	}
	for (int32 Index = 0; Index < Reputation.Num(); ++Index)
	{
		ReputationBySubject.FindOrAdd(Reputation[Index].SubjectId).Add(Index);
	}
}

FGenesisLifeProfile* FGenesisLifeSimulationState::FindProfile(const FGuid& EntityId)
{
	const int32* Index = ProfileIndex.Find(EntityId);
	return Index ? &Profiles[*Index] : nullptr;
}

const FGenesisLifeProfile* FGenesisLifeSimulationState::FindProfile(const FGuid& EntityId) const
{
	const int32* Index = ProfileIndex.Find(EntityId);
	return Index ? &Profiles[*Index] : nullptr;
}

FGenesisLifeProfile& FGenesisLifeSimulationState::AddOrUpdateProfile(const FGenesisLifeProfile& Profile)
{
	if (const int32* Index = ProfileIndex.Find(Profile.EntityId))
	{
		Profiles[*Index] = Profile;
		return Profiles[*Index];
	}

	const int32 NewIndex = Profiles.Add(Profile);
	ProfileIndex.Add(Profile.EntityId, NewIndex);
	return Profiles[NewIndex];
}

FGenesisTrustEdge* FGenesisLifeSimulationState::FindTrust(const FGuid& FromId, const FGuid& ToId)
{
	if (const TArray<int32>* Indices = TrustByFrom.Find(FromId))
	{
		for (int32 Index : *Indices)
		{
			if (TrustEdges[Index].ToId == ToId)
			{
				return &TrustEdges[Index];
			}
		}
	}
	return nullptr;
}

const FGenesisTrustEdge* FGenesisLifeSimulationState::FindTrust(const FGuid& FromId, const FGuid& ToId) const
{
	if (const TArray<int32>* Indices = TrustByFrom.Find(FromId))
	{
		for (int32 Index : *Indices)
		{
			if (TrustEdges[Index].ToId == ToId)
			{
				return &TrustEdges[Index];
			}
		}
	}
	return nullptr;
}

FGenesisTrustEdge& FGenesisLifeSimulationState::FindOrAddTrust(const FGuid& FromId, const FGuid& ToId)
{
	if (FGenesisTrustEdge* Existing = FindTrust(FromId, ToId))
	{
		return *Existing;
	}

	const int32 NewIndex = TrustEdges.AddDefaulted();
	TrustEdges[NewIndex].FromId = FromId;
	TrustEdges[NewIndex].ToId = ToId;
	TrustByFrom.FindOrAdd(FromId).Add(NewIndex);
	return TrustEdges[NewIndex];
}

float FGenesisLifeSimulationState::GetTrust(const FGuid& FromId, const FGuid& ToId) const
{
	const FGenesisTrustEdge* Edge = FindTrust(FromId, ToId);
	return Edge ? Edge->Trust : 0.0f;
}

float FGenesisLifeSimulationState::GetFamiliarity(const FGuid& FromId, const FGuid& ToId) const
{
	const FGenesisTrustEdge* Edge = FindTrust(FromId, ToId);
	return Edge ? Edge->Familiarity : 0.0f;
}

FGenesisReputationEntry& FGenesisLifeSimulationState::FindOrAddReputation(const FGuid& SubjectId, const FGuid& HolderId, const FGameplayTag& Trait)
{
	if (const TArray<int32>* Indices = ReputationBySubject.Find(SubjectId))
	{
		for (int32 Index : *Indices)
		{
			FGenesisReputationEntry& Entry = Reputation[Index];
			if (Entry.HolderId == HolderId && Entry.Trait == Trait)
			{
				return Entry;
			}
		}
	}

	const int32 NewIndex = Reputation.AddDefaulted();
	Reputation[NewIndex].SubjectId = SubjectId;
	Reputation[NewIndex].HolderId = HolderId;
	Reputation[NewIndex].Trait = Trait;
	ReputationBySubject.FindOrAdd(SubjectId).Add(NewIndex);
	return Reputation[NewIndex];
}

float FGenesisLifeSimulationState::GetReputation(const FGuid& SubjectId, const FGuid& HolderId, const FGameplayTag& Trait) const
{
	if (const TArray<int32>* Indices = ReputationBySubject.Find(SubjectId))
	{
		for (int32 Index : *Indices)
		{
			const FGenesisReputationEntry& Entry = Reputation[Index];
			if (Entry.HolderId == HolderId && Entry.Trait == Trait)
			{
				return Entry.Score;
			}
		}
	}
	return 0.0f;
}
