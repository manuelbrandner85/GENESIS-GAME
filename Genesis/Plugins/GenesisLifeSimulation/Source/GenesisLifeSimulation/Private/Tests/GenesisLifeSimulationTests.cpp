// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisGameplayTags.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisGenomePool.h"
#include "GenesisLifeSimulationEngine.h"
#include "GenesisLifeSimulationProcessors.h"
#include "GenesisMemoryWorld.h"
#include "GenesisPersistence.h"
#include "GenesisWorldClockSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisLifeTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGuid Child(2, 0, 0, 1);
	const FGuid Mother(2, 0, 0, 2);
	const FGuid Sibling(2, 0, 0, 3);
	const FGuid Friend(2, 0, 0, 4);

	/** Testumgebung mit eigenem Gedächtnis und Genom-Pool – keine GameInstance nötig. */
	struct FWorld
	{
		FGenesisMemoryWorld Memory;
		FGenesisGenomePool Genomes;
		TArray<FGenesisTraitDefinition> Traits;
		UGenesisLifeSimulationEngine* Engine = nullptr;
		FGenesisTimestamp Now = FGenesisTimestamp::FromCalendar(1500);

		explicit FWorld(const FGenesisLifeSimulationTuning& Tuning = FGenesisLifeSimulationTuning(), uint64 Seed = 1)
		{
			Memory.Reset(Seed);
			Genomes.Reset(Seed);
			Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
			Engine = NewObject<UGenesisLifeSimulationEngine>();
			Engine->Initialize(&Memory, &Genomes, &Traits, Tuning);
			Engine->AddDefaultProcessors();
			Engine->ResetState(Seed);
		}

		void Register(const FGuid& Id, EGenesisSimulationLevel Level = EGenesisSimulationLevel::Full)
		{
			FGenesisLifeProfile Profile;
			Profile.EntityId = Id;
			Profile.SimulationLevel = Level;
			Profile.GenomeId = Genomes.CreateFounder(Traits);
			Engine->RegisterEntity(Profile);
		}

		void Advance(double Years, bool bTimeSkip = true)
		{
			FGenesisSimulationStep Step;
			Step.From = Now;
			Step.To = Now + FGenesisTimestamp::YearsToSeconds(Years);
			Step.bIsTimeSkip = bTimeSkip;
			Now = Step.To;
			Engine->AdvanceTime(Step);
		}

		FGenesisLifeProfile& Profile(const FGuid& Id) { return *Engine->GetState().FindProfile(Id); }
	};

	UGenesisLifeActionDefinition* MakeLie()
	{
		UGenesisLifeActionDefinition* Lie = NewObject<UGenesisLifeActionDefinition>();
		Lie->ActionType = GenesisTags::Theme_Deception;
		Lie->Themes.AddTag(GenesisTags::Theme_Deception);
		Lie->BaseMagnitude = 0.5f;
		Lie->Valence = -0.5f;
		Lie->KarmaImpulse.Honesty = -8.0f;
		Lie->TrustImpact = -0.6f;
		Lie->SelfBenefit = 0.5f;
		Lie->OthersBenefit = -0.2f;
		Lie->Visibility = 1.0f;
		Lie->Ambiguity = 0.0f;
		Lie->StressLoad = 0.4f;
		Lie->ViolatesValues.AddTag(GenesisTags::Theme_Honesty);
		return Lie;
	}

	/** Missverständnisse ausschalten, damit Tests eindeutige Erwartungen prüfen können. */
	FGenesisLifeSimulationTuning ClearPerceptionTuning()
	{
		FGenesisLifeSimulationTuning Tuning;
		Tuning.BaseMisreadChance = 0.0f;
		Tuning.CulturalDistanceMisreadWeight = 0.0f;
		return Tuning;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisKarmaTest, "Genesis.Life.Karma.SaturationAndHabit", GenesisLifeTests::Flags)
