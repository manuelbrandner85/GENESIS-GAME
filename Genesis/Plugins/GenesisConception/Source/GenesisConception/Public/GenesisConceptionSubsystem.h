// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisPersistence.h"
#include "GenesisConceptionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FGenesisOnConceived, const FGenesisConceptionRecord& /*Record*/);

/**
 * Der Übergang vom Mikrokosmos in das Leben: Aus der Verschmelzung entstehen Genom, Körper und Inkarnation.
 *
 * Die Zeugung geschieht genau einmal je Welt. Persistenz-Ebene: World – der Moment gehört zur Biografie,
 * nicht zur Seele (die Seele führt ihn über ihre Inkarnationsliste).
 */
UCLASS()
class GENESISCONCEPTION_API UGenesisConceptionSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.Conception"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/**
	 * Aus der erfolgreichen Zelle entsteht ein Mensch: Elterngenome, Kindgenom, erster Körper,
	 * Beginn der Inkarnation der Spielerseele und die musikalische Lebensphase "Zeugung".
	 *
	 * Wirkt nur beim ersten Aufruf; ein zweiter liefert denselben Datensatz zurück.
	 */
	const FGenesisConceptionRecord& Conceive(const FGenesisFertilizationResult& Fertilization);

	bool HasConceived() const { return Record.bConceived; }
	const FGenesisConceptionRecord& GetRecord() const { return Record; }

	FGenesisOnConceived OnConceived;

private:
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisConceptionRecord Record;
};
