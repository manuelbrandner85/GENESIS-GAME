// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisGeneticsTypes.h"
#include "GenesisRandom.h"
#include "GenesisGenomePool.generated.h"

/**
 * Alle Genome einer Welt – auch die verstorbener Personen (Ahnenanalyse, Familienlinien).
 * Reiner Zustand + Logik, ohne Subsystem testbar.
 */
USTRUCT()
struct GENESISGENETICS_API FGenesisGenomePool
{
	GENERATED_BODY()

	/** Setzt den Pool leer zurück und initialisiert den Zufallsstrom. */
	void Reset(uint64 WorldSeed);

	/** Muss nach dem Laden aufgerufen werden. */
	void RebuildIndex();

	const FGenesisGenome* Find(const FGuid& GenomeId) const;
	FGenesisGenome* FindMutable(const FGuid& GenomeId);

	/** Fügt ein Genom ein (ersetzt bei gleicher ID). */
	const FGenesisGenome& Add(const FGenesisGenome& Genome);

	/** Erzeugt ein Gründergenom und nimmt es auf. */
	FGuid CreateFounder(const TArray<FGenesisTraitDefinition>& Traits);

	/** Zeugt ein Kind aus zwei vorhandenen Genomen. Ungültige GUID, wenn ein Elternteil fehlt. */
	FGuid Conceive(const FGuid& MotherId, const FGuid& FatherId, const TArray<FGenesisTraitDefinition>& Traits, const FGenesisConceptionParams& Params);

	int32 Num() const { return Genomes.Num(); }
	const TArray<FGenesisGenome>& GetGenomes() const { return Genomes; }

private:
	UPROPERTY()
	TArray<FGenesisGenome> Genomes;

	UPROPERTY()
	FGenesisRandomStream Rng;

	/** Laufzeit-Index (nicht gespeichert). */
	TMap<FGuid, int32> Index;
};