bool FGenesisKarmaTest::RunTest(const FString& Parameters)
{
	FGenesisKarmaTuning Tuning;
	FGenesisKarmaVector Impulse;
	Impulse.Compassion = 10.0f;

	FGenesisKarmaProfile Profile;
	Profile.ApplyImpulse(Impulse, Tuning);
	const float FirstStep = Profile.Values.Compassion;
	TestTrue(TEXT("Erster Impuls wirkt"), FirstStep > 9.0f && FirstStep <= 10.0f);

	// Gewohnheit verstärkt gleichgerichtete Handlungen – Vergleich mit identischem Profil ohne Gewohnheit
	FGenesisKarmaProfile WithoutHabit = Profile;
	WithoutHabit.Habit = FGenesisKarmaVector();
	Profile.ApplyImpulse(Impulse, Tuning);
	WithoutHabit.ApplyImpulse(Impulse, Tuning);
	TestTrue(TEXT("Gewohnheit verstärkt"), Profile.Values.Compassion > WithoutHabit.Values.Compassion);
	TestTrue(TEXT("Sättigung bremst trotzdem"), Profile.Values.Compassion - FirstStep < FirstStep);

	// Viele Handlungen: Werte nähern sich dem Extrem, überschreiten es nie
	for (int32 Index = 0; Index < 500; ++Index)
	{
		Profile.ApplyImpulse(Impulse, Tuning);
	}
	TestTrue(TEXT("Nie über +100"), Profile.Values.Compassion <= 100.0f);
	TestTrue(TEXT("Extrem nur durch viele Handlungen"), Profile.Values.Compassion > 80.0f);

	// Ein einzelner Gegenimpuls kippt ein gefestigtes Profil nicht
	FGenesisKarmaVector Cruelty;
	Cruelty.Compassion = -10.0f;
	Profile.ApplyImpulse(Cruelty, Tuning);
	TestTrue(TEXT("Gefestigter Charakter"), Profile.Values.Compassion > 70.0f);
	TestEqual(TEXT("Andere Dimensionen unberührt"), Profile.Values.Honesty, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisPipelineTest, "Genesis.Life.Pipeline.AllFourteenSystems", GenesisLifeTests::Flags)
bool FGenesisPipelineTest::RunTest(const FString& Parameters)
{
	GenesisLifeTests::FWorld World;
	const TArray<TObjectPtr<UGenesisLifeSimulationProcessor>>& Processors = World.Engine->GetProcessors();
	TestEqual(TEXT("14 Systeme"), Processors.Num(), 14);

	TSet<FGameplayTag> Tags;
	for (int32 Index = 0; Index < Processors.Num(); ++Index)
	{
		Tags.Add(Processors[Index]->GetSystemTag());
		if (Index > 0)
		{
			const bool bOrdered = Processors[Index - 1]->GetStage() < Processors[Index]->GetStage()
				|| (Processors[Index - 1]->GetStage() == Processors[Index]->GetStage() && Processors[Index - 1]->GetOrder() <= Processors[Index]->GetOrder());
			TestTrue(FString::Printf(TEXT("Reihenfolge an Position %d"), Index), bOrdered);
		}
	}
	TestEqual(TEXT("14 unterschiedliche Systeme"), Tags.Num(), 14);
	TestTrue(TEXT("Konsequenz-Netzwerk läuft zuletzt"), Processors.Last()->GetSystemTag() == GenesisTags::SimSystem_ConsequenceNetwork);

	AddExpectedMessagePlain(TEXT("ist bereits registriert"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	World.Engine->AddProcessor(NewObject<UGenesisProcessor_Karma>());
	TestEqual(TEXT("Keine doppelten Systeme"), World.Engine->GetProcessors().Num(), 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisLieConsequenceTest, "Genesis.Life.Consequences.LieAcrossTime", GenesisLifeTests::Flags)
bool FGenesisLieConsequenceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World(ClearPerceptionTuning());
	World.Register(Child);
	World.Register(Mother);
	World.Register(Sibling, EGenesisSimulationLevel::Reduced);

	FGenesisTrustEdge& Trust = World.Engine->GetState().FindOrAddTrust(Mother, Child);
	Trust.Trust = 0.7f;
	Trust.Familiarity = 0.9f;

	UGenesisLifeActionDefinition* Lie = MakeLie();
	FGenesisDelayedConsequenceSpec Distrust;
	Distrust.ConsequenceType = GenesisTags::Theme_Trust;
	Distrust.Themes.AddTag(GenesisTags::Theme_Trust);
	Distrust.Probability = 1.0f;
	Distrust.MinDelayYears = 2.0f;
	Distrust.MaxDelayYears = 3.0f;
	Distrust.bOpensThread = true;
	Lie->DelayedConsequences.Add(Distrust);

	FGenesisLifeAction Action;
	Action.Definition = Lie;
	Action.ActorId = Child;
	Action.TargetIds.Add(Mother);
	Action.WitnessIds.Add(Sibling);

	FGenesisLifeSimulationFrame Frame;
	const FGuid LieEvent = World.Engine->ProcessAction(Action, World.Now, &Frame);

	// Karma (verborgen), Egoismus, Belastung
	TestTrue(TEXT("Ehrlichkeit sinkt"), World.Profile(Child).Karma.Values.Honesty < 0.0f);
	TestTrue(TEXT("Egoismus-Tendenz"), World.Profile(Child).Altruism < 0.0f);
	TestTrue(TEXT("Belastung"), World.Profile(Child).Stress > 0.0f);

	// Vertrauen: Verrat aus hohem Vertrauen
	const FGenesisTrustEdge* MotherTrust = World.Engine->GetState().FindTrust(Mother, Child);
	if (TestNotNull(TEXT("Vertrauenskante"), MotherTrust))
	{
		TestTrue(TEXT("Vertrauen deutlich gesunken"), MotherTrust->Trust < 0.5f);
		TestEqual(TEXT("Als Verrat gezählt"), MotherTrust->Betrayals, 1);
	}

	// Ruf beim Zeugen
	TestTrue(TEXT("Ruf 'Ehrlichkeit' beim Geschwister gesunken"),
		World.Engine->GetState().GetReputation(Child, Sibling, GenesisTags::Theme_Honesty) < 0.0f);

	// Kausalgraph + subjektive Erinnerungen (Zeuge ist Level 2, Ereignis bedeutsam genug)
	const FGenesisCausalEvent* Event = World.Memory.GetGraph().FindEvent(LieEvent);
	if (TestNotNull(TEXT("Ereignis im Kausalgraph"), Event))
	{
		TestTrue(TEXT("Verletzter Wert gespeichert"), Event->ViolatedValues.HasTag(GenesisTags::Theme_Honesty));
	}
	TestNotNull(TEXT("Kind erinnert sich"), World.Memory.FindStore(Child));
	TestNotNull(TEXT("Mutter erinnert sich"), World.Memory.FindStore(Mother));
	TestNotNull(TEXT("Geschwister erinnert sich"), World.Memory.FindStore(Sibling));
	TestEqual(TEXT("Folge eingeplant"), Frame.ScheduledConsequences, 1);

	// Jahre später: Konsequenz tritt ein und ist mit der Lüge verknüpft
	int32 Fired = 0;
	World.Engine->OnConsequenceTriggered.AddLambda([&Fired](const FGenesisScheduledConsequence&, const FGuid&) { ++Fired; });
	World.Advance(1.0);
	TestEqual(TEXT("Nach 1 Jahr noch nicht"), Fired, 0);
	World.Advance(3.0);
	TestEqual(TEXT("Nach 4 Jahren eingetreten"), Fired, 1);
	TestEqual(TEXT("Keine offenen Planungen"), World.Engine->GetState().PendingConsequences.Num(), 0);

	const TArray<FGenesisCausalTraceEntry> Consequences = World.Memory.GetGraph().TraceConsequences(LieEvent, 3, 0.01f);
	TestTrue(TEXT("Folge im Graph mit der Lüge verbunden"), Consequences.Num() >= 1);

	// Erlernte Muster: eine spätere Lüge desselben Kindes wird automatisch mit der früheren verknüpft
	const FGuid SecondLie = World.Engine->ProcessAction(Action, World.Now);
	const TArray<FGenesisCausalTraceEntry> Causes = World.Memory.GetGraph().TraceCauses(SecondLie, 1, 0.0f);
	TestTrue(TEXT("Wiederholung mit Vergangenheit verknüpft"),
		Causes.ContainsByPredicate([&LieEvent](const FGenesisCausalTraceEntry& Entry) { return Entry.EventId == LieEvent; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMisunderstandingTest, "Genesis.Life.Misunderstanding.SubjectivePerception", GenesisLifeTests::Flags)
bool FGenesisMisunderstandingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FGenesisLifeSimulationTuning Tuning;
	Tuning.BaseMisreadChance = 0.5f;
	FWorld World(Tuning, 77);
	World.Register(Child);

	// Hilfe, die leicht missverstanden wird, vor 20 Fremden
	UGenesisLifeActionDefinition* Help = NewObject<UGenesisLifeActionDefinition>();
	Help->ActionType = GenesisTags::Theme_Care;
	Help->Themes.AddTag(GenesisTags::Theme_Care);
	Help->Themes.AddTag(GenesisTags::Theme_Courage);
	Help->OthersBenefit = 0.8f;
	Help->TrustImpact = 0.5f;
	Help->Ambiguity = 0.6f;
	Help->Visibility = 1.0f;
	Help->BaseMagnitude = 0.6f;
	Help->KarmaImpulse.Compassion = 6.0f;

	FGenesisLifeAction Action;
	Action.Definition = Help;
	Action.ActorId = Child;
	for (int32 Index = 0; Index < 20; ++Index)
	{
		const FGuid Stranger(3, 0, 0, Index + 1);
		World.Register(Stranger);
		Action.WitnessIds.Add(Stranger);
	}

	FGenesisLifeSimulationFrame Frame;
	const FGuid EventId = World.Engine->ProcessAction(Action, World.Now, &Frame);

	int32 Misread = 0;
	int32 Understood = 0;
	for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		Misread += Perception.bMisread ? 1 : 0;
		Understood += (Perception.bNoticed && !Perception.bMisread) ? 1 : 0;
	}
	TestTrue(TEXT("Manche missverstehen"), Misread > 0);
	TestTrue(TEXT("Manche verstehen richtig"), Understood > 0);

	// Karma folgt der Absicht, nicht der Wahrnehmung
	TestTrue(TEXT("Mitgefühl steigt trotz Missverständnissen"), World.Profile(Child).Karma.Values.Compassion > 0.0f);

	// Wer missversteht, speichert eine verzerrte Erinnerung
	bool bFoundDistorted = false;
	for (const FGenesisObserverPerception& Perception : Frame.Perceptions)
	{
		if (Perception.bMisread && Perception.PerceivedThemes.Num() < Help->Themes.Num())
		{
			const FGenesisMemoryStore* Store = World.Memory.FindStore(Perception.ObserverId);
			const FGenesisMemoryTrace* Trace = Store ? Store->FindTraceForEvent(EventId) : nullptr;
			bFoundDistorted |= Trace && Trace->bDistorted;
		}
	}
	TestTrue(TEXT("Verzerrte Erinnerung gespeichert"), bFoundDistorted);

	// Determinismus: gleiche Welt, gleiche Wahrnehmungen
	FWorld Replay(Tuning, 77);
	Replay.Register(Child);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		Replay.Register(FGuid(3, 0, 0, Index + 1));
	}
	FGenesisLifeSimulationFrame ReplayFrame;
	Replay.Engine->ProcessAction(Action, Replay.Now, &ReplayFrame);
	bool bSame = ReplayFrame.Perceptions.Num() == Frame.Perceptions.Num();
	for (int32 Index = 0; bSame && Index < Frame.Perceptions.Num(); ++Index)
	{
		bSame &= ReplayFrame.Perceptions[Index].bMisread == Frame.Perceptions[Index].bMisread;
	}
	TestTrue(TEXT("Deterministisch reproduzierbar"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSocialSystemsTest, "Genesis.Life.Social.GroupPressureAndDoubleStandard", GenesisLifeTests::Flags)
bool FGenesisSocialSystemsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World(ClearPerceptionTuning());
	World.Register(Child);
	World.Register(Mother);
	World.Register(Sibling);

	// Gruppenzwang: Ehrlich sein, obwohl die Clique Lügen erwartet
	UGenesisLifeActionDefinition* Truth = NewObject<UGenesisLifeActionDefinition>();
	Truth->ActionType = GenesisTags::Theme_Honesty;
	Truth->Themes.AddTag(GenesisTags::Theme_Honesty);
	Truth->ExpressesValues.AddTag(GenesisTags::Theme_Honesty);
	Truth->Visibility = 1.0f;
	Truth->Ambiguity = 0.0f;
	Truth->BaseMagnitude = 0.4f;

	FGenesisGroupContext Clique;
	Clique.Cohesion = 1.0f;
	Clique.PresentMemberIds = { Mother, Sibling, FGuid(9, 9, 9, 9) };
	Clique.ExpectedValues.AddTag(GenesisTags::Theme_Deception);
	Clique.RejectedValues.AddTag(GenesisTags::Theme_Honesty);

	FGenesisLifeAction Defy;
	Defy.Definition = Truth;
	Defy.ActorId = Child;
	Defy.WitnessIds = { Mother, Sibling };
	Defy.Groups.Add(Clique);

	FGenesisLifeSimulationFrame DefyFrame;
	World.Engine->ProcessAction(Defy, World.Now, &DefyFrame);
	TestTrue(TEXT("Gruppe lehnt ab"), DefyFrame.GroupPressure < -0.2f);
	TestTrue(TEXT("Widerstand erfordert Mut"), World.Profile(Child).Karma.Values.Courage > 0.0f);

	// Doppelmoral: erst lügen, dann öffentlich Ehrlichkeit predigen, dann erneut lügen
	FGenesisLifeAction Lie;
	Lie.Definition = MakeLie();
	Lie.ActorId = Sibling;
	Lie.TargetIds.Add(Mother);
	World.Engine->ProcessAction(Lie, World.Now);

	UGenesisLifeActionDefinition* Preach = NewObject<UGenesisLifeActionDefinition>();
	Preach->ActionType = GenesisTags::Theme_Honesty;
	Preach->ExpressesValues.AddTag(GenesisTags::Theme_Honesty);
	Preach->bIsAdvocacy = true;
	Preach->Visibility = 1.0f;
	FGenesisLifeAction PreachAction;
	PreachAction.Definition = Preach;
	PreachAction.ActorId = Sibling;
	PreachAction.WitnessIds.Add(Mother);

	FGenesisLifeSimulationFrame PreachFrame;
	World.Engine->ProcessAction(PreachAction, World.Now + 100, &PreachFrame);
	TestTrue(TEXT("Predigen nach eigener Lüge erkannt"), PreachFrame.Hypocrisy > 0.0f);

	const float ReputationBefore = World.Engine->GetState().GetReputation(Sibling, Mother, GenesisTags::Theme_Honesty);
	FGenesisLifeSimulationFrame SecondLieFrame;
	World.Engine->ProcessAction(Lie, World.Now + 200, &SecondLieFrame);
	TestTrue(TEXT("Lüge gegen erklärten Wert erkannt"), SecondLieFrame.Hypocrisy > 0.0f);
	const FGenesisObserverPerception* MotherView = SecondLieFrame.FindPerception(Mother);
	TestTrue(TEXT("Mutter nimmt den Widerspruch wahr"), MotherView && MotherView->HypocrisyPerceived > 0.0f);

	// Vergleich: dieselbe Lüge vom selben Rufstand aus, aber ohne erklärte Werte
	const float SiblingLoss = ReputationBefore - World.Engine->GetState().GetReputation(Sibling, Mother, GenesisTags::Theme_Honesty);
	FWorld Control(ClearPerceptionTuning());
	Control.Register(Sibling);
	Control.Register(Mother);
	Control.Engine->GetState().FindOrAddReputation(Sibling, Mother, GenesisTags::Theme_Honesty).Score = ReputationBefore;
	Control.Engine->ProcessAction(Lie, Control.Now);
	const float ControlLoss = ReputationBefore - Control.Engine->GetState().GetReputation(Sibling, Mother, GenesisTags::Theme_Honesty);
	TestTrue(FString::Printf(TEXT("Doppelmoral schadet dem Ruf stärker (%.3f > %.3f)"), SiblingLoss, ControlLoss), SiblingLoss > ControlLoss);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisRumorTest, "Genesis.Life.Social.RumorsSpreadAndDistort", GenesisLifeTests::Flags)
bool FGenesisRumorTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World(ClearPerceptionTuning(), 5);
	World.Register(Child);
	World.Register(Mother);
	World.Register(Sibling, EGenesisSimulationLevel::Reduced);
	World.Register(Friend, EGenesisSimulationLevel::Reduced);

	// Freund kennt und vertraut dem Geschwister
	FGenesisTrustEdge& Edge = World.Engine->GetState().FindOrAddTrust(Friend, Sibling);
	Edge.Trust = 0.8f;
	Edge.Familiarity = 1.0f;

	FGenesisLifeAction Lie;
	Lie.Definition = MakeLie();
	Lie.ActorId = Child;
	Lie.TargetIds.Add(Mother);
	Lie.WitnessIds.Add(Sibling);

	FGenesisLifeSimulationFrame Frame;
	const FGuid EventId = World.Engine->ProcessAction(Lie, World.Now, &Frame);
	TestEqual(TEXT("Gerücht entstanden"), Frame.SpawnedRumorIds.Num(), 1);
	TestEqual(TEXT("Freund weiß noch nichts"), World.Engine->GetState().GetReputation(Child, Friend, GenesisTags::Theme_Honesty), 0.0f);

	World.Advance(0.5);

	TestTrue(TEXT("Freund hat es gehört – Ruf gesunken"), World.Engine->GetState().GetReputation(Child, Friend, GenesisTags::Theme_Honesty) < 0.0f);
	const FGenesisMemoryStore* FriendMemory = World.Memory.FindStore(Friend);
	const FGenesisMemoryTrace* Hearsay = FriendMemory ? FriendMemory->FindTraceForEvent(EventId) : nullptr;
	if (TestNotNull(TEXT("Hörensagen im Gedächtnis des Freundes"), Hearsay))
	{
		TestTrue(TEXT("Perspektive Hörensagen"), Hearsay->Perspective == EGenesisMemoryPerspective::Hearsay);
	}

	// Gerüchte verklingen
	World.Advance(3.0);
	TestEqual(TEXT("Gerücht ausgeschwiegen"), World.Engine->GetState().Rumors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisLifeEpigeneticsTest, "Genesis.Life.Epigenetics.StressShapesGenome", GenesisLifeTests::Flags)
bool FGenesisLifeEpigeneticsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World;
	World.Register(Child);

	FGenesisLifeAction Lie;
	Lie.Definition = MakeLie();
	Lie.ActorId = Child;
	for (int32 Index = 0; Index < 40; ++Index)
	{
		World.Engine->ProcessAction(Lie, World.Now + Index);
	}

	const FGuid GenomeId = World.Profile(Child).GenomeId;
	TestEqual(TEXT("Vor Jahreswechsel keine Markierung"), World.Genomes.Find(GenomeId)->GetEpigeneticLevel(GenesisGeneticsTags::Epigenetic_StressResponse), 0.0f);

	World.Advance(1.0);
	TestTrue(TEXT("Chronische Belastung verändert Genaktivität"),
		World.Genomes.Find(GenomeId)->GetEpigeneticLevel(GenesisGeneticsTags::Epigenetic_StressResponse) > 0.0f);
	TestTrue(TEXT("Akuter Stress klingt ab"), World.Profile(Child).Stress < 0.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisLifePersistenceTest, "Genesis.Life.Persistence", GenesisLifeTests::Flags)
bool FGenesisLifePersistenceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World(ClearPerceptionTuning());
	World.Register(Child);
	World.Register(Mother);

	FGenesisLifeAction Lie;
	Lie.Definition = MakeLie();
	Lie.ActorId = Child;
	Lie.TargetIds.Add(Mother);
	World.Engine->ProcessAction(Lie, World.Now);

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben"), GenesisPersistence::Write(World.Engine->GetState(), Bytes));

	FGenesisLifeSimulationState Loaded;
	TestTrue(TEXT("Lesen"), GenesisPersistence::Read(Loaded, Bytes));
	Loaded.RebuildIndices();

	const FGenesisLifeProfile* LoadedChild = Loaded.FindProfile(Child);
	if (TestNotNull(TEXT("Profil"), LoadedChild))
	{
		TestEqual(TEXT("Verborgenes Karma erhalten"), LoadedChild->Karma.Values.Honesty, World.Profile(Child).Karma.Values.Honesty);
	}
	TestEqual(TEXT("Vertrauen erhalten"), Loaded.GetTrust(Mother, Child), World.Engine->GetState().GetTrust(Mother, Child));
	TestEqual(TEXT("Zufallsstrom setzt fort"), Loaded.Rng.NextUInt64(), World.Engine->GetState().Rng.NextUInt64());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisLifePerformanceTest, "Genesis.Life.Performance.Throughput", GenesisLifeTests::Flags)
bool FGenesisLifePerformanceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLifeTests;
	FWorld World(FGenesisLifeSimulationTuning(), 99);

	const int32 PersonCount = 300;
	const int32 ActionCount = 3000;
	TArray<FGuid> People;
	for (int32 Index = 0; Index < PersonCount; ++Index)
	{
		const FGuid Id(4, 0, 0, Index + 1);
		People.Add(Id);
		World.Register(Id, Index < 30 ? EGenesisSimulationLevel::Full : EGenesisSimulationLevel::Reduced);
	}

	UGenesisLifeActionDefinition* Lie = MakeLie();
	FGenesisRandomStream Rng(123);

	const double ActionStart = FPlatformTime::Seconds();
	for (int32 Index = 0; Index < ActionCount; ++Index)
	{
		FGenesisLifeAction Action;
		Action.Definition = Lie;
		Action.ActorId = People[Rng.RandRange(0, PersonCount - 1)];
		Action.TargetIds.Add(People[Rng.RandRange(0, PersonCount - 1)]);
		Action.WitnessIds.Add(People[Rng.RandRange(0, PersonCount - 1)]);
		Action.WitnessIds.Add(People[Rng.RandRange(0, PersonCount - 1)]);
		World.Engine->ProcessAction(Action, World.Now + Index * FGenesisTimestamp::SecondsPerHour);
	}
	const double ActionMs = (FPlatformTime::Seconds() - ActionStart) * 1000.0;

	World.Now = World.Now + ActionCount * FGenesisTimestamp::SecondsPerHour;
	const double StepStart = FPlatformTime::Seconds();
	for (int32 Day = 0; Day < 30; ++Day)
	{
		FGenesisSimulationStep Step;
		Step.From = World.Now;
		Step.To = World.Now + FGenesisTimestamp::SecondsPerDay;
		World.Now = Step.To;
		World.Engine->AdvanceTime(Step);
	}
	const double StepMs = (FPlatformTime::Seconds() - StepStart) * 1000.0 / 30.0;

	const double PerAction = ActionMs / ActionCount;
	AddInfo(FString::Printf(TEXT("Handlungen: %d in %.1f ms (%.3f ms/Handlung) | Tagesschritt: %.3f ms | Ereignisse %d, Kanten %d, Vertrauen %d, Ruf %d, Gerüchte %d"),
		ActionCount, ActionMs, PerAction, StepMs, World.Memory.GetGraph().NumEvents(), World.Memory.GetGraph().NumLinks(),
		World.Engine->GetState().TrustEdges.Num(), World.Engine->GetState().Reputation.Num(), World.Engine->GetState().Rumors.Num()));

	// Budget (Development-Build): Handlungen sind Einzelereignisse, Tagesschritte laufen einmal pro Weltstag
	TestTrue(FString::Printf(TEXT("Handlung unter 1 ms (%.3f ms)"), PerAction), PerAction < 1.0);
	TestTrue(FString::Printf(TEXT("Tagesschritt unter 5 ms (%.3f ms)"), StepMs), StepMs < 5.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
