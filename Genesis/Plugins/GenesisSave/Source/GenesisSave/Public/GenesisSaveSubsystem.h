// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisPersistence.h"
#include "GenesisSaveSubsystem.generated.h"

class UGenesisSaveGame;

UENUM(BlueprintType)
enum class EGenesisSaveResult : uint8
{
	Success,
	/** Kein Profil aktiv. */
	NoProfile,
	/** Serialisierung eines Systems fehlgeschlagen. */
	SerializationFailed,
	/** Datei konnte nicht geschrieben werden. */
	WriteFailed,
	/** Datei existiert nicht (weder Primär- noch Backup-Slot). */
	NotFound,
	/** Datei beschädigt, Prüfsumme/Format ungültig – auch Backup unbrauchbar. */
	Corrupted,
	/** Speicherstand stammt aus einer neueren Spielversion. */
	NewerVersion,
	/** Ein System konnte seinen Zustand nicht laden; alle Systeme des Scopes wurden zurückgerollt. */
	ApplyFailed,
	/** Für diesen Slot läuft bereits ein asynchroner Schreibvorgang. */
	Busy
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FGenesisOnSaveFinished, EGenesisPersistenceScope /*Scope*/, EGenesisSaveResult /*Result*/);

/**
 * Speichert und lädt die drei Persistenz-Ebenen (Soul / World / Life).
 *
 * Ablauf Speichern: Systeme des Scopes → Records → Prüfsumme → Backup des alten Stands → Schreiben.
 * Ablauf Laden:    Primärdatei → Validierung → (bei Fehler Backup) → Versionsprüfung → Snapshot → Anwenden → (bei Fehler Rollback).
 *
 * Gespeichert wird ereignisbasiert (Lebensphasen-Übergänge, Schlaf, Tod, bedeutende Ereignisse) – nie pro Frame.
 */
UCLASS()
class GENESISSAVE_API UGenesisSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Aktiviert ein Spielerprofil. Profil-IDs werden auf Dateinamen-sichere Zeichen reduziert. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Save")
	void SetActiveProfile(const FString& ProfileId);

	UFUNCTION(BlueprintPure, Category = "Genesis|Save")
	const FString& GetActiveProfile() const { return ActiveProfileId; }

	/** Wählt den Slot für die Life-Ebene (mehrere parallele Lebensstände pro Profil). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Save")
	void SetActiveLifeSlot(int32 LifeSlot) { ActiveLifeSlot = FMath::Max(0, LifeSlot); }

	/** Synchrones Speichern eines Scopes. Für kritische Übergänge (Tod → Seelenarchiv). */
	EGenesisSaveResult SaveScope(EGenesisPersistenceScope Scope);

	/** Asynchrones Speichern: Snapshot auf dem Game Thread, Schreiben im Hintergrund. */
	EGenesisSaveResult SaveScopeAsync(EGenesisPersistenceScope Scope);

	/** Lädt einen Scope transaktional. */
	EGenesisSaveResult LoadScope(EGenesisPersistenceScope Scope);

	/** Speichert Soul, World und Life nacheinander (synchron). */
	EGenesisSaveResult SaveAll();

	/** Lädt Soul, World und Life. Fehlende Dateien setzen den Scope auf Neuzustand zurück. */
	EGenesisSaveResult LoadAll();

	bool DoesScopeSaveExist(EGenesisPersistenceScope Scope) const;

	/** Setzt alle Systeme eines Scopes auf den Neuzustand. */
	void ResetScope(EGenesisPersistenceScope Scope);

	/** Erzeugt einen Speicherstand im Speicher (ohne Datei). Grundlage für Tests, Rollback und Async-Save. */
	UGenesisSaveGame* CaptureScope(EGenesisPersistenceScope Scope, EGenesisSaveResult& OutResult) const;

	/** Wendet einen Speicherstand transaktional an. */
	EGenesisSaveResult ApplySaveGame(const UGenesisSaveGame& SaveGame);

	/** Serialisiert eine beliebige Systemliste in einen Speicherstand (ohne GameInstance – testbar). */
	static UGenesisSaveGame* CaptureSystems(const TArray<IGenesisPersistentSystem*>& Systems, EGenesisPersistenceScope Scope, EGenesisSaveResult& OutResult);

	/** Wendet einen Speicherstand transaktional auf eine Systemliste an (Versionsprüfung, Snapshot, Rollback). */
	static EGenesisSaveResult ApplyToSystems(const UGenesisSaveGame& SaveGame, const TArray<IGenesisPersistentSystem*>& Systems);

	/** Slot-Name nach Schema Genesis_<Profil>_<Scope>[_<LifeSlot>]. */
	static FString BuildSlotName(const FString& ProfileId, EGenesisPersistenceScope Scope, int32 LifeSlot);

	static FString SanitizeProfileId(const FString& ProfileId);

	FGenesisOnSaveFinished OnSaveFinished;
	FGenesisOnSaveFinished OnLoadFinished;

private:
	FString GetSlotName(EGenesisPersistenceScope Scope) const;

	/** Schreibt Bytes, vorher wird der bisherige Stand in den Backup-Slot rotiert. */
	static bool WriteSlotWithBackup(const FString& SlotName, const TArray<uint8>& Bytes);

	/** Lädt und validiert einen Slot; fällt bei Beschädigung auf das Backup zurück. */
	UGenesisSaveGame* ReadValidatedSlot(const FString& SlotName, EGenesisSaveResult& OutResult) const;

	UGenesisPersistenceRegistry* GetRegistry() const;

	FString ActiveProfileId;
	int32 ActiveLifeSlot = 0;

	/** Slots mit laufendem Hintergrund-Schreibvorgang. */
	TSet<FString> SlotsBeingWritten;
};
