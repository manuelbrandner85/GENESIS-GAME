// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisPersistence.h"
#include "GenesisSoulMusicTypes.h"
#include "GenesisSoulMusicSubsystem.generated.h"

struct FGenesisMemoryTrace;
struct FGenesisSoulSeed;

/** Projekteinstellungen (Project Settings → Genesis → Soul Music). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Soul Music"))
class GENESISSOULMUSIC_API UGenesisSoulMusicSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Tuning")
	FGenesisSoulMusicTuning Tuning;
};

/**
 * Musik abgeschlossener Leben der Spielerseele. Persistenz-Ebene: Soul –
 * überdauert Welten, damit Soundtrack-Export und Jenseits jedes frühere Leben hören können.
 */
UCLASS()
class GENESISSOULMUSIC_API UGenesisSoulMusicArchive : public UObject, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.SoulMusic.Archive"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::Soul; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override { State = FGenesisSoulMusicArchiveState(); }

	const FGenesisSoulMusicArchiveState& GetState() const { return State; }
	void AddLife(FGenesisArchivedLifeMusic&& Life) { State.Lives.Add(MoveTemp(Life)); }

private:
	UPROPERTY()
	FGenesisSoulMusicArchiveState State;
};

/**
 * Seelenmusik der Welt. Persistenz-Ebene: World (Charaktermotive, Beziehungsthemen, Erinnerungsfragmente, laufende Soundtracks).
 *
 * Leitmotiv einer Person: das Seelenmotiv, wenn sie die aktuelle Inkarnation der Spielerseele ist – sonst ihr Charaktermotiv.
 * Zurückgegebene Zeiger gelten nur bis zur nächsten verändernden Operation.
 */
UCLASS()
class GENESISSOULMUSIC_API UGenesisSoulMusicSubsystem : public UGameInstanceSubsystem, public IGenesisPersistentSystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// IGenesisPersistentSystem
	virtual FName GetPersistenceId() const override { return TEXT("Genesis.SoulMusic"); }
	virtual EGenesisPersistenceScope GetPersistenceScope() const override { return EGenesisPersistenceScope::World; }
	virtual int32 GetSchemaVersion() const override { return 1; }
	virtual bool SaveState(TArray<uint8>& OutPayload) const override;
	virtual bool LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion) override;
	virtual void ResetState() override;

	/** Seelenmotiv (Spielerinkarnation) oder Charaktermotiv; fehlt Letzteres, wird es deterministisch aus der ID abgeleitet. */
	FGenesisSoulMotif GetLeitmotif(const FGuid& EntityId) const;
	bool IsPlayerIncarnation(const FGuid& EntityId) const;

	const FGenesisCharacterMotif& GetOrCreateCharacterMotif(const FGuid& EntityId);
	/** Familienmotiv aus den Motiven beider Eltern. Überschreibt ein bestehendes Motiv des Kindes nicht. */
	const FGenesisCharacterMotif& CreateInheritedMotif(const FGuid& ChildId, const FGuid& ParentA, const FGuid& ParentB);
	const FGenesisCharacterMotif* FindCharacterMotif(const FGuid& EntityId) const;

	/** Neue Lebensphase – verändert die Instrumentierung des Leitmotivs und wird ein Soundtrack-Moment. */
	void NotifyLifePhase(const FGuid& EntityId, const FGameplayTag& LifePhase, const FGenesisTimestamp& Time);
	FGameplayTag GetLifePhase(const FGuid& EntityId) const;

	/** Verschmelzung 0..1 (z. B. aus Beziehungsnähe). Beim ersten Überschreiten der Schwelle entsteht ein Soundtrack-Moment. */
	void SetRelationshipFusion(const FGuid& EntityA, const FGuid& EntityB, float Fusion, const FGenesisTimestamp& Time);
	/** Bruch oder Tod: Die Motive trennen sich, eine Narbe bleibt. */
	void SeparateRelationship(const FGuid& EntityA, const FGuid& EntityB, const FGenesisTimestamp& Time);
	const FGenesisRelationshipTheme* FindRelationship(const FGuid& EntityA, const FGuid& EntityB) const;
	/** Das Beziehungsmotiv, wie Perspective es hört. */
	FGenesisSoulMotif GetRelationshipMotif(const FGuid& Perspective, const FGuid& Other) const;

	/** Hält eine intensive Erinnerung musikalisch fest (Phase = aktuelle Lebensphase der Person). nullptr unter der Schwelle. */
	const FGenesisMemoryMusicFragment* CaptureMemoryFragment(const FGuid& OwnerId, const FGenesisMemoryTrace& Trace, const FGenesisTimestamp& Time);
	const FGenesisMemoryMusicFragment* FindFragment(const FGuid& FragmentId) const;
	FGenesisMusicPhrase RecallFragment(const FGuid& FragmentId, float Accuracy) const;

	/** Leitmotiv in der Instrumentierung der aktuellen Lebensphase. */
	FGenesisMusicPhrase RenderLeitmotif(const FGuid& EntityId) const;

	FGenesisDeathComposition ComposeDeath(const FGuid& EntityId) const;

	/**
	 * Nach dem Tod: Todeskomposition als Soundtrack-Moment, bei Spielerinkarnationen Übergabe an das Seelen-Archiv.
	 * Soundtrack und Fragmente der Person werden aus der Welt entfernt; Charaktermotiv und Beziehungen bleiben für Nachkommen.
	 */
	bool ArchiveLife(const FGuid& EntityId, const FGenesisTimestamp& Time);

	const FGenesisLifeSoundtrack* FindSoundtrack(const FGuid& EntityId) const;
	const FGenesisSoulMusicWorldState& GetState() const { return State; }
	const UGenesisSoulMusicArchive* GetArchive() const { return Archive; }

#if !UE_BUILD_SHIPPING
	/** Entwickler-Szenario: ein ganzes Leben musikalisch durchspielen (Ergebnis auf der HUD-Seite "SoulMusic"). */
	void RunDeveloperScenario();
#endif

private:
	const FGenesisSoulSeed* GetPlayerSoul() const;
	FGenesisLifeSoundtrack& GetOrAddSoundtrack(const FGuid& EntityId);
	FGenesisRelationshipTheme* FindRelationshipMutable(const FGuid& EntityA, const FGuid& EntityB);
	void RecordCue(const FGuid& EntityId, EGenesisSoundtrackCue Cue, const FGuid& ReferenceId, float Importance, const FGenesisTimestamp& Time);
	void RebuildIndices();
	void RegisterDebugPage();

	UPROPERTY()
	FGenesisSoulMusicWorldState State;

	UPROPERTY()
	TObjectPtr<UGenesisSoulMusicArchive> Archive;

	TMap<FGuid, int32> MotifIndex;
	TMap<FGuid, int32> SoundtrackIndex;
	TMap<FGuid, int32> FragmentIndex;

	TArray<FString> ScenarioReport;
};
