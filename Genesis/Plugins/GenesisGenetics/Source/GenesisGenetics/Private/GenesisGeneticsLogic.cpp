// GENESIS: Der Kreislauf des Lebens

#include "GenesisGeneticsLogic.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisMath.h"
#include "GenesisRandom.h"

const FGenesisTraitLoci* FGenesisGenome::FindTrait(const FGameplayTag& Trait) const
{
	return Traits.FindByPredicate([&Trait](const FGenesisTraitLoci& Loci) { return Loci.Trait == Trait; });
}

float FGenesisGenome::GetEpigeneticLevel(const FGameplayTag& Pathway) const
{
	for (const FGenesisEpigeneticMark& Mark : EpigeneticMarks)
	{
		if (Mark.Pathway == Pathway)
		{
			return Mark.Level;
		}
	}
	return 0.0f;
}

namespace GenesisGeneticsLogic
{
	namespace
	{
		/** Epigenetische Markierungen unter diesem Betrag gelten als neutral und werden entfernt. */
		constexpr float NegligibleMark = 0.005f;

		/** Standardabweichung einer Additiv-Mutation. */
		constexpr float MutationStdDev = 0.3f;

		bool IsMendelian(const FGenesisTraitDefinition& Definition)
		{
			return Definition.Mode != EGenesisInheritanceMode::Additive;
		}

		int32 GetEffectiveLocusCount(const FGenesisTraitDefinition& Definition)
		{
			return IsMendelian(Definition) ? 1 : FMath::Clamp(Definition.LocusCount, 1, 64);
		}

		float RandomFounderAllele(const FGenesisTraitDefinition& Definition, FGenesisRandomStream& Rng)
		{
			if (IsMendelian(Definition))
			{
				return Rng.Bernoulli(Definition.AlleleFrequency) ? 1.0f : -1.0f;
			}
			return Rng.FRandRange(-1.0f, 1.0f);
		}

		float Mutate(float Allele, const FGenesisTraitDefinition& Definition, FGenesisRandomStream& Rng)
		{
			if (IsMendelian(Definition))
			{
				return -Allele;
			}
			return FMath::Clamp(Allele + Rng.Gaussian(0.0f, MutationStdDev), -1.0f, 1.0f);
		}

		/** Gamete: eines der beiden Allele eines Elternteils (unabhängige Verteilung je Genort). */
		float PickGameteAllele(const FGenesisTraitLoci* ParentLoci, int32 LocusIndex, const FGenesisTraitDefinition& Definition, FGenesisRandomStream& Rng)
		{
			if (ParentLoci && ParentLoci->Loci.IsValidIndex(LocusIndex))
			{
				const FGenesisAllelePair& Pair = ParentLoci->Loci[LocusIndex];
				return Rng.Bernoulli(0.5f) ? Pair.Maternal : Pair.Paternal;
			}
			// Merkmal ist neuer als das Elterngenom (Katalog erweitert) → Allel aus der Grundbevölkerung
			return RandomFounderAllele(Definition, Rng);
		}

		FGenesisTraitDefinition MakeDefinition(const FGameplayTag& Trait, EGenesisInheritanceMode Mode, int32 Loci, float Mean, float StdDev,
			const FGameplayTag& Pathway = FGameplayTag(), float Sensitivity = 0.0f)
		{
			FGenesisTraitDefinition Definition;
			Definition.Trait = Trait;
			Definition.Mode = Mode;
			Definition.LocusCount = Loci;
			Definition.PhenotypeMean = Mean;
			Definition.PhenotypeStdDev = StdDev;
			Definition.EpigeneticPathway = Pathway;
			Definition.EpigeneticSensitivity = Sensitivity;
			return Definition;
		}
	}

	TArray<FGenesisTraitDefinition> MakeDefaultTraitDefinitions()
	{
		using namespace GenesisGeneticsTags;
		const EGenesisInheritanceMode Additive = EGenesisInheritanceMode::Additive;

		TArray<FGenesisTraitDefinition> Traits;
		Traits.Add(MakeDefinition(Trait_Body_Height, Additive, 16, 0.5f, 0.12f));
		Traits.Add(MakeDefinition(Trait_Body_MuscleMass, Additive, 12, 0.5f, 0.12f, Epigenetic_PhysicalConditioning, 0.08f));
		Traits.Add(MakeDefinition(Trait_Body_Metabolism, Additive, 10, 0.5f, 0.1f, Epigenetic_Metabolism, 0.1f));
		Traits.Add(MakeDefinition(Trait_Appearance_Pigmentation, Additive, 12, 0.5f, 0.2f));
		Traits.Add(MakeDefinition(Trait_Risk_Cardiovascular, Additive, 10, 0.3f, 0.1f, Epigenetic_Metabolism, 0.08f));
		Traits.Add(MakeDefinition(Trait_Risk_AnxietySensitivity, Additive, 10, 0.4f, 0.12f, Epigenetic_StressResponse, 0.12f));
		Traits.Add(MakeDefinition(Trait_Talent_Musical, Additive, 12, 0.4f, 0.15f));
		Traits.Add(MakeDefinition(Trait_Talent_Spatial, Additive, 12, 0.4f, 0.15f));
		Traits.Add(MakeDefinition(Trait_Sense_SmellAcuity, Additive, 8, 0.5f, 0.12f));

		FGenesisTraitDefinition LightEyes = MakeDefinition(Trait_Appearance_LightEyes, EGenesisInheritanceMode::Recessive, 1, 0.0f, 0.0f);
		LightEyes.AlleleFrequency = 0.4f;
		Traits.Add(LightEyes);

		return Traits;
	}

