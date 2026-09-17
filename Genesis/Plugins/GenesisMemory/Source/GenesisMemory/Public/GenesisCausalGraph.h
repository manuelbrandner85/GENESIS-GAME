// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisTypes.h"
#include "GenesisCausalGraph.generated.h"

/** Art einer Ursache-Wirkungs-Beziehung. */
UENUM(BlueprintType)
enum class EGenesisCausalLinkType : uint8
{
	/** Ursache hat die Wirkung unmittelbar ausgelöst. */
	Direct,
	/** Ursache hat zur Wirkung beigetragen (erlerntes Verhalten, Stimmung, Einfluss). */
	Contributing,
	/** Ursache hat die Wirkung erst möglich gemacht. */
	Enabling,
	/** Ursache hat die Wirkung erschwert. */
	Inhibiting,
	/** Über Generationen weitergegeben (Familie, Erziehung, Epigenetik). */
	Inherited,
	/** Thematisches Echo über Inkarnationen. */
	SoulEcho
};

/**
 * Objektives Ereignis – was tatsächlich geschah.
 * Subjektive Erinnerungen verweisen hierauf und können davon abweichen.
 * Im Jenseits kann die tatsächliche Ereignisversion sichtbar werden.
 */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisCausalEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid EventId;

	/** Streng monoton steigende Einfügereihenfolge. Garantiert einen azyklischen Graphen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	int64 Sequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGenesisTimestamp Time;

	/** Art des Ereignisses, z. B. Genesis.Action.Lie oder Genesis.Consequence.* (Content-Tags). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTag EventType;

	/** Lebensthemen (Genesis.Theme.*). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTagContainer Themes;

	/** Werte, die die Handlung ausdrückte bzw. verletzte (Doppelmoral-Erkennung, Ruf). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTagContainer ExpressedValues;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGameplayTagContainer ViolatedValues;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid ActorId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	TArray<FGuid> TargetIds;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	TArray<FGuid> WitnessIds;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid LocationId;

	/** Seele des Handelnden (für Echo-Abfragen über Leben hinweg). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid ActorSoulId;

	/** Objektive Tragweite 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	float Magnitude = 0.3f;

	/** Emotionale Grundfärbung −1..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	float Valence = 0.0f;

	/** false = offener Faden (ungelöster Konflikt, unausgesprochenes …). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	bool bResolved = true;

	/** Zusätzliche Kennwerte (z. B. Genesis.SimSystem.* → Einflussstärke). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	TArray<FGenesisWeightedTag> Facts;

	/** Durch Verdichtung zusammengefasst? */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	int32 AggregatedCount = 1;

	bool Involves(const FGuid& EntityId) const
	{
		return ActorId == EntityId || TargetIds.Contains(EntityId) || WitnessIds.Contains(EntityId);
	}
};

/** Gerichtete Kante Ursache → Wirkung. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisCausalLink
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid CauseId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid EffectId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	EGenesisCausalLinkType Type = EGenesisCausalLinkType::Contributing;

	/** 0..1 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	float Strength = 0.5f;
};

/** Ergebnis einer Kettenverfolgung. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisCausalTraceEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid EventId;

	/** Schritte vom Startereignis. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	int32 Depth = 0;

	/** Produkt der Kantenstärken entlang des stärksten Pfades. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	float PathStrength = 0.0f;

	/** Vorgänger auf dem stärksten Pfad (für die Darstellung der Kette). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Memory")
	FGuid ViaEventId;
};

/** Filter für Ereignissuche. Leere Felder filtern nicht. */
USTRUCT(BlueprintType)
struct GENESISMEMORY_API FGenesisEventQuery
{
	GENERATED_BODY()

