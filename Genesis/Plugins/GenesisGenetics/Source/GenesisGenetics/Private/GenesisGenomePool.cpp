// GENESIS: Der Kreislauf des Lebens

#include "GenesisGenomePool.h"
#include "GenesisGeneticsLogic.h"

namespace
{
	constexpr uint64 GenomePoolSalt = 0x6E0E5ull;
}

void FGenesisGenomePool::Reset(uint64 WorldSeed)
{
	Genomes.Reset();
	Index.Reset();
	Rng = FGenesisRandomStream(WorldSeed).Derive(GenomePoolSalt);
}

void FGenesisGenomePool::RebuildIndex()
{
	Index.Reset();
	Index.Reserve(Genomes.Num());
	for (int32 GenomeIndex = 0; GenomeIndex < Genomes.Num(); ++GenomeIndex)
	{
		Index.Add(Genomes[GenomeIndex].GenomeId, GenomeIndex);
	}
}

const FGenesisGenome* FGenesisGenomePool::Find(const FGuid& GenomeId) const
{
	if (const int32* Found = Index.Find(GenomeId))
	{
		return &Genomes[*Found];
	}
	return nullptr;
}

FGenesisGenome* FGenesisGenomePool::FindMutable(const FGuid& GenomeId)
{
	if (const int32* Found = Index.Find(GenomeId))
	{
		return &Genomes[*Found];
	}
	return nullptr;
}

const FGenesisGenome& FGenesisGenomePool::Add(const FGenesisGenome& Genome)
{
	if (const int32* Found = Index.Find(Genome.GenomeId))
	{
		Genomes[*Found] = Genome;
		return Genomes[*Found];
	}

	const int32 NewIndex = Genomes.Add(Genome);
	Index.Add(Genome.GenomeId, NewIndex);
	return Genomes[NewIndex];
}

FGuid FGenesisGenomePool::CreateFounder(const TArray<FGenesisTraitDefinition>& Traits)
{
	return Add(GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng)).GenomeId;
}

FGuid FGenesisGenomePool::Conceive(const FGuid& MotherId, const FGuid& FatherId, const TArray<FGenesisTraitDefinition>& Traits, const FGenesisConceptionParams& Params)
{
	const FGenesisGenome* Mother = Find(MotherId);
	const FGenesisGenome* Father = Find(FatherId);
	if (!Mother || !Father)
	{
		return FGuid();
	}

	// Kopie vor Add: Add kann das Array umlagern und Eltern-Zeiger ungültig machen
	const FGenesisGenome Child = GenesisGeneticsLogic::Conceive(*Mother, *Father, Traits, Rng, Params);
	return Add(Child).GenomeId;
}
