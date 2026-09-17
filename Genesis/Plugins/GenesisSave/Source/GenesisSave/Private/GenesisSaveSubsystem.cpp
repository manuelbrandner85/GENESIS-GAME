// GENESIS: Der Kreislauf des Lebens

#include "GenesisSaveSubsystem.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "HAL/IConsoleManager.h"
#include "GenesisLog.h"
#include "GenesisSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

namespace
{
	const TCHAR* ScopeToString(EGenesisPersistenceScope Scope)
	{
		switch (Scope)
		{
		case EGenesisPersistenceScope::Soul:  return TEXT("Soul");
		case EGenesisPersistenceScope::World: return TEXT("World");
		case EGenesisPersistenceScope::Life:  return TEXT("Life");
		default:                              return TEXT("Unknown");
		}
	}

	const TCHAR* BackupSuffix = TEXT("_Backup");
	const int32 SaveUserIndex = 0;
}

#if !UE_BUILD_SHIPPING
namespace
{
	UGenesisSaveSubsystem* GetSaveSubsystem(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisSaveSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisSaveAllCommand(
		TEXT("genesis.Save.All"),
		TEXT("Speichert Soul, World und Life. Optional: Profil-ID (Standard: Dev)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisSaveSubsystem* Save = GetSaveSubsystem(World))
			{
				Save->SetActiveProfile(Args.Num() > 0 ? Args[0] : TEXT("Dev"));
				UE_LOG(LogGenesis, Display, TEXT("genesis.Save.All → %d"), static_cast<int32>(Save->SaveAll()));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisLoadAllCommand(
		TEXT("genesis.Save.LoadAll"),
		TEXT("Lädt Soul, World und Life. Optional: Profil-ID (Standard: Dev)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisSaveSubsystem* Save = GetSaveSubsystem(World))
			{
				Save->SetActiveProfile(Args.Num() > 0 ? Args[0] : TEXT("Dev"));
				UE_LOG(LogGenesis, Display, TEXT("genesis.Save.LoadAll → %d"), static_cast<int32>(Save->LoadAll()));
			}
		}));
}
#endif

void UGenesisSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGenesisPersistenceRegistry>();

#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisSaveSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Save"),
		TEXT("Save"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisSaveSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			OutLines.Add(FString::Printf(TEXT("Profil: %s | Life-Slot: %d | Async-Schreibvorgänge: %d"),
				Self->ActiveProfileId.IsEmpty() ? TEXT("<keins>") : *Self->ActiveProfileId, Self->ActiveLifeSlot, Self->SlotsBeingWritten.Num()));
			if (const UGenesisPersistenceRegistry* Registry = Self->GetRegistry())
			{
				for (EGenesisPersistenceScope Scope : { EGenesisPersistenceScope::Soul, EGenesisPersistenceScope::World, EGenesisPersistenceScope::Life })
				{
					TArray<FString> Ids;
					for (const IGenesisPersistentSystem* System : Registry->GetSystems(Scope))
					{
						Ids.Add(FString::Printf(TEXT("%s(v%d)"), *System->GetPersistenceId().ToString(), System->GetSchemaVersion()));
					}
					OutLines.Add(FString::Printf(TEXT("  %s: %s"), ScopeToString(Scope), *FString::Join(Ids, TEXT(", "))));
				}
			}
		}
	});
#endif
}

void UGenesisSaveSubsystem::SetActiveProfile(const FString& ProfileId)
{
	ActiveProfileId = SanitizeProfileId(ProfileId);
}

FString UGenesisSaveSubsystem::SanitizeProfileId(const FString& ProfileId)
{
	FString Result;
	Result.Reserve(ProfileId.Len());
	for (TCHAR Character : ProfileId)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('-'))
		{
			Result.AppendChar(Character);
		}
	}
	return Result.Left(48);
}

FString UGenesisSaveSubsystem::BuildSlotName(const FString& ProfileId, EGenesisPersistenceScope Scope, int32 LifeSlot)
{
	if (Scope == EGenesisPersistenceScope::Life)
	{
		return FString::Printf(TEXT("Genesis_%s_%s_%d"), *ProfileId, ScopeToString(Scope), LifeSlot);
	}
	return FString::Printf(TEXT("Genesis_%s_%s"), *ProfileId, ScopeToString(Scope));
}

FString UGenesisSaveSubsystem::GetSlotName(EGenesisPersistenceScope Scope) const
{
	return BuildSlotName(ActiveProfileId, Scope, ActiveLifeSlot);
}

