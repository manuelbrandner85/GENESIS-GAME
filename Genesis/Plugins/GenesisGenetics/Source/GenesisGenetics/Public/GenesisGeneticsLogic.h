// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisGeneticsTypes.h"

struct FGenesisRandomStream;

/**
 * Zustandslose Genetik-Logik. Deterministisch über den übergebenen Zufallsstrom.
 * Epigenetik bildet Lebensstil-Einflüsse ab – ohne Wertung: Ein Pfad wird verstärkt oder gedämpft, nicht "verbessert".
 */
namespace GenesisGeneticsLogic
{
	/** Merkmale, die C++-Systeme direkt verwenden. Grundlage, wenn kein Katalog-Asset gesetzt ist. */
	GENESISGENETICS_API TArray<FGenesisTraitDefinition> MakeDefaultTraitDefinitions();

	/** Genom ohne Eltern (Gründerbevölkerung). */
	GENESISGENETICS_API FGenesisGenome CreateFounderGenome(const TArray<FGenesisTraitDefinition>& Traits, FGenesisRandomStream& Rng);

	/** Befruchtung: zufällige Allel-Auswahl je Genort, Mutation, epigenetische Weitergabe. */
	GENESISGENETICS_API FGenesisGenome Conceive(const FGenesisGenome& Mother, const FGenesisGenome& Father, const TArray<FGenesisTraitDefinition>& Traits,
		FGenesisRandomStream& Rng, const FGenesisConceptionParams& Params);

	/** Phänotypische Ausprägung eines Merkmals (inklusive Umweltstreuung und Epigenetik). */
	GENESISGENETICS_API float ExpressTrait(const FGenesisGenome& Genome, const FGenesisTraitDefinition& Definition);

	/** Wie ExpressTrait, sucht die Definition per Tag. Fallback, wenn Merkmal unbekannt. */
	GENESISGENETICS_API float ExpressTraitByTag(const FGenesisGenome& Genome, const TArray<FGenesisTraitDefinition>& Traits, const FGameplayTag& Trait, float Fallback);

	/** Lebensstil-Exposition verschiebt die Markierung sättigend (Dose −1..1 typisch, Plasticity 0..1). */
	GENESISGENETICS_API void ApplyEpigeneticExposure(FGenesisGenome& Genome, const FGameplayTag& Pathway, float Dose, float Plasticity);

	/** Markierungen bilden sich ohne Exposition langsam zurück. */
	GENESISGENETICS_API void RevertEpigeneticMarks(FGenesisGenome& Genome, double ElapsedYears, float ReversionPerYear);
}
