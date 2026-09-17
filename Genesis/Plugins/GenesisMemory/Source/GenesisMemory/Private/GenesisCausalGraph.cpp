// GENESIS: Der Kreislauf des Lebens

#include "GenesisCausalGraph.h"
#include "GenesisLog.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

namespace
{
	/** Kombiniert unabhängige Belege für dieselbe Kausalbeziehung. */
	float CombineEvidence(float A, float B)
	{
		return 1.0f - (1.0f - FMath::Clamp(A, 0.0f, 1.0f)) * (1.0f - FMath::Clamp(B, 0.0f, 1.0f));
	}

	struct FTraceNode
	{
		FGuid EventId;
		FGuid ViaEventId;
		int32 Depth = 0;
		float Strength = 0.0f;
	};
}

void FGenesisCausalGraph::Reset()
{
	Events.Reset();
	Links.Reset();
	NextSequence = 1;
	EventIndex.Reset();
	OutgoingLinks.Reset();
	IncomingLinks.Reset();
}

void FGenesisCausalGraph::RebuildIndices()
{
	EventIndex.Reset();
	OutgoingLinks.Reset();
	IncomingLinks.Reset();

	EventIndex.Reserve(Events.Num());
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		EventIndex.Add(Events[Index].EventId, Index);
		NextSequence = FMath::Max(NextSequence, Events[Index].Sequence + 1);
	}

	for (int32 Index = 0; Index < Links.Num(); ++Index)
	{
		OutgoingLinks.FindOrAdd(Links[Index].CauseId).Add(Index);
		IncomingLinks.FindOrAdd(Links[Index].EffectId).Add(Index);
	}
}

const FGenesisCausalEvent* FGenesisCausalGraph::RecordEvent(const FGenesisCausalEvent& Event)
{
	if (!Event.EventId.IsValid() || EventIndex.Contains(Event.EventId))
	{
		UE_LOG(LogGenesis, Error, TEXT("CausalGraph: Ereignis mit ungültiger oder doppelter ID %s abgelehnt."), *Event.EventId.ToString());
		return nullptr;
	}

	const int32 Index = Events.Add(Event);
	Events[Index].Sequence = NextSequence++;
	EventIndex.Add(Event.EventId, Index);
	return &Events[Index];
}

bool FGenesisCausalGraph::AddLink(const FGuid& CauseId, const FGuid& EffectId, EGenesisCausalLinkType Type, float Strength)
{
	const FGenesisCausalEvent* Cause = FindEvent(CauseId);
	const FGenesisCausalEvent* Effect = FindEvent(EffectId);
	if (!Cause || !Effect || Strength <= 0.0f)
	{
		return false;
	}

	// Azyklizität: Ursache muss vor der Wirkung existiert haben
	if (Cause->Sequence >= Effect->Sequence)
	{
		UE_LOG(LogGenesis, Warning, TEXT("CausalGraph: Kante %s → %s widerspricht der zeitlichen Ordnung und wird abgelehnt."),
			*CauseId.ToString(EGuidFormats::Short), *EffectId.ToString(EGuidFormats::Short));
		return false;
	}

	if (const TArray<int32>* Outgoing = OutgoingLinks.Find(CauseId))
	{
		for (int32 LinkIndex : *Outgoing)
		{
			FGenesisCausalLink& Existing = Links[LinkIndex];
			if (Existing.EffectId == EffectId)
			{
				Existing.Strength = CombineEvidence(Existing.Strength, Strength);
				// Direkte Verursachung ist die stärkere Aussage
				if (Type == EGenesisCausalLinkType::Direct)
				{
					Existing.Type = Type;
				}
				return true;
			}
		}
	}

	FGenesisCausalLink& Link = Links.AddDefaulted_GetRef();
	Link.CauseId = CauseId;
	Link.EffectId = EffectId;
	Link.Type = Type;
	Link.Strength = FMath::Clamp(Strength, 0.0f, 1.0f);

	const int32 LinkIndex = Links.Num() - 1;
	OutgoingLinks.FindOrAdd(CauseId).Add(LinkIndex);
	IncomingLinks.FindOrAdd(EffectId).Add(LinkIndex);
	return true;
}

const FGenesisCausalEvent* FGenesisCausalGraph::FindEvent(const FGuid& EventId) const
{
	const int32* Index = EventIndex.Find(EventId);
	return Index ? &Events[*Index] : nullptr;
}

bool FGenesisCausalGraph::SetResolved(const FGuid& EventId, bool bResolved)
{
	if (const int32* Index = EventIndex.Find(EventId))
	{
		Events[*Index].bResolved = bResolved;
		return true;
	}
	return false;
}

TArray<FGenesisCausalTraceEntry> FGenesisCausalGraph::TraceConsequences(const FGuid& RootId, int32 MaxDepth, float MinPathStrength) const
{
	return Trace(RootId, MaxDepth, MinPathStrength, /*bForward*/ true);
}

