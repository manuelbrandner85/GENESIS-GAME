// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisPersistence.generated.h"

/**
 * Persistenz-Ebenen. Jede Ebene wird in eine eigene Speicherdatei geschrieben,
 * weil sie unterschiedliche Lebensdauern haben.
 */
UENUM(BlueprintType)
enum class EGenesisPersistenceScope : uint8
{
	/** Überdauert alle Leben und Welten: Soul Seed, Echos, Seelenarchiv, Meta-Ebene. */
	Soul,
	/** Überdauert Generationen innerhalb einer Welt: Weltgeschichte, Kausalgraph, Familien, Orte, Gegenstände. */
	World,
	/** Gilt für die aktuelle Inkarnation: Körper, Psyche, laufende Beziehungen, offene Konsequenzen. */
	Life
};

/** Serialisierter Zustand eines einzelnen Systems innerhalb einer Speicherdatei. */
USTRUCT()
struct GENESISCORE_API FGenesisSystemRecord
{
	GENERATED_BODY()

	/** Stabile System-ID (z. B. "Genesis.Soul"). Darf nach Release nie umbenannt werden. */
	UPROPERTY(SaveGame)
	FName SystemId;

	/** Schema-Version des Systems zum Zeitpunkt des Speicherns (für Migrationen). */
	UPROPERTY(SaveGame)
	int32 SchemaVersion = 0;

	/** Tagged-Property-serialisierter Zustand (tolerant gegenüber hinzugefügten/entfernten Feldern). */
	UPROPERTY(SaveGame)
	TArray<uint8> Payload;
};

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class GENESISCORE_API UGenesisPersistentSystem : public UInterface
{
	GENERATED_BODY()
};

/**
 * Vertrag für jedes System mit persistentem Zustand.
 *
 * Grundsatz: Die Simulation ist die Wahrheit, Actors sind nur Darstellung.
 * Gespeichert werden ausschließlich Simulationsdaten (IDs, Structs), niemals Actor-Zeiger.
 */
class GENESISCORE_API IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	/** Stabile, eindeutige ID dieses Systems. */
	virtual FName GetPersistenceId() const = 0;

	/** In welche Speicherdatei der Zustand gehört. */
	virtual EGenesisPersistenceScope GetPersistenceScope() const = 0;

	/** Aktuelle Schema-Version. Bei inkompatiblen Datenänderungen erhöhen und in LoadState migrieren. */
	virtual int32 GetSchemaVersion() const = 0;

	/** Schreibt den kompletten Zustand in Payload. */
	virtual bool SaveState(TArray<uint8>& OutPayload) const = 0;

	/** Stellt den Zustand wieder her. SavedSchemaVersion erlaubt Migration älterer Stände. */
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) = 0;

	/** Setzt den Zustand auf "neu" zurück (neues Profil, neue Welt oder neues Leben – je nach Scope). */
	virtual void ResetState() = 0;
};

/** Serialisierungshelfer für Zustands-Structs. */
namespace GenesisPersistence
{
	/**
	 * Serialisiert ein USTRUCT per Tagged Property Serialization.
	 * Alle UPROPERTYs (außer Transient) werden geschrieben; Objekt-Referenzen als Pfad-Strings.
	 */
	GENESISCORE_API bool WriteStruct(UScriptStruct* Struct, const void* Value, TArray<uint8>& OutBytes);

	/** Gegenstück zu WriteStruct. Unbekannte Felder werden übersprungen, fehlende behalten Default-Werte. */
	GENESISCORE_API bool ReadStruct(UScriptStruct* Struct, void* Value, const TArray<uint8>& Bytes);

	template <typename TStruct>
	bool Write(const TStruct& Value, TArray<uint8>& OutBytes)
	{
		return WriteStruct(TStruct::StaticStruct(), &Value, OutBytes);
	}

	template <typename TStruct>
	bool Read(TStruct& Value, const TArray<uint8>& Bytes)
	{
		return ReadStruct(TStruct::StaticStruct(), &Value, Bytes);
	}
}

/**
 * Registry aller persistenten Systeme einer GameInstance.
 * Systeme registrieren sich selbst in Initialize(); GenesisSave fragt die Registry beim Speichern/Laden ab.
 * Dadurch hängt kein Spielsystem vom Save-Modul ab – nur von dieser Schnittstelle.
 */
UCLASS()
class GENESISCORE_API UGenesisPersistenceRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** System muss IGenesisPersistentSystem implementieren und eine eindeutige Persistence-ID besitzen. */
	void RegisterSystem(UObject* SystemObject);
	void UnregisterSystem(UObject* SystemObject);

	/** Alle Systeme eines Scopes, sortiert nach ID (deterministische Reihenfolge in Speicherdateien). */
	TArray<IGenesisPersistentSystem*> GetSystems(EGenesisPersistenceScope Scope) const;

	IGenesisPersistentSystem* FindSystem(FName PersistenceId) const;

private:
	TArray<TWeakObjectPtr<UObject>> RegisteredSystems;
};
