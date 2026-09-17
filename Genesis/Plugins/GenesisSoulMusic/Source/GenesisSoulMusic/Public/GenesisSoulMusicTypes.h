// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisSoulTypes.h"
#include "GenesisTypes.h"
#include "GenesisSoulMusicTypes.generated.h"

/**
 * Klangfarben der Seelenmusik. Konkrete Klänge liefern ab GENESIS-011 prozedurale MetaSounds;
 * hier steht nur, WELCHES Instrument eine Note spielt.
 */
UENUM(BlueprintType)
enum class EGenesisInstrument : uint8
{
	MusicBox,
	Piano,
	Guitar,
	Strings,
	Woodwinds,
	Orchestra,
	Choir,
	CosmicPad
};

/** Eine Instrumentenschicht einer Phasen-Instrumentierung. */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisInstrumentLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music")
	EGenesisInstrument Instrument = EGenesisInstrument::Piano;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "0", ClampMax = "1"))
	float Gain = 1.0f;

	/** Oktavlage relativ zur Tonika der Phase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "-3", ClampMax = "3"))
	int32 OctaveOffset = 0;
};

/**
 * Wie das Seelenmotiv in einer Lebensphase klingt.
 * Spieldauer-Idee: Spieluhr am Anfang, Klavier/Gitarre in der Jugend, Streicher im Erwachsenenleben, reduziertes Klavier im Alter,
 * Orchester beim Tod, Chor und kosmische Flächen danach, kaum hörbare Spieluhr bei der Wiedergeburt.
 */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisPhaseArrangement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (Categories = "Genesis.LifePhase"))
	FGameplayTag LifePhase;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music")
	TArray<FGenesisInstrumentLayer> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "20", ClampMax = "200"))
	float TempoBpm = 72.0f;

	/** 0..1 – Anteil der gespielten Motivnoten. Der Identitätskern und die letzte Note bleiben immer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "0", ClampMax = "1"))
	float NoteDensity = 1.0f;

	/** Tonika als MIDI-Note (60 = C4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "24", ClampMax = "96"))
	int32 TonicMidiNote = 60;

	/** 0..1 – wie präsent die Musik insgesamt ist (0,05 = kaum hörbar). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "0", ClampMax = "1"))
	float Presence = 0.5f;

	/** 0..1 – Raumanteil (Hall). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Music", meta = (ClampMin = "0", ClampMax = "1"))
	float Space = 0.4f;
};

/** Eine gespielte Note. */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisMusicNote
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	int32 MidiPitch = 60;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float StartBeat = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float DurationBeats = 1.0f;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float Velocity = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	EGenesisInstrument Instrument = EGenesisInstrument::Piano;
};

/** Eine spielbare Phrase (Eingang für Music Director / Quartz / MetaSounds). */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisMusicPhrase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	TArray<FGenesisMusicNote> Notes;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float TempoBpm = 72.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float Presence = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float Space = 0.4f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float LengthBeats = 0.0f;

	float GetLengthSeconds() const { return TempoBpm > 0.0f ? LengthBeats * 60.0f / TempoBpm : 0.0f; }
};

/**
 * Motiv einer Person (Familien-/Generationsmotiv).
 * Getrennt vom Seelenmotiv: Das Charaktermotiv folgt der Abstammung, das Seelenmotiv dem Soul Seed.
 */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisCharacterMotif
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EntityId;

	UPROPERTY()
	FGenesisSoulMotif Motif;

	UPROPERTY()
	FGuid ParentA;

	UPROPERTY()
	FGuid ParentB;

	/** 0 = ohne bekannte Eltern-Motive. */
	UPROPERTY()
	int32 Generation = 0;
};

/** Musikalisches Beziehungsthema zweier Personen. EntityA < EntityB (kanonische Reihenfolge). */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisRelationshipTheme
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EntityA;

	UPROPERTY()
	FGuid EntityB;

	/** 0..1 – wie sehr die Motive ineinander aufgegangen sind. */
	UPROPERTY()
	float Fusion = 0.0f;

	/** Nach einer Trennung: wie viel des anderen im eigenen Motiv bleibt (Narbe). */
	UPROPERTY()
	float Scar = 0.0f;

	UPROPERTY()
	bool bSeparated = false;

	/** Die Verschmelzung wurde bereits als Soundtrack-Moment festgehalten. */
	UPROPERTY()
	bool bFusionCueRecorded = false;
};

UENUM(BlueprintType)
enum class EGenesisSoundtrackCue : uint8
{
	PhaseChange,
	SoulMotif,
	CharacterMotif,
	RelationshipFusion,
	RelationshipSeparation,
	MemoryFragment,
	DeathComposition
};

/**
 * Musikalisches Erinnerungsfragment. Hält das Motiv so fest, wie es im Moment des Erlebens klang –
 * spätere Motiv-Entwicklungen verändern die Erinnerung nicht, ungenaues Erinnern schon.
 */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisMemoryMusicFragment
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid FragmentId;

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	FGuid TraceId;

	UPROPERTY()
	FGuid EventId;

	UPROPERTY()
	FGenesisTimestamp CreatedAt;

	UPROPERTY()
	FGameplayTag LifePhase;

	/** Ausschnitt des Motivs (2–4 Noten), emotional eingefärbt. */
	UPROPERTY()
	FGenesisSoulMotif Snapshot;

	/** Position des Ausschnitts im Quellmotiv. */
	UPROPERTY()
	int32 SourceNoteIndex = 0;

	/** −1..1 */
	UPROPERTY()
	float Valence = 0.0f;

	/** 0..1 */
	UPROPERTY()
	float Intensity = 0.0f;
};

