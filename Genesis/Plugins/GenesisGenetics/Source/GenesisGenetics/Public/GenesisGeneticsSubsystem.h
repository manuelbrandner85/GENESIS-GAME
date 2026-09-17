// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisGenomePool.h"
#include "GenesisPersistence.h"
#include "GenesisGeneticsSubsystem.generated.h"

/**
 * Genetik der Welt: Genom-Pool, Merkmalskatalog, Zeugung.
 * Persistenz-Ebene: World (Genome überdauern ihre Träger – Ahnen, Familienlinien).
 */
UCLASS()
class GENESISGENETICS_API UGenesisGeneticsSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Genetics"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Neue Welt: leert den Pool und setzt den Seed. */
	void ResetWorld(uint64 WorldSeed);

	const TArray<FGenesisTraitDefinition>& GetTraitDefinitions() const { return TraitDefinitions; }

	FGuid CreateFounderGenome();
	FGuid ConceiveChild(const FGuid& MotherGenomeId, const FGuid& FatherGenomeId);

	const FGenesisGenome* FindGenome(const FGuid& GenomeId) const { return Pool.Find(GenomeId); }

	/** Ausprägung eines Merkmals; Fallback bei unbekanntem Genom oder Merkmal. */
	float ExpressTrait(const FGuid& GenomeId, const FGameplayTag& Trait, float Fallback = 0.5f) const;

	/** Lebensstil-Einfluss auf einen epigenetischen Pfad. */
	void ApplyEpigeneticExposure(const FGuid& GenomeId, const FGameplayTag& Pathway, float Dose);

	/** Natürliche Rückbildung aller Markierungen eines Genoms. */
	void AdvanceEpigenetics(const FGuid& GenomeId, double ElapsedYears);

	FGenesisGenomePool& GetPool() { return Pool; }

private:
	void LoadCatalog();

	UPROPERTY()
	FGenesisGenomePool Pool;

	UPROPERTY(Transient)
	TArray<FGenesisTraitDefinition> TraitDefinitions;
};
