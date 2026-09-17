// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisGameplayTags.h"
#include "GenesisMemoryWorld.h"
#include "GenesisPersistence.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisMemoryTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGuid Child(1, 0, 0, 1);
	const FGuid Parent(1, 0, 0, 2);
	const FGuid Partner(1, 0, 0, 3);
	const FGuid Witness(1, 0, 0, 4);

	FGenesisCausalEvent MakeEvent(FGenesisMemoryWorld& World, int64 Year, const FGuid& Actor, const FGameplayTag& Theme, float Magnitude = 0.5f)
	{
		FGenesisCausalEvent Event;
		Event.EventId = World.NewEventId();
		Event.Time = FGenesisTimestamp::FromCalendar(Year);
		Event.ActorId = Actor;
		Event.Themes.AddTag(Theme);
		Event.Magnitude = Magnitude;
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisCausalChainTest, "Genesis.Memory.Graph.CausalChainAcrossGenerations", GenesisMemoryTests::Flags)
bool FGenesisCausalChainTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMemoryTests;
	FGenesisMemoryWorld World;
	World.Reset(1);

	// Kind lügt → Eltern verlieren Vertrauen → Strategie → Bindungsangst → Partnerschaft → eigenes Kind übernimmt → Familienkonflikt
	const FGuid Lie = World.RecordEvent(MakeEvent(World, 1408, Child, GenesisTags::Theme_Deception))->EventId;
	const FGuid TrustLost = World.RecordEvent(MakeEvent(World, 1408, Parent, GenesisTags::Theme_Trust))->EventId;
	const FGuid Strategy = World.RecordEvent(MakeEvent(World, 1410, Child, GenesisTags::Theme_Deception, 0.1f))->EventId;
	const FGuid Attachment = World.RecordEvent(MakeEvent(World, 1416, Child, GenesisTags::Theme_Fear))->EventId;
	const FGuid Partnership = World.RecordEvent(MakeEvent(World, 1425, Child, GenesisTags::Theme_Love))->EventId;
	const FGuid Inherited = World.RecordEvent(MakeEvent(World, 1440, Partner, GenesisTags::Theme_Deception))->EventId;
	const FGuid Conflict = World.RecordEvent(MakeEvent(World, 1470, Partner, GenesisTags::Theme_Betrayal, 0.9f))->EventId;

	TestTrue(TEXT("Link 1"), World.AddLink(Lie, TrustLost, EGenesisCausalLinkType::Direct, 0.9f));
	TestTrue(TEXT("Link 2"), World.AddLink(TrustLost, Strategy, EGenesisCausalLinkType::Contributing, 0.7f));
	TestTrue(TEXT("Link 3"), World.AddLink(Strategy, Attachment, EGenesisCausalLinkType::Contributing, 0.6f));
	TestTrue(TEXT("Link 4"), World.AddLink(Attachment, Partnership, EGenesisCausalLinkType::Contributing, 0.6f));
	TestTrue(TEXT("Link 5"), World.AddLink(Partnership, Inherited, EGenesisCausalLinkType::Inherited, 0.8f));
	TestTrue(TEXT("Link 6"), World.AddLink(Inherited, Conflict, EGenesisCausalLinkType::Contributing, 0.9f));

	// Azyklizität: Wirkung kann nicht Ursache eines früheren Ereignisses sein
	AddExpectedMessagePlain(TEXT("widerspricht der zeitlichen Ordnung"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("Rückwärtskante abgelehnt"), World.AddLink(Conflict, Lie, EGenesisCausalLinkType::Direct, 1.0f));

	const TArray<FGenesisCausalTraceEntry> Consequences = World.GetGraph().TraceConsequences(Lie, 10, 0.01f);
	const FGenesisCausalTraceEntry* ConflictEntry = Consequences.FindByPredicate([&Conflict](const FGenesisCausalTraceEntry& Entry) { return Entry.EventId == Conflict; });
	if (!TestNotNull(TEXT("Lüge wirkt Jahrzehnte später"), ConflictEntry))
	{
		return false;
	}
	TestEqual(TEXT("Kettenlänge"), ConflictEntry->Depth, 6);
	TestTrue(TEXT("Pfadstärke = Produkt"), FMath::IsNearlyEqual(ConflictEntry->PathStrength, 0.9f * 0.7f * 0.6f * 0.6f * 0.8f * 0.9f, 1.0e-4f));

	const TArray<FGenesisCausalTraceEntry> Causes = World.GetGraph().TraceCauses(Conflict, 10, 0.01f);
	TestTrue(TEXT("Rückverfolgung bis zur Lüge"), Causes.ContainsByPredicate([&Lie](const FGenesisCausalTraceEntry& Entry) { return Entry.EventId == Lie; }));

	TestEqual(TEXT("Tiefenlimit"), World.GetGraph().TraceConsequences(Lie, 2, 0.0f).Num(), 2);

	// Verdichtung: unbedeutende "Strategie" entfernt, Kette bleibt über Brücke erhalten
	const int32 Removed = World.Compact(0.15f, 0.01f);
	TestEqual(TEXT("Ein Ereignis verdichtet"), Removed, 1);
	TestNull(TEXT("Strategie entfernt"), World.GetGraph().FindEvent(Strategy));
	const TArray<FGenesisCausalTraceEntry> AfterCompaction = World.GetGraph().TraceConsequences(Lie, 10, 0.01f);
	const FGenesisCausalTraceEntry* ConflictAfter = AfterCompaction.FindByPredicate([&Conflict](const FGenesisCausalTraceEntry& Entry) { return Entry.EventId == Conflict; });
	if (TestNotNull(TEXT("Kette nach Verdichtung intakt"), ConflictAfter))
	{
		TestTrue(TEXT("Stärke bleibt erhalten"), FMath::IsNearlyEqual(ConflictAfter->PathStrength, ConflictEntry->PathStrength, 1.0e-4f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisCausalQueryTest, "Genesis.Memory.Graph.QueriesAndEvidence", GenesisMemoryTests::Flags)
bool FGenesisCausalQueryTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMemoryTests;
	FGenesisMemoryWorld World;
	World.Reset(2);

	FGenesisCausalEvent Conflict = MakeEvent(World, 1500, Parent, GenesisTags::Theme_Betrayal);
	Conflict.bResolved = false;
	Conflict.TargetIds.Add(Child);
	const FGuid ConflictId = World.RecordEvent(Conflict)->EventId;
	const FGuid LieA = World.RecordEvent(MakeEvent(World, 1501, Child, GenesisTags::Theme_Deception))->EventId;
	World.RecordEvent(MakeEvent(World, 1502, Child, GenesisTags::Theme_Deception));

	FGenesisEventQuery Query;
	Query.InvolvedEntityId = Child;
	TestEqual(TEXT("Beteiligung inkl. Ziel"), World.GetGraph().FindEvents(Query).Num(), 3);
	Query.bInvolvedAsActorOnly = true;
	TestEqual(TEXT("Nur als Handelnder"), World.GetGraph().FindEvents(Query).Num(), 2);

	FGenesisEventQuery Open;
	Open.bUnresolvedOnly = true;
	TestEqual(TEXT("Offene Fäden"), World.GetGraph().FindEvents(Open).Num(), 1);

	TestEqual(TEXT("Wiederholung eines Themas"), World.GetGraph().CountThemeRecurrence(Child, GenesisTags::Theme_Deception), 2);

	// Mehrfache Belege kombinieren sich, statt Kanten zu duplizieren
	World.AddLink(ConflictId, LieA, EGenesisCausalLinkType::Contributing, 0.5f);
	World.AddLink(ConflictId, LieA, EGenesisCausalLinkType::Contributing, 0.5f);
	TestEqual(TEXT("Keine doppelte Kante"), World.GetGraph().NumLinks(), 1);
	TestTrue(TEXT("Belege kombiniert"), FMath::IsNearlyEqual(World.GetGraph().GetLinks()[0].Strength, 0.75f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSubjectiveMemoryTest, "Genesis.Memory.Traces.SubjectiveAndFallible", GenesisMemoryTests::Flags)
bool FGenesisSubjectiveMemoryTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMemoryTests;
	FGenesisMemoryWorld World;
	World.Reset(3);
	const FGenesisTimestamp Now = FGenesisTimestamp::FromCalendar(1600);

	FGenesisCausalEvent Event = MakeEvent(World, 1600, Parent, GenesisTags::Theme_Care, 0.8f);
	Event.Themes.AddTag(GenesisTags::Theme_Loss);
	Event.TargetIds.Add(Child);
	Event.WitnessIds.Add(Witness);
	Event.Valence = 0.6f;
	const FGuid EventId = World.RecordEvent(Event)->EventId;

	// Zwei Personen speichern dasselbe Ereignis unterschiedlich
	FGenesisEncodingContext ChildContext;
	ChildContext.Perspective = EGenesisMemoryPerspective::Target;
	ChildContext.Arousal = 0.9f;
	ChildContext.SensoryCues.AddTag(GenesisTags::Sense_Smell);

	FGenesisEncodingContext WitnessContext;
	WitnessContext.Perspective = EGenesisMemoryPerspective::Witness;
	WitnessContext.Arousal = 0.2f;
	WitnessContext.Attention = 0.4f;
	WitnessContext.bHasPerceptionOverride = true; // Missverständnis: Zeuge hält es für Verrat durch das Kind
	WitnessContext.PerceivedThemesOverride.AddTag(GenesisTags::Theme_Betrayal);
	WitnessContext.PerceivedActorOverride = Child;

	const FGuid ChildTrace = World.EncodeMemory(Child, EventId, ChildContext, Now);
	const FGuid WitnessTrace = World.EncodeMemory(Witness, EventId, WitnessContext, Now);

	const FGenesisMemoryTrace* ChildMemory = World.FindStore(Child)->FindTrace(ChildTrace);
	const FGenesisMemoryTrace* WitnessMemory = World.FindStore(Witness)->FindTrace(WitnessTrace);
	if (!TestNotNull(TEXT("Kind erinnert"), ChildMemory) || !TestNotNull(TEXT("Zeuge erinnert"), WitnessMemory))
	{
		return false;
	}

	TestTrue(TEXT("Betroffene Person erinnert intensiver"), ChildMemory->Intensity > WitnessMemory->Intensity);
	TestTrue(TEXT("Zeuge verzerrt"), WitnessMemory->bDistorted);
	TestTrue(TEXT("Wahrheit: Zeuge weit daneben"), GenesisMemoryLogic::CompareWithTruth(*WitnessMemory, *World.GetGraph().FindEvent(EventId)) < 0.3f);

	// Zeit vergeht: beiläufige Erinnerung verblasst, emotionale bleibt (ruhend)
	const float ChildIntensityBefore = ChildMemory->Intensity;
	const float ChildAccuracyBefore = ChildMemory->Accuracy;
	FGenesisMemoryDynamicsParams Params;
	World.DecayAll(60.0, Params);
	const FGenesisMemoryStore* WitnessStore = World.FindStore(Witness);
	TestNull(TEXT("Zeuge hat vergessen"), WitnessStore->FindTrace(WitnessTrace));
	const FGenesisMemoryTrace* ChildAfter = World.FindStore(Child)->FindTrace(ChildTrace);
	if (!TestNotNull(TEXT("Emotionale Erinnerung bleibt"), ChildAfter))
	{
		return false;
	}
	TestTrue(TEXT("aber schwächer"), ChildAfter->Intensity < ChildIntensityBefore);
	TestTrue(TEXT("und ungenauer"), ChildAfter->Accuracy < ChildAccuracyBefore);

	// Ein Geruch Jahrzehnte später weckt die Erinnerung
	FGenesisMemoryStore* ChildStore = World.FindStoreMutable(Child);
	GenesisMemoryLogic::Recall(*ChildStore, ChildTrace, Now + FGenesisTimestamp::YearsToSeconds(60), 0.0f, Params);
	const TArray<FGenesisRecallCandidate> Recalled = GenesisMemoryLogic::FindByCues(*ChildStore, FGameplayTagContainer(GenesisTags::Sense_Smell), FGameplayTagContainer(), 3, 0.0f, Params);
	TestEqual(TEXT("Geruch weckt Erinnerung"), Recalled.Num(), 1);

	// Verdrängung dämpft den Zugang
	const float ScoreBefore = Recalled.Num() > 0 ? Recalled[0].Score : 0.0f;
	GenesisMemoryLogic::AdjustRepression(*ChildStore, ChildTrace, 1.0f);
	const TArray<FGenesisRecallCandidate> Repressed = GenesisMemoryLogic::FindByCues(*ChildStore, FGameplayTagContainer(GenesisTags::Sense_Smell), FGameplayTagContainer(), 3, 0.0f, Params);
	TestTrue(TEXT("Verdrängte Erinnerung schwerer zugänglich"), Repressed.Num() == 1 && Repressed[0].Score < ScoreBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMemoryPersistenceTest, "Genesis.Memory.Persistence", GenesisMemoryTests::Flags)
bool FGenesisMemoryPersistenceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMemoryTests;
	FGenesisMemoryWorld World;
	World.Reset(4);

	const FGuid A = World.RecordEvent(MakeEvent(World, 1700, Child, GenesisTags::Theme_Courage))->EventId;
	const FGuid B = World.RecordEvent(MakeEvent(World, 1701, Parent, GenesisTags::Theme_Forgiveness))->EventId;
	World.AddLink(A, B, EGenesisCausalLinkType::Direct, 0.8f);
	World.EncodeMemory(Parent, B, FGenesisEncodingContext(), FGenesisTimestamp::FromCalendar(1701));

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben"), GenesisPersistence::Write(World, Bytes));
	FGenesisMemoryWorld Loaded;
	TestTrue(TEXT("Lesen"), GenesisPersistence::Read(Loaded, Bytes));
	Loaded.RebuildIndices();

	TestEqual(TEXT("Ereignisse"), Loaded.GetGraph().NumEvents(), 2);
	TestEqual(TEXT("Kette"), Loaded.GetGraph().TraceConsequences(A, 3, 0.0f).Num(), 1);
	TestNotNull(TEXT("Gedächtnis"), Loaded.FindStore(Parent));

	// Nach dem Laden dürfen neue Ereignisse nicht mit alten Sequenzen kollidieren
	const FGenesisCausalEvent* C = Loaded.RecordEvent(MakeEvent(Loaded, 1702, Child, GenesisTags::Theme_Love));
	TestTrue(TEXT("Neue Sequenz nach geladenen"), C && C->Sequence > Loaded.GetGraph().FindEvent(B)->Sequence);
	TestTrue(TEXT("Neue Kante zum geladenen Ereignis"), C && Loaded.AddLink(B, C->EventId, EGenesisCausalLinkType::Contributing, 0.5f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