UGenesisPersistenceRegistry* UGenesisSaveSubsystem::GetRegistry() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UGenesisPersistenceRegistry>() : nullptr;
}

UGenesisSaveGame* UGenesisSaveSubsystem::CaptureSystems(const TArray<IGenesisPersistentSystem*>& Systems, EGenesisPersistenceScope Scope, EGenesisSaveResult& OutResult)
{
	OutResult = EGenesisSaveResult::Success;

	UGenesisSaveGame* SaveGame = NewObject<UGenesisSaveGame>(GetTransientPackage());
	SaveGame->Header.FormatVersion = UGenesisSaveGame::CurrentFormatVersion;
	SaveGame->Header.Scope = Scope;
	SaveGame->Header.SavedAtUtc = FDateTime::UtcNow();
	SaveGame->Header.BuildVersion = FApp::GetBuildVersion();

	for (const IGenesisPersistentSystem* System : Systems)
	{
		if (!System || System->GetPersistenceScope() != Scope)
		{
			continue;
		}

		FGenesisSystemRecord& Record = SaveGame->Records.AddDefaulted_GetRef();
		Record.SystemId = System->GetPersistenceId();
		Record.SchemaVersion = System->GetSchemaVersion();
		if (!System->SaveState(Record.Payload))
		{
			UE_LOG(LogGenesis, Error, TEXT("Speichern: System '%s' konnte seinen Zustand nicht serialisieren."), *Record.SystemId.ToString());
			OutResult = EGenesisSaveResult::SerializationFailed;
		}
	}

	SaveGame->UpdateChecksum();
	return SaveGame;
}

EGenesisSaveResult UGenesisSaveSubsystem::ApplyToSystems(const UGenesisSaveGame& SaveGame, const TArray<IGenesisPersistentSystem*>& AllSystems)
{
	const EGenesisPersistenceScope Scope = SaveGame.Header.Scope;

	TArray<IGenesisPersistentSystem*> Systems;
	for (IGenesisPersistentSystem* System : AllSystems)
	{
		if (System && System->GetPersistenceScope() == Scope)
		{
			Systems.Add(System);
		}
	}

	// 1) Versionsprüfung vor jeder Änderung: Stände aus neueren Builds werden nie teilweise angewendet
	for (const IGenesisPersistentSystem* System : Systems)
	{
		if (const FGenesisSystemRecord* Record = SaveGame.FindRecord(System->GetPersistenceId()))
		{
			if (Record->SchemaVersion > System->GetSchemaVersion())
			{
				UE_LOG(LogGenesis, Error, TEXT("Laden: '%s' hat Schema v%d, dieser Build unterstützt nur v%d."),
					*Record->SystemId.ToString(), Record->SchemaVersion, System->GetSchemaVersion());
				return EGenesisSaveResult::NewerVersion;
			}
		}
	}

	for (const FGenesisSystemRecord& Record : SaveGame.Records)
	{
		const bool bKnown = Systems.ContainsByPredicate([&Record](const IGenesisPersistentSystem* System)
		{
			return System->GetPersistenceId() == Record.SystemId;
		});
		if (!bKnown)
		{
			UE_LOG(LogGenesis, Warning, TEXT("Laden: Record '%s' gehört zu keinem registrierten System und wird ignoriert."), *Record.SystemId.ToString());
		}
	}

	// 2) Snapshot des aktuellen Zustands für Rollback
	EGenesisSaveResult SnapshotResult;
	const UGenesisSaveGame* Snapshot = CaptureSystems(Systems, Scope, SnapshotResult);

	// 3) Anwenden
	bool bAllApplied = true;
	for (IGenesisPersistentSystem* System : Systems)
	{
		const FGenesisSystemRecord* Record = SaveGame.FindRecord(System->GetPersistenceId());
		if (!Record)
		{
			// System ist neuer als der Speicherstand → Neuzustand
			System->ResetState();
			continue;
		}

		if (!System->LoadState(Record->Payload, Record->SchemaVersion))
		{
			UE_LOG(LogGenesis, Error, TEXT("Laden: System '%s' konnte seinen Zustand nicht wiederherstellen."), *Record->SystemId.ToString());
			bAllApplied = false;
			break;
		}
	}

	if (bAllApplied)
	{
		return EGenesisSaveResult::Success;
	}

	// 4) Rollback auf Snapshot
	for (IGenesisPersistentSystem* System : Systems)
	{
		const FGenesisSystemRecord* Record = Snapshot ? Snapshot->FindRecord(System->GetPersistenceId()) : nullptr;
		if (!Record || !System->LoadState(Record->Payload, Record->SchemaVersion))
		{
			System->ResetState();
		}
	}
	return EGenesisSaveResult::ApplyFailed;
}

