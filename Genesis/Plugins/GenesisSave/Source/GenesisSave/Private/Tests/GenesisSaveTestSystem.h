// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenesisPersistence.h"
#include "GenesisWorldClockSubsystem.h"
#include "GenesisSaveTestSystem.generated.h"

/**
 * Testsystem für die Automation-Tests des Save-Moduls.
 * Simuliert ein persistentes System inklusive Fehlerfällen (Ladefehler, neuere Schema-Version).
 * Wird nur von Tests instanziert.
 */
UCLASS(NotBlueprintable, Transient, HideDropdown)
class UGenesisSaveTestSystem : public UObject, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual FName GetPersistenceId() const override { return SystemId; }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return Scope; }
	virtual int32 GetSchemaVersion() const override { return SchemaVersion; }

	virtual bool SaveState(TArray<uint8>& OutPayload) const override
	{
		return GenesisPersistence::Write(State, OutPayload);
	}

	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override
	{
		if (bFailOnLoad)
		{
			return false;
		}
		FGenesisWorldClockState Loaded;
		if (!GenesisPersistence::Read(Loaded, Payload))
		{
			return false;
		}
		State = Loaded;
		return true;
	}

	virtual void ResetState() override
	{
		State = FGenesisWorldClockState();
		++ResetCount;
	}

	FName SystemId = TEXT("Genesis.Test.System");
	EGenesisPersistenceScope Scope = EGenesisPersistenceScope::World;
	int32 SchemaVersion = 1;
	bool bFailOnLoad = false;
	int32 ResetCount = 0;

	UPROPERTY()
	FGenesisWorldClockState State;
};