TArray<FGenesisCausalTraceEntry> FGenesisCausalGraph::TraceCauses(const FGuid& RootId, int32 MaxDepth, float MinPathStrength) const
{
	return Trace(RootId, MaxDepth, MinPathStrength, /*bForward*/ false);
}

TArray<FGenesisCausalTraceEntry> FGenesisCausalGraph::Trace(const FGuid& RootId, int32 MaxDepth, float MinPathStrength, bool bForward) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisCausalGraph_Trace);

	TArray<FGenesisCausalTraceEntry> Result;
	if (!FindEvent(RootId) || MaxDepth <= 0)
	{
		return Result;
	}

	// Stärkster Pfad zuerst (Produkte ≤ 1 sind monoton fallend → Dijkstra-artige Suche ist korrekt)
	auto HeapPredicate = [](const FTraceNode& A, const FTraceNode& B) { return A.Strength > B.Strength; };

	TArray<FTraceNode> Heap;
	Heap.HeapPush({ RootId, FGuid(), 0, 1.0f }, HeapPredicate);

	TSet<FGuid> Visited;
	const TMap<FGuid, TArray<int32>>& Adjacency = bForward ? OutgoingLinks : IncomingLinks;

	while (Heap.Num() > 0)
	{
		FTraceNode Node;
		Heap.HeapPop(Node, HeapPredicate, EAllowShrinking::No);

		if (Visited.Contains(Node.EventId))
		{
			continue;
		}
		Visited.Add(Node.EventId);

		if (Node.Depth > 0)
		{
			FGenesisCausalTraceEntry& Entry = Result.AddDefaulted_GetRef();
			Entry.EventId = Node.EventId;
			Entry.Depth = Node.Depth;
			Entry.PathStrength = Node.Strength;
			Entry.ViaEventId = Node.ViaEventId;
		}

		if (Node.Depth >= MaxDepth)
		{
			continue;
		}

		if (const TArray<int32>* LinkIndices = Adjacency.Find(Node.EventId))
		{
			for (int32 LinkIndex : *LinkIndices)
			{
				const FGenesisCausalLink& Link = Links[LinkIndex];
				const FGuid& Next = bForward ? Link.EffectId : Link.CauseId;
				const float Strength = Node.Strength * Link.Strength;
				if (Strength >= MinPathStrength && !Visited.Contains(Next))
				{
					Heap.HeapPush({ Next, Node.EventId, Node.Depth + 1, Strength }, HeapPredicate);
				}
			}
		}
	}

	return Result;
}

TArray<const FGenesisCausalEvent*> FGenesisCausalGraph::FindEvents(const FGenesisEventQuery& Query) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisCausalGraph_FindEvents);

	TArray<const FGenesisCausalEvent*> Result;

	// Rückwärts iterieren: neueste zuerst, frühes Abbrechen bei MaxResults
	for (int32 Index = Events.Num() - 1; Index >= 0; --Index)
	{
		const FGenesisCausalEvent& Event = Events[Index];

		if (Query.InvolvedEntityId.IsValid())
		{
			const bool bInvolved = Query.bInvolvedAsActorOnly ? Event.ActorId == Query.InvolvedEntityId : Event.Involves(Query.InvolvedEntityId);
			if (!bInvolved)
			{
				continue;
			}
		}
		if (Query.EventType.IsValid() && !Event.EventType.MatchesTag(Query.EventType))
		{
			continue;
		}
		if (Query.AnyThemes.Num() > 0 && !Event.Themes.HasAny(Query.AnyThemes))
		{
			continue;
		}
		if (Query.AnyViolatedValues.Num() > 0 && !Event.ViolatedValues.HasAny(Query.AnyViolatedValues))
		{
			continue;
		}
		if (Query.bUnresolvedOnly && Event.bResolved)
		{
			continue;
		}
		if (Event.Magnitude < Query.MinMagnitude)
		{
			continue;
		}
		if (Query.bUseTimeRange && (Event.Time < Query.From || Event.Time > Query.To))
		{
			continue;
		}

		Result.Add(&Event);
		if (Query.MaxResults > 0 && Result.Num() >= Query.MaxResults)
		{
			break;
		}
	}

	// Gleiche Zeitpunkte: spätere Einfügung zuerst – Rückwärtsiteration liefert das bereits,
	// abweichende Zeitstempel (nachträglich rekonstruierte Ereignisse) werden hier korrigiert.
	Result.StableSort([](const FGenesisCausalEvent& A, const FGenesisCausalEvent& B)
	{
		return A.Time > B.Time;
	});
	return Result;
}

int32 FGenesisCausalGraph::CountThemeRecurrence(const FGuid& ActorId, const FGameplayTag& Theme) const
{
	int32 Count = 0;
	for (const FGenesisCausalEvent& Event : Events)
	{
		if (Event.ActorId == ActorId && Event.Themes.HasTag(Theme))
		{
			Count += FMath::Max(1, Event.AggregatedCount);
		}
	}
	return Count;
}

