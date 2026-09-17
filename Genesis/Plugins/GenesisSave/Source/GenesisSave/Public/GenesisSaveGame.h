// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GenesisPersistence.h"
#include "GenesisSaveGame.generated.h"

/** Kopfdaten einer GENESIS-Speicherdatei. */
USTRUCT(BlueprintType)
struct GENESISSAVE_API FGenesisSaveHeader
{
	GENERATED_BODY()

	/** Version des Datei-Containers (nicht der einzelnen Systeme). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	int32 FormatVersion = 0;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	EGenesisPersistenceScope Scope = EGenesisPersistenceScope::Life;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	FString ProfileId;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	FString SlotName;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	FDateTime SavedAtUtc;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Save")
	FString BuildVersion;

	/** CRC32 über alle System-Records – erkennt beschädigte oder manipulierte Dateien. */
	UPROPERTY(SaveGame)
	uint32 RecordsChecksum = 0;
};

/**
 * Container einer Speicherdatei. Enthält keine Spiellogik – nur Header und die Records
 * der registrierten persistenten Systeme eines Scopes.
 */
UCLASS()
class GENESISSAVE_API UGenesisSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Aktuelle Container-Version. Bei Änderungen am Dateiaufbau erhöhen. */
	static constexpr int32 CurrentFormatVersion = 1;

	UPROPERTY(SaveGame)
	FGenesisSaveHeader Header;

	UPROPERTY(SaveGame)
	TArray<FGenesisSystemRecord> Records;

	const FGenesisSystemRecord* FindRecord(FName SystemId) const;

	/** Deterministische Prüfsumme über alle Records. */
	static uint32 ComputeChecksum(const TArray<FGenesisSystemRecord>& InRecords);

	/** Aktualisiert Header.RecordsChecksum. */
	void UpdateChecksum();

	/** true, wenn Prüfsumme und Format-Version gültig sind. */
	bool Validate(FString& OutError) const;
};