/** Ein Moment im Lebens-Soundtrack. */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisSoundtrackEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGenesisTimestamp Time;

	UPROPERTY()
	EGenesisSoundtrackCue Cue = EGenesisSoundtrackCue::PhaseChange;

	UPROPERTY()
	FGameplayTag LifePhase;

	/** Fragment, andere Person oder leer. */
	UPROPERTY()
	FGuid ReferenceId;

	/** 0..1 – entscheidet, was bei Platzmangel verdichtet wird. */
	UPROPERTY()
	float Importance = 0.5f;
};

/** Soundtrack eines Lebens (Grundlage für Todeskomposition und Soundtrack-Export). */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisLifeSoundtrack
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EntityId;

	UPROPERTY()
	FGameplayTag CurrentPhase;

	/** Chronologisch. */
	UPROPERTY()
	TArray<FGenesisSoundtrackEntry> Entries;
};

/** Ein Abschnitt der Todeskomposition. */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisDeathSection
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	EGenesisSoundtrackCue Cue = EGenesisSoundtrackCue::SoulMotif;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	FGuid ReferenceId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	FGameplayTag LifePhase;

	/** Sekunden statt Schlägen, weil jeder Abschnitt sein eigenes Tempo hat. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float StartSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	FGenesisMusicPhrase Phrase;
};

/** Das Leben als Musik im Moment des Todes: Anfang, prägende Erinnerungen, wichtigste Bindung, Seele. */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisDeathComposition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	FGuid EntityId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	TArray<FGenesisDeathSection> Sections;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Music")
	float TotalSeconds = 0.0f;
};

/** Stellschrauben (Project Settings → Genesis → Soul Music). */
USTRUCT(BlueprintType)
struct GENESISSOULMUSIC_API FGenesisSoulMusicTuning
{
	GENERATED_BODY()

	/** Ersetzt die eingebaute Instrumentierung einzelner Lebensphasen. Nicht aufgeführte Phasen nutzen die Standardwerte. */
	UPROPERTY(EditAnywhere, Category = "Arrangement")
	TArray<FGenesisPhaseArrangement> ArrangementOverrides;

	/** Erinnerungen ab dieser Intensität bekommen ein musikalisches Fragment. */
	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "0", ClampMax = "1"))
	float FragmentIntensityThreshold = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "1"))
	int32 MaxFragmentsPerEntity = 64;

	/** Beim ungenauen Erinnern: Rate falscher Töne, ausgelassener Töne und Zeitversatz (je bei Genauigkeit 0). */
	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "0", ClampMax = "1"))
	float RecallWrongNoteRate = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "0", ClampMax = "1"))
	float RecallDropRate = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "0"))
	float RecallTimingJitterBeats = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Soundtrack", meta = (ClampMin = "8"))
	int32 MaxSoundtrackEntries = 256;

	/** Mutationswahrscheinlichkeit je Intervall bei der Vererbung von Familienmotiven. */
	UPROPERTY(EditAnywhere, Category = "Inheritance", meta = (ClampMin = "0", ClampMax = "1"))
	float InheritanceMutationChance = 0.15f;

	/** Anteil der Verschmelzung, der nach einer Trennung als Narbe im eigenen Motiv bleibt. */
	UPROPERTY(EditAnywhere, Category = "Relationships", meta = (ClampMin = "0", ClampMax = "1"))
	float ScarRetention = 0.35f;

	/** Ab dieser Verschmelzung wird die Bindung ein Soundtrack-Moment. */
	UPROPERTY(EditAnywhere, Category = "Relationships", meta = (ClampMin = "0", ClampMax = "1"))
	float FusionCueThreshold = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Death", meta = (ClampMin = "0", ClampMax = "12"))
	int32 DeathFragmentCount = 5;

	UPROPERTY(EditAnywhere, Category = "Death", meta = (ClampMin = "0"))
	float DeathSectionGapSeconds = 1.5f;
};

/** Persistenter Welt-Zustand der Seelenmusik. */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisSoulMusicWorldState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGenesisCharacterMotif> CharacterMotifs;

	UPROPERTY()
	TArray<FGenesisRelationshipTheme> Relationships;

	UPROPERTY()
	TArray<FGenesisMemoryMusicFragment> Fragments;

	UPROPERTY()
	TArray<FGenesisLifeSoundtrack> Soundtracks;
};

/** Musik eines abgeschlossenen Lebens – bleibt bei der Seele (Soundtrack-Export, Jenseits). */
USTRUCT()
struct GENESISSOULMUSIC_API FGenesisArchivedLifeMusic
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EntityId;

	UPROPERTY()
	int32 IncarnationIndex = INDEX_NONE;

	UPROPERTY()
	FGenesisLifeSoundtrack Soundtrack;

	/** Die Fragmente, die in der Todeskomposition erklangen. */
	UPROPERTY()
	TArray<FGenesisMemoryMusicFragment> KeyFragments;

	UPROPERTY()
	FGenesisSoulMotif SoulMotifAtDeath;
};

USTRUCT()
struct GENESISSOULMUSIC_API FGenesisSoulMusicArchiveState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGenesisArchivedLifeMusic> Lives;
};