UGenesisSaveGame* UGenesisSaveSubsystem::CaptureScope(EGenesisPersistenceScope Scope, EGenesisSaveResult& OutResult) const
{
	const UGenesisPersistenceRegistry* Registry = GetRegistry();
	UGenesisSaveGame* SaveGame = CaptureSystems(Registry ? Registry->GetSystems(Scope) : TArray<IGenesisPersistentSystem*>(), Scope, OutResult);
	SaveGame->Header.ProfileId = ActiveProfileId;
	SaveGame->Header.SlotName = GetSlotName(Scope);
	return SaveGame;
}

EGenesisSaveResult UGenesisSaveSubsystem::ApplySaveGame(const UGenesisSaveGame& SaveGame)
{
	const UGenesisPersistenceRegistry* Registry = GetRegistry();
	if (!Registry)
	{
		return EGenesisSaveResult::ApplyFailed;
	}
	return ApplyToSystems(SaveGame, Registry->GetSystems(SaveGame.Header.Scope));
}

bool UGenesisSaveSubsystem::WriteSlotWithBackup(const FString& SlotName, const TArray<uint8>& Bytes)
{
	TArray<uint8> PreviousBytes;
	if (UGameplayStatics::LoadDataFromSlot(PreviousBytes, SlotName, SaveUserIndex) && PreviousBytes.Num() > 0)
	{
		UGameplayStatics::SaveDataToSlot(PreviousBytes, SlotName + BackupSuffix, SaveUserIndex);
	}
	return UGameplayStatics::SaveDataToSlot(Bytes, SlotName, SaveUserIndex);
}

UGenesisSaveGame* UGenesisSaveSubsystem::ReadValidatedSlot(const FString& SlotName, EGenesisSaveResult& OutResult) const
{
	bool bAnyFileFound = false;

	for (const FString& Candidate : { SlotName, SlotName + BackupSuffix })
	{
		TArray<uint8> Bytes;
		if (!UGameplayStatics::LoadDataFromSlot(Bytes, Candidate, SaveUserIndex) || Bytes.Num() == 0)
		{
			continue;
		}
		bAnyFileFound = true;

		UGenesisSaveGame* SaveGame = Cast<UGenesisSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
		FString Error = TEXT("Kein GENESIS-Speicherstand.");
		if (SaveGame && SaveGame->Validate(Error))
		{
			if (Candidate != SlotName)
			{
				UE_LOG(LogGenesis, Warning, TEXT("Laden: '%s' war beschädigt – Backup wird verwendet."), *SlotName);
			}
			OutResult = EGenesisSaveResult::Success;
			return SaveGame;
		}

		UE_LOG(LogGenesis, Error, TEXT("Laden: '%s' ungültig: %s"), *Candidate, *Error);
	}

	OutResult = bAnyFileFound ? EGenesisSaveResult::Corrupted : EGenesisSaveResult::NotFound;
	return nullptr;
}

EGenesisSaveResult UGenesisSaveSubsystem::SaveScope(EGenesisPersistenceScope Scope)
{
	if (ActiveProfileId.IsEmpty())
	{
		return EGenesisSaveResult::NoProfile;
	}

	const FString SlotName = GetSlotName(Scope);
	if (SlotsBeingWritten.Contains(SlotName))
	{
		return EGenesisSaveResult::Busy;
	}

	EGenesisSaveResult Result;
	UGenesisSaveGame* SaveGame = CaptureScope(Scope, Result);

	TArray<uint8> Bytes;
	if (Result == EGenesisSaveResult::Success && !UGameplayStatics::SaveGameToMemory(SaveGame, Bytes))
	{
		Result = EGenesisSaveResult::SerializationFailed;
	}
	if (Result == EGenesisSaveResult::Success && !WriteSlotWithBackup(SlotName, Bytes))
	{
		Result = EGenesisSaveResult::WriteFailed;
	}

	UE_LOG(LogGenesis, Log, TEXT("Speichern %s → %s: %s (%d Bytes)"), ScopeToString(Scope), *SlotName,
		Result == EGenesisSaveResult::Success ? TEXT("OK") : TEXT("FEHLER"), Bytes.Num());
	OnSaveFinished.Broadcast(Scope, Result);
	return Result;
}

