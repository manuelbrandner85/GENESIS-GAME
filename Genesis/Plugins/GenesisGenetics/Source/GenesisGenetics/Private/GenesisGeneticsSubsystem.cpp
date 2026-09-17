// GENESIS: Der Kreislauf des Lebens

#include "GenesisGeneticsSubsystem.h"
#include "Engine/GameInstance.h"
#include "GenesisDebug.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisGeneticsSettings.h"
#include "GenesisLog.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Standard-Weltseed, bis GenesisWorld eine Welt mit eigenem Seed anlegt. */
	constexpr uint64 DefaultWorldSeed = 0x47454E45534953ull; // "GENESIS"
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisGeneticsFamilyCommand(
		TEXT("genesis.Genetics.SimulateFamily"),
		TEXT("Entwickler: erzeugt zwei Gründer, belastet die Mutter N Jahre chronisch (Standard 15) und zeugt ein Kind."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UGenesisGeneticsSubsystem* Genetics = GameInstance ? GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
			if (!Genetics)
			{
				return;
			}

			const int32 StressYears = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 15;
			const FGuid Mother = Genetics->CreateFounderGenome();
			const FGuid Father = Genetics->CreateFounderGenome();
			for (int32 Year = 0; Year < StressYears; ++Year)
			{
				Genetics->ApplyEpigeneticExposure(Mother, GenesisGeneticsTags::Epigenetic_StressResponse, 1.0f);
			}
			const FGuid Child = Genetics->ConceiveChild(Mother, Father);
			UE_LOG(LogGenesis, Display, TEXT("Genetik: Mutter %s, Vater %s, Kind %s"),
				*Mother.ToString(EGuidFormats::Short), *Father.ToString(EGuidFormats::Short), *Child.ToString(EGuidFormats::Short));
		}));
}
#endif

void UGenesisGeneticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	LoadCatalog();
	ResetState();

#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisGeneticsSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Genetics"),
		TEXT("Genetics"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisGeneticsSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			OutLines.Add(FString::Printf(TEXT("Genome: %d | Merkmale im Katalog: %d"), Self->Pool.Num(), Self->TraitDefinitions.Num()));

			// Die zuletzt erzeugten Genome (neueste zuerst)
			const TArray<FGenesisGenome>& Genomes = Self->Pool.GetGenomes();
			for (int32 Index = Genomes.Num() - 1; Index >= FMath::Max(0, Genomes.Num() - 3); --Index)
			{
				const FGenesisGenome& Genome = Genomes[Index];
				OutLines.Add(FString::Printf(TEXT("  %s Gen.%d Größe %.2f Musik %.2f Angst %.2f Stress-Epi %.2f"),
					*Genome.GenomeId.ToString(EGuidFormats::Short), Genome.Generation,
					GenesisGeneticsLogic::ExpressTraitByTag(Genome, Self->TraitDefinitions, GenesisGeneticsTags::Trait_Body_Height, -1.0f),
					GenesisGeneticsLogic::ExpressTraitByTag(Genome, Self->TraitDefinitions, GenesisGeneticsTags::Trait_Talent_Musical, -1.0f),
					GenesisGeneticsLogic::ExpressTraitByTag(Genome, Self->TraitDefinitions, GenesisGeneticsTags::Trait_Risk_AnxietySensitivity, -1.0f),
					Genome.GetEpigeneticLevel(GenesisGeneticsTags::Epigenetic_StressResponse)));
			}
		}
	});
#endif
}

void UGenesisGeneticsSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* Registry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			Registry->UnregisterSystem(this);
		}
	}

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Genetics"));
#endif

	Super::Deinitialize();
}

void UGenesisGeneticsSubsystem::LoadCatalog()
{
	const UGenesisGeneticsSettings* Settings = GetDefault<UGenesisGeneticsSettings>();
	if (!Settings->Catalog.IsNull())
	{
		if (const UGenesisGeneticsCatalog* Catalog = Settings->Catalog.LoadSynchronous())
		{
			TraitDefinitions = Catalog->Traits;
			UE_LOG(LogGenesis, Log, TEXT("Genetics: Katalog %s mit %d Merkmalen geladen."), *Catalog->GetName(), TraitDefinitions.Num());
			return;
		}
		UE_LOG(LogGenesis, Warning, TEXT("Genetics: Katalog %s konnte nicht geladen werden – Standardmerkmale aktiv."), *Settings->Catalog.ToString());
	}
	TraitDefinitions = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
}

bool UGenesisGeneticsSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(Pool, OutPayload);
}

bool UGenesisGeneticsSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisGenomePool Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	Pool = MoveTemp(Loaded);
	Pool.RebuildIndex();
	return true;
}

void UGenesisGeneticsSubsystem::ResetState()
{
	Pool.Reset(DefaultWorldSeed);
}

void UGenesisGeneticsSubsystem::ResetWorld(uint64 WorldSeed)
{
	Pool.Reset(WorldSeed);
}

FGuid UGenesisGeneticsSubsystem::CreateFounderGenome()
{
	return Pool.CreateFounder(TraitDefinitions);
}

FGuid UGenesisGeneticsSubsystem::ConceiveChild(const FGuid& MotherGenomeId, const FGuid& FatherGenomeId)
{
	return Pool.Conceive(MotherGenomeId, FatherGenomeId, TraitDefinitions, GetDefault<UGenesisGeneticsSettings>()->Conception);
}

float UGenesisGeneticsSubsystem::ExpressTrait(const FGuid& GenomeId, const FGameplayTag& Trait, float Fallback) const
{
	const FGenesisGenome* Genome = Pool.Find(GenomeId);
	return Genome ? GenesisGeneticsLogic::ExpressTraitByTag(*Genome, TraitDefinitions, Trait, Fallback) : Fallback;
}

void UGenesisGeneticsSubsystem::ApplyEpigeneticExposure(const FGuid& GenomeId, const FGameplayTag& Pathway, float Dose)
{
	if (FGenesisGenome* Genome = Pool.FindMutable(GenomeId))
	{
		GenesisGeneticsLogic::ApplyEpigeneticExposure(*Genome, Pathway, Dose, GetDefault<UGenesisGeneticsSettings>()->EpigeneticPlasticity);
	}
}

void UGenesisGeneticsSubsystem::AdvanceEpigenetics(const FGuid& GenomeId, double ElapsedYears)
{
	if (FGenesisGenome* Genome = Pool.FindMutable(GenomeId))
	{
		GenesisGeneticsLogic::RevertEpigeneticMarks(*Genome, ElapsedYears, GetDefault<UGenesisGeneticsSettings>()->EpigeneticReversionPerYear);
	}
}
