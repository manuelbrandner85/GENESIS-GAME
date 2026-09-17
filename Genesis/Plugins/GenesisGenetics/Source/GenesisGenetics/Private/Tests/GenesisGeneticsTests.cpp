// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisGenomePool.h"
#include "GenesisPersistence.h"
#include "GenesisRandom.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisGeneticsTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGenesisTraitDefinition* FindDefinition(const TArray<FGenesisTraitDefinition>& Traits, const FGameplayTag& Tag)
	{
		return Traits.FindByPredicate([&Tag](const FGenesisTraitDefinition& Definition) { return Definition.Trait == Tag; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisGeneticsDeterminismTest, "Genesis.Genetics.DNA.Determinism", GenesisGeneticsTests::Flags)
bool FGenesisGeneticsDeterminismTest::RunTest(const FString& Parameters)
{
	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();

	FGenesisRandomStream RngA(123);
	FGenesisRandomStream RngB(123);
	const FGenesisGenome A = GenesisGeneticsLogic::CreateFounderGenome(Traits, RngA);
	const FGenesisGenome B = GenesisGeneticsLogic::CreateFounderGenome(Traits, RngB);

	TestEqual(TEXT("Gleiche GenomeId"), A.GenomeId, B.GenomeId);
	TestEqual(TEXT("Alle Merkmale angelegt"), A.Traits.Num(), Traits.Num());

	const FGameplayTag Height = GenesisGeneticsTags::Trait_Body_Height;
	TestEqual(TEXT("Gleiche Ausprägung"),
		GenesisGeneticsLogic::ExpressTraitByTag(A, Traits, Height, -1.0f),
		GenesisGeneticsLogic::ExpressTraitByTag(B, Traits, Height, -1.0f));

	const FGenesisTraitDefinition* LightEyes = GenesisGeneticsTests::FindDefinition(Traits, GenesisGeneticsTags::Trait_Appearance_LightEyes);
	if (TestNotNull(TEXT("Rezessives Merkmal vorhanden"), LightEyes))
	{
		TestEqual(TEXT("Mendel: ein Genort"), A.FindTrait(LightEyes->Trait)->Loci.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisGeneticsInheritanceTest, "Genesis.Genetics.DNA.Inheritance", GenesisGeneticsTests::Flags)
bool FGenesisGeneticsInheritanceTest::RunTest(const FString& Parameters)
{
	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
	FGenesisConceptionParams Params;
	Params.MutationRateMultiplier = 0.0f; // für eindeutige Herkunftsprüfung

	FGenesisRandomStream Rng(99);
	const FGenesisGenome Mother = GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng);
	const FGenesisGenome Father = GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng);
	const FGenesisGenome Child = GenesisGeneticsLogic::Conceive(Mother, Father, Traits, Rng, Params);

	TestEqual(TEXT("Mutter verknüpft"), Child.MaternalGenomeId, Mother.GenomeId);
	TestEqual(TEXT("Vater verknüpft"), Child.PaternalGenomeId, Father.GenomeId);
	TestEqual(TEXT("Generation"), Child.Generation, 1);

	// Jedes mütterliche Allel des Kindes stammt aus dem Allelpaar der Mutter (ohne Mutation)
	bool bAllInherited = true;
	for (const FGenesisTraitLoci& ChildLoci : Child.Traits)
	{
		const FGenesisTraitLoci* MotherLoci = Mother.FindTrait(ChildLoci.Trait);
		const FGenesisTraitLoci* FatherLoci = Father.FindTrait(ChildLoci.Trait);
		for (int32 Index = 0; Index < ChildLoci.Loci.Num(); ++Index)
		{
			const FGenesisAllelePair& MotherPair = MotherLoci->Loci[Index];
			const FGenesisAllelePair& FatherPair = FatherLoci->Loci[Index];
			const float Maternal = ChildLoci.Loci[Index].Maternal;
			const float Paternal = ChildLoci.Loci[Index].Paternal;
			bAllInherited &= (Maternal == MotherPair.Maternal || Maternal == MotherPair.Paternal);
			bAllInherited &= (Paternal == FatherPair.Maternal || Paternal == FatherPair.Paternal);
		}
	}
	TestTrue(TEXT("Alle Allele stammen von den Eltern"), bAllInherited);

	// Rezessiv: zwei homozygot positive Eltern → Kind exprimiert immer
	const FGenesisTraitDefinition* LightEyes = GenesisGeneticsTests::FindDefinition(Traits, GenesisGeneticsTags::Trait_Appearance_LightEyes);
	FGenesisGenome HomoMother = Mother;
	FGenesisGenome HomoFather = Father;
	for (FGenesisGenome* Parent : { &HomoMother, &HomoFather })
	{
		for (FGenesisTraitLoci& Loci : Parent->Traits)
		{
			if (Loci.Trait == LightEyes->Trait)
			{
				Loci.Loci[0].Maternal = 1.0f;
				Loci.Loci[0].Paternal = 1.0f;
			}
		}
	}
	const FGenesisGenome RecessiveChild = GenesisGeneticsLogic::Conceive(HomoMother, HomoFather, Traits, Rng, Params);
	TestEqual(TEXT("Rezessives Merkmal ausgeprägt"), GenesisGeneticsLogic::ExpressTrait(RecessiveChild, *LightEyes), LightEyes->PhenotypeMax);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisGeneticsPopulationTest, "Genesis.Genetics.DNA.PopulationDistribution", GenesisGeneticsTests::Flags)
bool FGenesisGeneticsPopulationTest::RunTest(const FString& Parameters)
{
	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
	const FGenesisTraitDefinition* Height = GenesisGeneticsTests::FindDefinition(Traits, GenesisGeneticsTags::Trait_Body_Height);

	FGenesisRandomStream Rng(7);
	const int32 PopulationSize = 2000;
	double Sum = 0.0;
	double SquareSum = 0.0;
	for (int32 Index = 0; Index < PopulationSize; ++Index)
	{
		const float Value = GenesisGeneticsLogic::ExpressTrait(GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng), *Height);
		Sum += Value;
		SquareSum += Value * Value;
	}
	const double Mean = Sum / PopulationSize;
	const double StdDev = FMath::Sqrt(SquareSum / PopulationSize - Mean * Mean);

	TestTrue(FString::Printf(TEXT("Mittelwert ~%.2f (ist %.3f)"), Height->PhenotypeMean, Mean), FMath::Abs(Mean - Height->PhenotypeMean) < 0.02);
	TestTrue(FString::Printf(TEXT("Streuung ~%.2f (ist %.3f)"), Height->PhenotypeStdDev, StdDev), FMath::Abs(StdDev - Height->PhenotypeStdDev) < 0.03);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEpigeneticsTest, "Genesis.Genetics.Epigenetics", GenesisGeneticsTests::Flags)
bool FGenesisEpigeneticsTest::RunTest(const FString& Parameters)
{
	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
	const FGenesisTraitDefinition* Anxiety = GenesisGeneticsTests::FindDefinition(Traits, GenesisGeneticsTags::Trait_Risk_AnxietySensitivity);
	const FGameplayTag Stress = GenesisGeneticsTags::Epigenetic_StressResponse;

	FGenesisRandomStream Rng(31);
	FGenesisGenome Parent = GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng);
	const FGenesisGenome OtherParent = GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng);
	const float Before = GenesisGeneticsLogic::ExpressTrait(Parent, *Anxiety);

	// Chronische Belastung über viele Jahre
	for (int32 Year = 0; Year < 20; ++Year)
	{
		GenesisGeneticsLogic::ApplyEpigeneticExposure(Parent, Stress, 1.0f, 0.1f);
	}
	const float Level = Parent.GetEpigeneticLevel(Stress);
	TestTrue(TEXT("Markierung aufgebaut"), Level > 0.5f && Level <= 1.0f);
	TestTrue(TEXT("Angstempfindlichkeit erhöht"), GenesisGeneticsLogic::ExpressTrait(Parent, *Anxiety) > Before);

	// Transgenerational: abgeschwächte Weitergabe an das Kind
	FGenesisConceptionParams Params;
	const FGenesisGenome Child = GenesisGeneticsLogic::Conceive(Parent, OtherParent, Traits, Rng, Params);
	const float ChildLevel = Child.GetEpigeneticLevel(Stress);
	TestTrue(TEXT("Kind erbt abgeschwächt"), ChildLevel > 0.0f && ChildLevel < Level);
	TestTrue(TEXT("Erwarteter Anteil"), FMath::IsNearlyEqual(ChildLevel, 0.5f * Level * Params.EpigeneticInheritance, 1.0e-4f));

	// Rückbildung ohne Exposition
	GenesisGeneticsLogic::RevertEpigeneticMarks(Parent, 30.0, 0.1f);
	TestTrue(TEXT("Rückbildung"), Parent.GetEpigeneticLevel(Stress) < Level * 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisGenomePoolTest, "Genesis.Genetics.PoolAndPersistence", GenesisGeneticsTests::Flags)