EGenesisSaveResult UGenesisSaveSubsystem::SaveScopeAsync(EGenesisPersistenceScope Scope)
{
	if (ActiveProfileId.IsEmpty())
	{
		return EGenesisSaveResult::NoProfile;
	}

	const FString SlotName = GetSlotName(Scope);
	if (SlotsBeingWritten.Contains(SlotName))
	{
		return EGenesisSaveResult::Busy;
	}

	// Snapshot + Serialisierung auf dem Game Thread (Systemzustand ist hier konsistent)
	EGenesisSaveResult Result;
	UGenesisSaveGame* SaveGame = CaptureScope(Scope, Result);
	TArray<uint8> Bytes;
	if (Result != EGenesisSaveResult::Success || !UGameplayStatics::SaveGameToMemory(SaveGame, Bytes))
	{
		return EGenesisSaveResult::SerializationFailed;
	}

	SlotsBeingWritten.Add(SlotName);
	TWeakObjectPtr<UGenesisSaveSubsystem> WeakThis(this);

	// Nur die Dateioperation läuft im Hintergrund
	Async(EAsyncExecution::ThreadPool, [WeakThis, SlotName, Scope, Bytes = MoveTemp(Bytes)]()
	{
		const bool bWritten = WriteSlotWithBackup(SlotName, Bytes);

		AsyncTask(ENamedThreads::GameThread, [WeakThis, SlotName, Scope, bWritten]()
		{
			if (UGenesisSaveSubsystem* Self = WeakThis.Get())
			{
				Self->SlotsBeingWritten.Remove(SlotName);
				Self->OnSaveFinished.Broadcast(Scope, bWritten ? EGenesisSaveResult::Success : EGenesisSaveResult::WriteFailed);
			}
		});
	});

	return EGenesisSaveResult::Success;
}

EGenesisSaveResult UGenesisSaveSubsystem::LoadScope(EGenesisPersistenceScope Scope)
{
	if (ActiveProfileId.IsEmpty())
	{
		return EGenesisSaveResult::NoProfile;
	}

	EGenesisSaveResult Result;
	const UGenesisSaveGame* SaveGame = ReadValidatedSlot(GetSlotName(Scope), Result);
	if (SaveGame)
	{
		Result = ApplySaveGame(*SaveGame);
	}

	UE_LOG(LogGenesis, Log, TEXT("Laden %s: Ergebnis %d"), ScopeToString(Scope), static_cast<int32>(Result));
	OnLoadFinished.Broadcast(Scope, Result);
	return Result;
}

EGenesisSaveResult UGenesisSaveSubsystem::SaveAll()
{
	for (EGenesisPersistenceScope Scope : { EGenesisPersistenceScope::Soul, EGenesisPersistenceScope::World, EGenesisPersistenceScope::Life })
	{
		const EGenesisSaveResult Result = SaveScope(Scope);
		if (Result != EGenesisSaveResult::Success)
		{
			return Result;
		}
	}
	return EGenesisSaveResult::Success;
}

EGenesisSaveResult UGenesisSaveSubsystem::LoadAll()
{
	for (EGenesisPersistenceScope Scope : { EGenesisPersistenceScope::Soul, EGenesisPersistenceScope::World, EGenesisPersistenceScope::Life })
	{
		const EGenesisSaveResult Result = LoadScope(Scope);
		if (Result == EGenesisSaveResult::NotFound)
		{
			ResetScope(Scope);
			continue;
		}
		if (Result != EGenesisSaveResult::Success)
		{
			return Result;
		}
	}
	return EGenesisSaveResult::Success;
}

bool UGenesisSaveSubsystem::DoesScopeSaveExist(EGenesisPersistenceScope Scope) const
{
	const FString SlotName = GetSlotName(Scope);
	return UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex)
		|| UGameplayStatics::DoesSaveGameExist(SlotName + BackupSuffix, SaveUserIndex);
}

void UGenesisSaveSubsystem::ResetScope(EGenesisPersistenceScope Scope)
{
	if (UGenesisPersistenceRegistry* Registry = GetRegistry())
	{
		for (IGenesisPersistentSystem* System : Registry->GetSystems(Scope))
		{
			System->ResetState();
		}
	}
}