	FGenesisGenome CreateFounderGenome(const TArray<FGenesisTraitDefinition>& Traits, FGenesisRandomStream& Rng)
	{
		FGenesisGenome Genome;
		Genome.GenomeId = Rng.NewGuid();
		Genome.IndividualSeed = Rng.NextUInt64();
		Genome.Generation = 0;

		Genome.Traits.Reserve(Traits.Num());
		for (const FGenesisTraitDefinition& Definition : Traits)
		{
			if (!Definition.Trait.IsValid())
			{
				continue;
			}

			FGenesisTraitLoci& Loci = Genome.Traits.AddDefaulted_GetRef();
			Loci.Trait = Definition.Trait;

			const int32 Count = GetEffectiveLocusCount(Definition);
			Loci.Loci.SetNum(Count);
			for (FGenesisAllelePair& Pair : Loci.Loci)
			{
				Pair.Maternal = RandomFounderAllele(Definition, Rng);
				Pair.Paternal = RandomFounderAllele(Definition, Rng);
			}
		}
		return Genome;
	}

	FGenesisGenome Conceive(const FGenesisGenome& Mother, const FGenesisGenome& Father, const TArray<FGenesisTraitDefinition>& Traits,
		FGenesisRandomStream& Rng, const FGenesisConceptionParams& Params)
	{
		FGenesisGenome Child;
		Child.GenomeId = Rng.NewGuid();
		Child.IndividualSeed = Rng.NextUInt64();
		Child.MaternalGenomeId = Mother.GenomeId;
		Child.PaternalGenomeId = Father.GenomeId;
		Child.Generation = FMath::Max(Mother.Generation, Father.Generation) + 1;

		Child.Traits.Reserve(Traits.Num());
		for (const FGenesisTraitDefinition& Definition : Traits)
		{
			if (!Definition.Trait.IsValid())
			{
				continue;
			}

			const FGenesisTraitLoci* MotherLoci = Mother.FindTrait(Definition.Trait);
			const FGenesisTraitLoci* FatherLoci = Father.FindTrait(Definition.Trait);
			const float MutationChance = FMath::Clamp(Definition.MutationRate * Params.MutationRateMultiplier, 0.0f, 1.0f);

			FGenesisTraitLoci& Loci = Child.Traits.AddDefaulted_GetRef();
			Loci.Trait = Definition.Trait;

			const int32 Count = GetEffectiveLocusCount(Definition);
			Loci.Loci.SetNum(Count);
			for (int32 LocusIndex = 0; LocusIndex < Count; ++LocusIndex)
			{
				FGenesisAllelePair& Pair = Loci.Loci[LocusIndex];
				Pair.Maternal = PickGameteAllele(MotherLoci, LocusIndex, Definition, Rng);
				Pair.Paternal = PickGameteAllele(FatherLoci, LocusIndex, Definition, Rng);

				if (Rng.Bernoulli(MutationChance))
				{
					Pair.Maternal = Mutate(Pair.Maternal, Definition, Rng);
				}
				if (Rng.Bernoulli(MutationChance))
				{
					Pair.Paternal = Mutate(Pair.Paternal, Definition, Rng);
				}
			}
		}

		// Transgenerationale Epigenetik: abgeschwächter Mittelwert beider Eltern
		TArray<FGameplayTag> Pathways;
		for (const FGenesisEpigeneticMark& Mark : Mother.EpigeneticMarks)
		{
			Pathways.AddUnique(Mark.Pathway);
		}
		for (const FGenesisEpigeneticMark& Mark : Father.EpigeneticMarks)
		{
			Pathways.AddUnique(Mark.Pathway);
		}
		for (const FGameplayTag& Pathway : Pathways)
		{
			const float Inherited = 0.5f * (Mother.GetEpigeneticLevel(Pathway) + Father.GetEpigeneticLevel(Pathway)) * Params.EpigeneticInheritance;
			if (FMath::Abs(Inherited) >= NegligibleMark)
			{
				FGenesisEpigeneticMark& Mark = Child.EpigeneticMarks.AddDefaulted_GetRef();
				Mark.Pathway = Pathway;
				Mark.Level = FMath::Clamp(Inherited, -1.0f, 1.0f);
			}
		}

		return Child;
	}