bool FGenesisGenomePoolTest::RunTest(const FString& Parameters)
{
	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();

	FGenesisGenomePool Pool;
	Pool.Reset(2026);
	const FGuid Mother = Pool.CreateFounder(Traits);
	const FGuid Father = Pool.CreateFounder(Traits);
	const FGuid Child = Pool.Conceive(Mother, Father, Traits, FGenesisConceptionParams());
	TestTrue(TEXT("Kind erzeugt"), Child.IsValid());
	TestFalse(TEXT("Fehlender Elternteil"), Pool.Conceive(Mother, FGuid(9, 9, 9, 9), Traits, FGenesisConceptionParams()).IsValid());
	TestEqual(TEXT("Drei Genome"), Pool.Num(), 3);

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben"), GenesisPersistence::Write(Pool, Bytes));
	FGenesisGenomePool Loaded;
	TestTrue(TEXT("Lesen"), GenesisPersistence::Read(Loaded, Bytes));
	Loaded.RebuildIndex();

	const FGenesisGenome* LoadedChild = Loaded.Find(Child);
	if (!TestNotNull(TEXT("Kind nach Laden auffindbar"), LoadedChild))
	{
		return false;
	}
	TestEqual(TEXT("Elternlinie erhalten"), LoadedChild->MaternalGenomeId, Mother);

	// Zufallsstrom setzt nach dem Laden identisch fort
	const FGuid NextOriginal = Pool.CreateFounder(Traits);
	const FGuid NextLoaded = Loaded.CreateFounder(Traits);
	TestEqual(TEXT("Deterministische Fortsetzung"), NextLoaded, NextOriginal);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