	/** Person war Handelnder, Ziel oder Zeuge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGuid InvolvedEntityId;

	/** Nur Ereignisse, in denen die Person der Handelnde war. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	bool bInvolvedAsActorOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGameplayTag EventType;

	/** Mindestens eines dieser Themen (hierarchisch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGameplayTagContainer AnyThemes;

	/** Mindestens einer dieser verletzten Werte (hierarchisch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGameplayTagContainer AnyViolatedValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	bool bUnresolvedOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	float MinMagnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	bool bUseTimeRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGenesisTimestamp From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	FGenesisTimestamp To;

	/** 0 = unbegrenzt. Ergebnisse sind nach Zeit absteigend sortiert (neueste zuerst). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Memory")
	int32 MaxResults = 0;
};

/**
 * Causal Memory Graph – Ursache-Wirkungs-Graph aller bedeutsamen Ereignisse einer Welt.
 *
 * Invarianten:
 * - Eine Kante zeigt immer von einem früher eingefügten auf ein später eingefügtes Ereignis → azyklisch.
 * - Ereignisse werden nie ohne Übertrag ihrer Kausalketten entfernt (siehe PruneEvents).
 */
USTRUCT()
struct GENESISMEMORY_API FGenesisCausalGraph
{
	GENERATED_BODY()

	/** Nimmt ein Ereignis auf. EventId muss gültig und neu sein. Sequence wird vergeben. */
	const FGenesisCausalEvent* RecordEvent(const FGenesisCausalEvent& Event);

	/**
	 * Verknüpft Ursache und Wirkung. Existiert die Kante bereits, werden die Belege kombiniert
	 * (Stärke = 1 − (1 − a)(1 − b)). Abgelehnt, wenn die Ursache nicht vor der Wirkung eingefügt wurde.
	 */
	bool AddLink(const FGuid& CauseId, const FGuid& EffectId, EGenesisCausalLinkType Type, float Strength);

	const FGenesisCausalEvent* FindEvent(const FGuid& EventId) const;
	bool SetResolved(const FGuid& EventId, bool bResolved);

	/** Folgen eines Ereignisses (vorwärts), stärkster Pfad je Ereignis. */
	TArray<FGenesisCausalTraceEntry> TraceConsequences(const FGuid& RootId, int32 MaxDepth, float MinPathStrength) const;

	/** Ursachen eines Ereignisses (rückwärts). */
	TArray<FGenesisCausalTraceEntry> TraceCauses(const FGuid& RootId, int32 MaxDepth, float MinPathStrength) const;

	TArray<const FGenesisCausalEvent*> FindEvents(const FGenesisEventQuery& Query) const;

	/** Wie oft eine Person als Handelnde ein Thema wiederholt hat (Karma-Gericht: Wiederholung). */
	int32 CountThemeRecurrence(const FGuid& ActorId, const FGameplayTag& Theme) const;

	/**
	 * Entfernt Ereignisse und erhält ihre Kausalketten: Für jede entfernte Mitte x wird aus A → x → B die Kante A → B
	 * mit Stärke s(A,x)·s(x,B), sofern ≥ MinBridgedStrength.
	 * @return Anzahl entfernter Ereignisse
	 */
	int32 PruneEvents(TFunctionRef<bool(const FGenesisCausalEvent&)> ShouldRemove, float MinBridgedStrength);

	/** Muss nach dem Laden aufgerufen werden. */
	void RebuildIndices();

	int32 NumEvents() const { return Events.Num(); }
	int32 NumLinks() const { return Links.Num(); }
	const TArray<FGenesisCausalEvent>& GetEvents() const { return Events; }
	const TArray<FGenesisCausalLink>& GetLinks() const { return Links; }

	void Reset();

private:
	TArray<FGenesisCausalTraceEntry> Trace(const FGuid& RootId, int32 MaxDepth, float MinPathStrength, bool bForward) const;

	UPROPERTY()
	TArray<FGenesisCausalEvent> Events;

	UPROPERTY()
	TArray<FGenesisCausalLink> Links;

	UPROPERTY()
	int64 NextSequence = 1;

	// --- Laufzeit-Indizes (nicht gespeichert) ---
	TMap<FGuid, int32> EventIndex;
	TMap<FGuid, TArray<int32>> OutgoingLinks;
	TMap<FGuid, TArray<int32>> IncomingLinks;
};