int32 FGenesisCausalGraph::PruneEvents(TFunctionRef<bool(const FGenesisCausalEvent&)> ShouldRemove, float MinBridgedStrength)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GenesisCausalGraph_Prune);

	// Ereignisse liegen in Einfügereihenfolge vor → aufsteigende Sequence
	TArray<FGuid> RemovalOrder;
	TSet<FGuid> RemovalSet;
	for (const FGenesisCausalEvent& Event : Events)
	{
		if (ShouldRemove(Event))
		{
			RemovalOrder.Add(Event.EventId);
			RemovalSet.Add(Event.EventId);
		}
	}
	if (RemovalOrder.Num() == 0)
	{
		return 0;
	}

	// Arbeitskopie der Kanten als Adjazenz: Out[Ursache][Wirkung]
	TMap<FGuid, TMap<FGuid, FGenesisCausalLink>> Out;
	TMap<FGuid, TSet<FGuid>> In;
	for (const FGenesisCausalLink& Link : Links)
	{
		Out.FindOrAdd(Link.CauseId).Add(Link.EffectId, Link);
		In.FindOrAdd(Link.EffectId).Add(Link.CauseId);
	}

	for (const FGuid& Middle : RemovalOrder)
	{
		const TSet<FGuid> Causes = In.Contains(Middle) ? In[Middle] : TSet<FGuid>();
		const TMap<FGuid, FGenesisCausalLink> Effects = Out.Contains(Middle) ? Out[Middle] : TMap<FGuid, FGenesisCausalLink>();

		for (const FGuid& CauseId : Causes)
		{
			const FGenesisCausalLink* Incoming = Out.Find(CauseId) ? Out[CauseId].Find(Middle) : nullptr;
			if (!Incoming)
			{
				continue;
			}
			const FGenesisCausalLink IncomingCopy = *Incoming;

			for (const TPair<FGuid, FGenesisCausalLink>& EffectPair : Effects)
			{
				const float Bridged = IncomingCopy.Strength * EffectPair.Value.Strength;
				if (Bridged < MinBridgedStrength)
				{
					continue;
				}

				TMap<FGuid, FGenesisCausalLink>& CauseOut = Out.FindOrAdd(CauseId);
				if (FGenesisCausalLink* Existing = CauseOut.Find(EffectPair.Key))
				{
					Existing->Strength = CombineEvidence(Existing->Strength, Bridged);
				}
				else
				{
					FGenesisCausalLink BridgeLink;
					BridgeLink.CauseId = CauseId;
					BridgeLink.EffectId = EffectPair.Key;
					BridgeLink.Type = IncomingCopy.Type == EffectPair.Value.Type ? IncomingCopy.Type : EGenesisCausalLinkType::Contributing;
					BridgeLink.Strength = Bridged;
					CauseOut.Add(EffectPair.Key, BridgeLink);
					In.FindOrAdd(EffectPair.Key).Add(CauseId);
				}
			}
		}

		// Mitte aus der Adjazenz entfernen
		for (const FGuid& CauseId : Causes)
		{
			if (TMap<FGuid, FGenesisCausalLink>* CauseOut = Out.Find(CauseId))
			{
				CauseOut->Remove(Middle);
			}
		}
		for (const TPair<FGuid, FGenesisCausalLink>& EffectPair : Effects)
		{
			if (TSet<FGuid>* EffectIn = In.Find(EffectPair.Key))
			{
				EffectIn->Remove(Middle);
			}
		}
		Out.Remove(Middle);
		In.Remove(Middle);
	}

	// Ereignisse entfernen
	Events.RemoveAll([&RemovalSet](const FGenesisCausalEvent& Event)
	{
		return RemovalSet.Contains(Event.EventId);
	});

	// Kanten deterministisch neu aufbauen (Reihenfolge: Ursache, dann Wirkung nach Sequence)
	TMap<FGuid, int64> SequenceOf;
	for (const FGenesisCausalEvent& Event : Events)
	{
		SequenceOf.Add(Event.EventId, Event.Sequence);
	}

	Links.Reset();
	for (const TPair<FGuid, TMap<FGuid, FGenesisCausalLink>>& CausePair : Out)
	{
		for (const TPair<FGuid, FGenesisCausalLink>& EffectPair : CausePair.Value)
		{
			if (SequenceOf.Contains(CausePair.Key) && SequenceOf.Contains(EffectPair.Key))
			{
				Links.Add(EffectPair.Value);
			}
		}
	}
	Links.Sort([&SequenceOf](const FGenesisCausalLink& A, const FGenesisCausalLink& B)
	{
		const int64 CauseA = SequenceOf[A.CauseId];
		const int64 CauseB = SequenceOf[B.CauseId];
		return CauseA != CauseB ? CauseA < CauseB : SequenceOf[A.EffectId] < SequenceOf[B.EffectId];
	});

	RebuildIndices();
	return RemovalOrder.Num();
}