	float ExpressTrait(const FGenesisGenome& Genome, const FGenesisTraitDefinition& Definition)
	{
		const FGenesisTraitLoci* Loci = Genome.FindTrait(Definition.Trait);
		if (!Loci || Loci->Loci.Num() == 0)
		{
			return FMath::Clamp(Definition.PhenotypeMean, Definition.PhenotypeMin, Definition.PhenotypeMax);
		}

		const float EpigeneticShift = Definition.EpigeneticPathway.IsValid()
			? Genome.GetEpigeneticLevel(Definition.EpigeneticPathway) * Definition.EpigeneticSensitivity
			: 0.0f;

		switch (Definition.Mode)
		{
		case EGenesisInheritanceMode::Dominant:
		{
			const FGenesisAllelePair& Pair = Loci->Loci[0];
			const bool bExpressed = Pair.Maternal > 0.0f || Pair.Paternal > 0.0f;
			return bExpressed ? Definition.PhenotypeMax : Definition.PhenotypeMin;
		}
		case EGenesisInheritanceMode::Recessive:
		{
			const FGenesisAllelePair& Pair = Loci->Loci[0];
			const bool bExpressed = Pair.Maternal > 0.0f && Pair.Paternal > 0.0f;
			return bExpressed ? Definition.PhenotypeMax : Definition.PhenotypeMin;
		}
		case EGenesisInheritanceMode::Additive:
		default:
		{
			double Sum = 0.0;
			for (const FGenesisAllelePair& Pair : Loci->Loci)
			{
				Sum += Pair.Maternal + Pair.Paternal;
			}

			// Summe gleichverteilter Allele (−1..1, Varianz 1/3) → auf Standardnormal normieren
			const double AlleleCount = 2.0 * static_cast<double>(Loci->Loci.Num());
			const double ZScore = Sum / FMath::Sqrt(AlleleCount / 3.0);

			// Entwicklungsstreuung: pro Individuum und Merkmal fest
			FGenesisRandomStream EnvironmentRng(GenesisHash::Combine(Genome.IndividualSeed, GenesisHash::FromName(Definition.Trait.GetTagName())));
			const float Environment = EnvironmentRng.Gaussian(0.0f, Definition.EnvironmentalStdDev);

			const float Value = Definition.PhenotypeMean + Definition.PhenotypeStdDev * static_cast<float>(ZScore) + Environment + EpigeneticShift;
			return FMath::Clamp(Value, Definition.PhenotypeMin, Definition.PhenotypeMax);
		}
		}
	}

	float ExpressTraitByTag(const FGenesisGenome& Genome, const TArray<FGenesisTraitDefinition>& Traits, const FGameplayTag& Trait, float Fallback)
	{
		for (const FGenesisTraitDefinition& Definition : Traits)
		{
			if (Definition.Trait == Trait)
			{
				return ExpressTrait(Genome, Definition);
			}
		}
		return Fallback;
	}

	void ApplyEpigeneticExposure(FGenesisGenome& Genome, const FGameplayTag& Pathway, float Dose, float Plasticity)
	{
		if (!Pathway.IsValid() || FMath::IsNearlyZero(Dose) || Plasticity <= 0.0f)
		{
			return;
		}

		FGenesisEpigeneticMark* Mark = Genome.EpigeneticMarks.FindByPredicate([&Pathway](const FGenesisEpigeneticMark& Existing)
		{
			return Existing.Pathway == Pathway;
		});
		if (!Mark)
		{
			Mark = &Genome.EpigeneticMarks.AddDefaulted_GetRef();
			Mark->Pathway = Pathway;
		}

		Mark->Level = GenesisMath::ApplySaturatingDelta(Mark->Level, Dose * Plasticity, -1.0f, 1.0f, 1.0f);
	}

	void RevertEpigeneticMarks(FGenesisGenome& Genome, double ElapsedYears, float ReversionPerYear)
	{
		if (ElapsedYears <= 0.0)
		{
			return;
		}

		for (FGenesisEpigeneticMark& Mark : Genome.EpigeneticMarks)
		{
			Mark.Level = GenesisMath::ApproachExponential(Mark.Level, 0.0f, ReversionPerYear, ElapsedYears);
		}
		Genome.EpigeneticMarks.RemoveAll([](const FGenesisEpigeneticMark& Mark)
		{
			return FMath::Abs(Mark.Level) < NegligibleMark;
		});
	}
}
