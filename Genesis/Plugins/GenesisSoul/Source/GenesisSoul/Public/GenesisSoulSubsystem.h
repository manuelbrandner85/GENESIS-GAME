// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisPersistence.h"
#include "GenesisSoulTypes.h"
#include "GenesisSoulSubsystem.generated.h"

/** Persistenter Zustand der Soul-Ebene. */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulArchive
{
	GENERATED_BODY()

	/** Die Seele des Spielers. */
	UPROPERTY()
	FGenesisSoulSeed PlayerSoul;

	/**
	 * Seelen bedeutsamer NPCs, die über Leben hinweg wiederkehren können (Seelen-Begegnungen).
	 * Später auch Seelen realer Mitspieler (GenesisMultiplayer).
	 */
	UPROPERTY()
	TArray<FGenesisSoulSeed> CompanionSouls;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnIncarnationChanged, const FGenesisIncarnationRecord& /*Record*/);

/**
 * Soul Engine – verwaltet Seelen über alle Leben.
 * Persistenz-Ebene: Soul (überdauert Welten und Inkarnationen).
 */
UCLASS()
class GENESISSOUL_API UGenesisSoulSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Soul"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::Soul; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Legt die Spielerseele neu an (neues Profil). Überschreibt eine bestehende Seele nicht. */
	bool CreatePlayerSoul(uint64 OriginSeed);

	bool HasPlayerSoul() const { return Archive.PlayerSoul.IsValid(); }
	const FGenesisSoulSeed& GetPlayerSoul() const { return Archive.PlayerSoul; }

	/** Beginnt eine Inkarnation der Spielerseele. */
	bool BeginIncarnation(const FGenesisIncarnationRecord& Template);

	/** Schließt die aktuelle Inkarnation der Spielerseele ab (nach dem Karma-Gericht). */
	bool CloseIncarnation(const FGenesisLifeClosure& Closure);

	/** Ein Muster wurde erlebt (z. B. Faszination beim ersten Blick aufs Meer). */
	void ReinforcePlayerResonance(const FGameplayTag& Pattern, float Amount);

	float GetPlayerResonance(const FGameplayTag& PatternQuery) const;
	float GetPlayerEchoPull(const FGameplayTagContainer& SituationThemes) const;
	float GetPlayerRecognitionOf(const FGuid& OtherSoulId, const FGameplayTagContainer& ContextThemes) const;

	/** Erzeugt (oder liefert) eine Begleiterseele aus einem deterministischen Seed. */
	const FGenesisSoulSeed& GetOrCreateCompanionSoul(uint64 OriginSeed);

	const FGenesisSoulSeed* FindSoul(const FGuid& SoulId) const;

	FGenesisOnIncarnationChanged OnIncarnationBegan;
	FGenesisOnIncarnationChanged OnIncarnationClosed;

private:
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisSoulArchive Archive;
};
