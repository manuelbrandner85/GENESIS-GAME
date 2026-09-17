// GENESIS: Der Kreislauf des Lebens

#include "GenesisPersistence.h"
#include "GenesisLog.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace GenesisPersistence
{
	bool WriteStruct(UScriptStruct* Struct, const void* Value, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		if (!Struct || !Value)
		{
			return false;
		}

		FMemoryWriter Writer(OutBytes, /*bIsPersistent*/ true);
		FObjectAndNameAsStringProxyArchive Archive(Writer, /*bInLoadIfFindFails*/ false);

		// Bewusst ohne ArIsSaveGame: Zustands-Structs sind reine Speicherdaten, jedes UPROPERTY gehört dazu.
		// Laufzeit-Caches werden nicht als UPROPERTY deklariert oder als Transient markiert.
		Struct->SerializeItem(Archive, const_cast<void*>(Value), nullptr);

		return !Archive.IsError() && !Writer.IsError();
	}

	bool ReadStruct(UScriptStruct* Struct, void* Value, const TArray<uint8>& Bytes)
	{
		if (!Struct || !Value || Bytes.Num() == 0)
		{
			return false;
		}

		FMemoryReader Reader(Bytes, /*bIsPersistent*/ true);
		FObjectAndNameAsStringProxyArchive Archive(Reader, /*bInLoadIfFindFails*/ true);

		Struct->SerializeItem(Archive, Value, nullptr);

		return !Archive.IsError() && !Reader.IsError();
	}
}

void UGenesisPersistenceRegistry::RegisterSystem(UObject* SystemObject)
{
	IGenesisPersistentSystem* System = Cast<IGenesisPersistentSystem>(SystemObject);
	if (!ensureMsgf(System, TEXT("RegisterSystem: %s implementiert IGenesisPersistentSystem nicht."), *GetNameSafe(SystemObject)))
	{
		return;
	}

	const FName NewId = System->GetPersistenceId();
	if (IGenesisPersistentSystem* Existing = FindSystem(NewId))
	{
		if (Existing != System)
		{
			UE_LOG(LogGenesis, Error, TEXT("Persistence-ID '%s' ist doppelt vergeben (%s). System wird nicht registriert."),
				*NewId.ToString(), *GetNameSafe(SystemObject));
		}
		return;
	}

	RegisteredSystems.Add(SystemObject);
	UE_LOG(LogGenesis, Verbose, TEXT("Persistentes System registriert: %s"), *NewId.ToString());
}

void UGenesisPersistenceRegistry::UnregisterSystem(UObject* SystemObject)
{
	RegisteredSystems.RemoveAll([SystemObject](const TWeakObjectPtr<UObject>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == SystemObject;
	});
}

TArray<IGenesisPersistentSystem*> UGenesisPersistenceRegistry::GetSystems(EGenesisPersistenceScope Scope) const
{
	TArray<IGenesisPersistentSystem*> Result;
	for (const TWeakObjectPtr<UObject>& Entry : RegisteredSystems)
	{
		if (IGenesisPersistentSystem* System = Cast<IGenesisPersistentSystem>(Entry.Get()))
		{
			if (System->GetPersistenceScope() == Scope)
			{
				Result.Add(System);
			}
		}
	}

	Result.Sort([](const IGenesisPersistentSystem& A, const IGenesisPersistentSystem& B)
	{
		return A.GetPersistenceId().LexicalLess(B.GetPersistenceId());
	});
	return Result;
}

IGenesisPersistentSystem* UGenesisPersistenceRegistry::FindSystem(FName PersistenceId) const
{
	for (const TWeakObjectPtr<UObject>& Entry : RegisteredSystems)
	{
		if (IGenesisPersistentSystem* System = Cast<IGenesisPersistentSystem>(Entry.Get()))
		{
			if (System->GetPersistenceId() == PersistenceId)
			{
				return System;
			}
		}
	}
	return nullptr;
}
