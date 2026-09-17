// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisPersistence.h"
#include "GenesisBodySubsystem.generated.h"

struct FGenesisSimulationStep;

/** Projekteinstellungen (Project Settings → Genesis → Body). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Body"))
class GENESISBODY_API UGenesisBodySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Tuning")
	FGenesisBodyTuning Tuning;
};

/** Persistenter Zustand aller simulierten Körper. */
USTRUCT()
struct GENESISBODY_API FGenesisBodyRegistry
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGenesisBodyState> Bodies;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnVitalFailure, const FGuid& /*EntityId*/, const FGenesisTimestamp& /*Time*/);

/**
 * Body Simulation der Welt. Persistenz-Ebene: World (Körper Verstorbener bleiben für Geschichte und Ahnen erhalten).
 * Level 1: stündliche Physiologie + tägliche Entwicklung. Level 2: nur täglich.
 */
UCLASS()
class GENESISBODY_API UGenesisBodySubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Body"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Neuer Körper im Moment der Befruchtung (Genom aus GenesisGenetics, Basisparameter aus dem Spermium-Prolog). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void CreateBodyAtConception(const FGuid& EntityId, const FGuid& GenomeId, const FGenesisConceptionVitality& Vitality, EGenesisSimulationLevel Level);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void Birth(const FGuid& EntityId);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void SetActivity(const FGuid& EntityId, EGenesisActivity Activity);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void ApplyInjury(const FGuid& EntityId, FGameplayTag Region, float Severity);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void ApplyAcuteStressor(const FGuid& EntityId, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void ApplyBonding(const FGuid& EntityId, float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	void Eat(const FGuid& EntityId, float Quality);

	/** Was die Person gerade körperlich erlebt – Grundlage für Postprocess, Animation, Audio. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	TArray<FGenesisSymptom> GetSymptoms(const FGuid& EntityId) const;

	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	EGenesisDevelopmentStage GetDevelopmentStage(const FGuid& EntityId) const;

	/** Sinnesschärfe 0..1 (z. B. Sehschärfe → Tiefenunschärfe). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Body")
	float GetSenseAcuity(const FGuid& EntityId, EGenesisBodySense Sense) const;

	const FGenesisBodyState* FindBody(const FGuid& EntityId) const;
	const FGenesisBodyState* GetBodyByIndex(int32 BodyIndex) const { return Registry.Bodies.IsValidIndex(BodyIndex) ? &Registry.Bodies[BodyIndex] : nullptr; }
	int32 GetBodyCount() const { return Registry.Bodies.Num(); }
	FGenesisBodyState* FindBodyMutable(const FGuid& EntityId);

	FGenesisOnVitalFailure OnVitalFailure;

private:
	void HandleSimulationStep(const FGenesisSimulationStep& Step);
	float GetPsychologicalStress(const FGuid& EntityId) const;
	FGenesisTimestamp GetNow() const;
	void RebuildIndex();
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisBodyRegistry Registry;

	TMap<FGuid, int32> Index;
	FDelegateHandle ClockHandle;
};
