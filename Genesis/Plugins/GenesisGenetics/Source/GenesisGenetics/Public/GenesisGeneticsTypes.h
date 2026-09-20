// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "GenesisGeneticsTypes.generated.h"

/** Vererbungsmodus eines Merkmals. */
UENUM(BlueprintType)
enum class EGenesisInheritanceMode : uint8
{
	/** Viele Genorte mit kleinen Effekten (Größe, Talent, Risiko). Normalverteilte Ausprägung. */
	Additive,
	/** Ein Genort, ein dominantes Allel genügt (vereinfachtes Mendel-Modell). */
	Dominant,
	/** Ein Genort, beide Allele müssen vorliegen. */
	Recessive
};

/** Definition eines genetischen Merkmals (Authoring-Daten). */
USTRUCT(BlueprintType)
struct GENESISGENETICS_API FGenesisTraitDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait", meta = (Categories = "Genesis.Trait"))
	FGameplayTag Trait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait")
	EGenesisInheritanceMode Mode = EGenesisInheritanceMode::Additive;

	/** Anzahl der Genorte. Bei Dominant/Recessive wird nur der erste verwendet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trait", meta = (ClampMin = "1", ClampMax = "64"))
	int32 LocusCount = 8;

	/** Mittelwert der Ausprägung in der Grundbevölkerung (normiert). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expression")
	float PhenotypeMean = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expression")
	float PhenotypeStdDev = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expression")
	float PhenotypeMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expression")
	float PhenotypeMax = 1.0f;

	/** Nicht-genetische Streuung (Umwelt in der Entwicklung), pro Individuum fest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Expression", meta = (ClampMin = "0"))
	float EnvironmentalStdDev = 0.03f;

	/** Häufigkeit des "positiven" Allels bei Gründern (nur Dominant/Recessive). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0", ClampMax = "1"))
	float AlleleFrequency = 0.5f;

	/** Mutationswahrscheinlichkeit pro Genort pro Zeugung. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0", ClampMax = "1"))
	float MutationRate = 0.002f;

	/** Epigenetischer Pfad, der dieses Merkmal moduliert (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Epigenetics", meta = (Categories = "Genesis.Epigenetic"))
	FGameplayTag EpigeneticPathway;

	/** Verschiebung der Ausprägung pro Einheit epigenetischer Markierung (−1..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Epigenetics")
	float EpigeneticSensitivity = 0.0f;
};

/** Zwei Allele eines Genorts. Allelwerte −1..1 (Additiv) bzw. >0 = "positives" Allel (Mendel). */
USTRUCT()
struct GENESISGENETICS_API FGenesisAllelePair
{
	GENERATED_BODY()

	UPROPERTY()
	float Maternal = 0.0f;

	UPROPERTY()
	float Paternal = 0.0f;
};

/** Genorte eines Merkmals. */
USTRUCT()
struct GENESISGENETICS_API FGenesisTraitLoci
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Trait;

	UPROPERTY()
	TArray<FGenesisAllelePair> Loci;
};

/** Epigenetische Markierung: Aktivitätsverschiebung eines Pfades (−1 gedämpft … +1 verstärkt). */
USTRUCT()
struct GENESISGENETICS_API FGenesisEpigeneticMark
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Pathway;

	UPROPERTY()
	float Level = 0.0f;
};

/**
 * Biologisches Geschlecht aus den Geschlechtschromosomen.
 *
 * Das ist eine körperliche Angabe – sie steuert Stimmbruch, Hormonverläufe und Körperbau.
 * Über die Person, die daraus wird, sagt sie nichts: Geschlechtsidentität ist ein eigenes Thema
 * und wird hier bewusst nicht mitmodelliert.
 */
UENUM(BlueprintType)
enum class EGenesisBiologicalSex : uint8
{
	Female,
	Male
};

/** Genom eines Individuums. Unabhängig von der Seele. */
USTRUCT()
struct GENESISGENETICS_API FGenesisGenome
{
	GENERATED_BODY()

	/** Aus dem väterlichen Gameten: X ergibt XX, Y ergibt XY. Die Mutter gibt immer ein X weiter. */
	UPROPERTY()
	EGenesisBiologicalSex Sex = EGenesisBiologicalSex::Female;

	UPROPERTY()
	FGuid GenomeId;

	/** Seed für individuelle, nicht-genetische Entwicklungsstreuung. */
	UPROPERTY()
	uint64 IndividualSeed = 0;

	UPROPERTY()
	FGuid MaternalGenomeId;

	UPROPERTY()
	FGuid PaternalGenomeId;

	/** 0 = Gründer. */
	UPROPERTY()
	int32 Generation = 0;

	UPROPERTY()
	TArray<FGenesisTraitLoci> Traits;

	UPROPERTY()
	TArray<FGenesisEpigeneticMark> EpigeneticMarks;

	const FGenesisTraitLoci* FindTrait(const FGameplayTag& Trait) const;
	float GetEpigeneticLevel(const FGameplayTag& Pathway) const;
};

/** Parameter einer Zeugung. */
USTRUCT(BlueprintType)
struct GENESISGENETICS_API FGenesisConceptionParams
{
	GENERATED_BODY()

	/** Anteil elterlicher epigenetischer Markierungen, der an das Kind weitergegeben wird. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conception", meta = (ClampMin = "0", ClampMax = "1"))
	float EpigeneticInheritance = 0.3f;

	/** Multiplikator auf die Mutationsraten aller Merkmale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conception", meta = (ClampMin = "0"))
	float MutationRateMultiplier = 1.0f;
};

/** Gen-Katalog: alle Merkmale einer Welt (Data Asset, Präfix DA_). */
UCLASS(BlueprintType)
class GENESISGENETICS_API UGenesisGeneticsCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genetics", meta = (TitleProperty = "Trait"))
	TArray<FGenesisTraitDefinition> Traits;
};
