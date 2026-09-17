// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisCausalGraph.h"
#include "GenesisDecisionConsiderations.h"
#include "GenesisDecisionEngine.h"
#include "GenesisGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisGenomePool.h"
#include "GenesisLifeSimulationEngine.h"
#include "GenesisMemoryWorld.h"
#include "GenesisSoulLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisDecisionTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGuid Honest(5, 0, 0, 1);
	const FGuid Deceiver(5, 0, 0, 2);
	const FGuid Victim(5, 0, 0, 3);

	struct FWorld
	{
		FGenesisMemoryWorld Memory;
		FGenesisGenomePool Genomes;
		TArray<FGenesisTraitDefinition> Traits;
		UGenesisLifeSimulationEngine* Life = nullptr;
		UGenesisDecisionEngine* Decision = nullptr;
		FGenesisTimestamp Now = FGenesisTimestamp::FromCalendar(1600);

		explicit FWorld(uint64 Seed = 11)
		{
			Memory.Reset(Seed);
			Genomes.Reset(Seed);
			Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
			Life = NewObject<UGenesisLifeSimulationEngine>();
			FGenesisLifeSimulationTuning Tuning;
			Tuning.BaseMisreadChance = 0.0f;
			Tuning.CulturalDistanceMisreadWeight = 0.0f;
			Life->Initialize(&Memory, &Genomes, &Traits, Tuning);
			Life->AddDefaultProcessors();
			Life->ResetState(Seed);
			Decision = NewObject<UGenesisDecisionEngine>();
			Decision->AddDefaultConsiderations();
		}

		FGenesisLifeProfile& Register(const FGuid& Id)
		{
			FGenesisLifeProfile Profile;
			Profile.EntityId = Id;
			Profile.SimulationLevel = EGenesisSimulationLevel::Full;
			Life->RegisterEntity(Profile);
			return *Life->GetState().FindProfile(Id);
		}

		FGenesisDecisionInputs Inputs(const FGenesisSoulSeed* Soul = nullptr) const
		{
			FGenesisDecisionInputs Result;
			Result.State = &Life->GetState();
			Result.Memory = &Memory;
			Result.DeciderSoul = Soul;
			Result.Now = Now;
			return Result;
		}
	};

	UGenesisLifeActionDefinition* MakeConfess()
	{
		UGenesisLifeActionDefinition* Action = NewObject<UGenesisLifeActionDefinition>();
		Action->ActionType = GenesisTags::Theme_Honesty;
		Action->Themes.AddTag(GenesisTags::Theme_Honesty);
		Action->ExpressesValues.AddTag(GenesisTags::Theme_Honesty);
		Action->KarmaImpulse.Honesty = 6.0f;
		Action->CostToSelf = 0.4f;
		Action->TrustImpact = 0.3f;
		Action->BaseMagnitude = 0.5f;
		return Action;
	}

	UGenesisLifeActionDefinition* MakeLie()
	{
		UGenesisLifeActionDefinition* Action = NewObject<UGenesisLifeActionDefinition>();
		Action->ActionType = GenesisTags::Theme_Deception;
		Action->Themes.AddTag(GenesisTags::Theme_Deception);
		Action->ViolatesValues.AddTag(GenesisTags::Theme_Honesty);
		Action->KarmaImpulse.Honesty = -8.0f;
		Action->SelfBenefit = 0.6f;
		Action->TrustImpact = -0.5f;
		Action->BaseMagnitude = 0.5f;
		return Action;
	}

	FGenesisDecisionSituation MakeDilemma(const FGuid& Decider)
	{
		FGenesisDecisionSituation Situation;
		Situation.DeciderId = Decider;
		Situation.Stakes = 0.8f;

		FGenesisDecisionOption Confess;
		Confess.Action = MakeConfess();
		Confess.TargetIds.Add(Victim);
		Situation.Options.Add(Confess);

		FGenesisDecisionOption Lie;
		Lie.Action = MakeLie();
		Lie.TargetIds.Add(Victim);
		Situation.Options.Add(Lie);
		return Situation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionCharacterTest, "Genesis.Decision.CharacterShapesChoice", GenesisDecisionTests::Flags)
bool FGenesisDecisionCharacterTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Honest).Karma.Values.Honesty = 70.0f;
	FGenesisLifeProfile& DeceiverProfile = World.Register(Deceiver);
	DeceiverProfile.Karma.Values.Honesty = -60.0f;
	DeceiverProfile.Altruism = -0.5f;
	World.Register(Victim);

	const FGenesisDecisionResult HonestResult = World.Decision->Evaluate(MakeDilemma(Honest), World.Inputs());
	const FGenesisDecisionResult DeceiverResult = World.Decision->Evaluate(MakeDilemma(Deceiver), World.Inputs());

	TestEqual(TEXT("Zwei Optionen bewertet"), HonestResult.Evaluations.Num(), 2);
	TestTrue(TEXT("Ehrliche Person neigt zum Geständnis"), HonestResult.Evaluations[0].Total > HonestResult.Evaluations[1].Total);
	TestTrue(TEXT("Täuschende Person neigt zur Lüge"), DeceiverResult.Evaluations[1].Total > DeceiverResult.Evaluations[0].Total);
	TestTrue(TEXT("Wahrscheinlichkeiten summieren sich zu 1"),
		FMath::IsNearlyEqual(HonestResult.Evaluations[0].Probability + HonestResult.Evaluations[1].Probability, 1.0f, 1.0e-4f));

	// NPC-Wahl über viele Durchläufe: Tendenz statt Determinismus – aber reproduzierbar
	FGenesisRandomStream Rng(42);
	int32 HonestConfessions = 0;
	int32 DeceiverLies = 0;
	for (int32 Run = 0; Run < 200; ++Run)
	{
		FGenesisDecisionResult A = HonestResult;
		World.Decision->ChooseForNpc(A, Rng);
		HonestConfessions += A.ChosenIndex == 0 ? 1 : 0;

		FGenesisDecisionResult B = DeceiverResult;
		World.Decision->ChooseForNpc(B, Rng);
		DeceiverLies += B.ChosenIndex == 1 ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("Ehrliche Person gesteht meistens (%d/200)"), HonestConfessions), HonestConfessions > 140);
	TestTrue(FString::Printf(TEXT("Täuschende Person lügt meistens (%d/200)"), DeceiverLies), DeceiverLies > 140);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionSayNoTest, "Genesis.Decision.NpcCanRefuse", GenesisDecisionTests::Flags)
bool FGenesisDecisionSayNoTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	FGenesisLifeProfile& Stranger = World.Register(Deceiver);
	Stranger.Altruism = -0.6f;
	World.Register(Honest).Altruism = 0.7f;
	World.Register(Victim);

	// Der Spieler (Victim) bittet um kostspielige Hilfe – der Fremde misstraut ihm
	World.Life->GetState().FindOrAddTrust(Deceiver, Victim).Trust = -0.6f;
	World.Life->GetState().FindOrAddTrust(Honest, Victim).Trust = 0.6f;

	UGenesisLifeActionDefinition* Help = NewObject<UGenesisLifeActionDefinition>();
	Help->ActionType = GenesisTags::Theme_Care;
	Help->Themes.AddTag(GenesisTags::Theme_Care);
	Help->OthersBenefit = 0.8f;
	Help->CostToSelf = 0.7f;
	Help->TrustImpact = 0.4f;
	Help->KarmaImpulse.Generosity = 5.0f;

	UGenesisLifeActionDefinition* Refuse = NewObject<UGenesisLifeActionDefinition>();
	Refuse->ActionType = GenesisTags::Theme_Freedom;
	Refuse->Themes.AddTag(GenesisTags::Theme_Freedom);
	Refuse->SelfBenefit = 0.2f;
	Refuse->TrustImpact = -0.2f;

	auto MakeRequest = [&](const FGuid& Decider)
	{
		FGenesisDecisionSituation Situation;
		Situation.DeciderId = Decider;
		FGenesisDecisionOption HelpOption;
		HelpOption.Action = Help;
		HelpOption.TargetIds.Add(Victim);
		Situation.Options.Add(HelpOption);
		FGenesisDecisionOption RefuseOption;
		RefuseOption.Action = Refuse;
		RefuseOption.TargetIds.Add(Victim);
		Situation.Options.Add(RefuseOption);
		return Situation;
	};

	const FGenesisDecisionResult StrangerResult = World.Decision->Evaluate(MakeRequest(Deceiver), World.Inputs());
	const FGenesisDecisionResult FriendResult = World.Decision->Evaluate(MakeRequest(Honest), World.Inputs());
	TestTrue(TEXT("Misstrauischer Egoist sagt eher Nein"), StrangerResult.Evaluations[1].Total > StrangerResult.Evaluations[0].Total);
	TestTrue(TEXT("Vertrauter Altruist hilft eher"), FriendResult.Evaluations[0].Total > FriendResult.Evaluations[1].Total);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionExperienceTest, "Genesis.Decision.ExperienceAndCausality", GenesisDecisionTests::Flags)
bool FGenesisDecisionExperienceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Honest);
	World.Register(Deceiver);
	World.Register(Victim);

	// Früher wurde die Person selbst schmerzhaft belogen
	FGenesisLifeAction PastLie;
	PastLie.Definition = MakeLie();
	PastLie.Definition->Valence = -0.9f;
	PastLie.ActorId = Deceiver;
	PastLie.TargetIds.Add(Honest);
	const FGuid PastEvent = World.Life->ProcessAction(PastLie, World.Now);

	const FGenesisDecisionSituation Situation = MakeDilemma(Honest);
	const FGenesisDecisionResult Result = World.Decision->Evaluate(Situation, World.Inputs());

	const FGenesisConsiderationScore* Experience = Result.Evaluations[1].Breakdown.FindByPredicate(
		[](const FGenesisConsiderationScore& Score) { return Score.Consideration == TEXT("Experience"); });
	if (TestNotNull(TEXT("Erfahrung bewertet"), Experience))
	{
		TestTrue(TEXT("Eigene schmerzhafte Erfahrung spricht gegen Lügen"), Experience->Score < 0.0f);
	}
	TestTrue(TEXT("Erinnerung als Einfluss vermerkt"), Result.Evaluations[1].InfluenceEventIds.Contains(PastEvent));

	// Wählt die Person trotzdem die Lüge, wird die prägende Erinnerung im Kausalgraph zur Ursache
	FGenesisDecisionResult Forced = Result;
	UGenesisDecisionEngine::ApplyPlayerChoice(Forced, 1);
	const FGuid NewEvent = World.Life->ProcessAction(UGenesisDecisionEngine::BuildAction(Situation, Forced), World.Now + 3600);
	const TArray<FGenesisCausalTraceEntry> Causes = World.Memory.GetGraph().TraceCauses(NewEvent, 1, 0.0f);
	TestTrue(TEXT("Frühere Erfahrung ist Ursache der neuen Handlung"),
		Causes.ContainsByPredicate([&PastEvent](const FGenesisCausalTraceEntry& Entry) { return Entry.EventId == PastEvent; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionSubconsciousTest, "Genesis.Decision.SubconsciousAndTimePressure", GenesisDecisionTests::Flags)
bool FGenesisDecisionSubconsciousTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Victim);
	World.Register(Honest).Karma.Values.Courage = 30.0f;

	// Verdrängtes Trauma zum Thema Mut (z. B. als Kind nach einer Konfrontation verletzt)
	FGenesisCausalEvent Trauma;
	Trauma.Time = FGenesisTimestamp::FromCalendar(1580);
	Trauma.ActorId = Victim;
	Trauma.TargetIds.Add(Honest);
	Trauma.Themes.AddTag(GenesisTags::Theme_Courage);
	Trauma.Magnitude = 0.9f;
	Trauma.Valence = -1.0f;
	const FGuid TraumaId = World.Memory.RecordEvent(Trauma)->EventId;
	FGenesisEncodingContext Encoding;
	Encoding.Perspective = EGenesisMemoryPerspective::Target;
	Encoding.Arousal = 1.0f;
	const FGuid TraceId = World.Memory.EncodeMemory(Honest, TraumaId, Encoding, Trauma.Time);
	GenesisMemoryLogic::AdjustRepression(*World.Memory.FindStoreMutable(Honest), TraceId, 0.9f);

	UGenesisLifeActionDefinition* Confront = NewObject<UGenesisLifeActionDefinition>();
	Confront->ActionType = GenesisTags::Theme_Courage;
	Confront->Themes.AddTag(GenesisTags::Theme_Courage);
	Confront->KarmaImpulse.Courage = 6.0f;
	Confront->CostToSelf = 0.2f;

	UGenesisLifeActionDefinition* Withdraw = NewObject<UGenesisLifeActionDefinition>();
	Withdraw->ActionType = GenesisTags::Theme_Fear;
	Withdraw->Themes.AddTag(GenesisTags::Theme_Fear);
	Withdraw->KarmaImpulse.Courage = -2.0f;
	Withdraw->SelfBenefit = 0.2f;

	FGenesisDecisionSituation Situation;
	Situation.DeciderId = Honest;
	FGenesisDecisionOption ConfrontOption;
	ConfrontOption.Action = Confront;
	Situation.Options.Add(ConfrontOption);
	FGenesisDecisionOption WithdrawOption;
	WithdrawOption.Action = Withdraw;
	Situation.Options.Add(WithdrawOption);

	FGenesisDecisionResult Calm = World.Decision->Evaluate(Situation, World.Inputs());
	TestEqual(TEXT("Mit Zeit: Kopf wählt Konfrontation"), Calm.ConsciousBestIndex, 0);
	TestEqual(TEXT("Bauch drängt zum Rückzug"), Calm.ImpulseIndex, 1);
	TestTrue(TEXT("Innerer Konflikt spürbar"), Calm.InnerConflict > 0.0f);
	const FGenesisConsiderationScore* RepressedExperience = Calm.Evaluations[0].Breakdown.FindByPredicate([](const FGenesisConsiderationScore& Score) { return Score.Consideration == TEXT("Experience"); });
	TestTrue(TEXT("Verdrängtes ist bewusst kaum zugänglich"), RepressedExperience && FMath::Abs(RepressedExperience->Score) < 0.1f);
	const FGenesisConsiderationScore* Subconscious = Calm.Evaluations[0].Breakdown.FindByPredicate([](const FGenesisConsiderationScore& Score) { return Score.Consideration == TEXT("Subconscious"); });
	TestTrue(TEXT("…wirkt aber stark im Unterbewusstsein"), Subconscious && Subconscious->Score < -0.5f);

	Situation.TimePressure = 1.0f;
	World.Life->GetState().FindProfile(Honest)->Stress = 0.8f;
	const FGenesisDecisionResult Pressured = World.Decision->Evaluate(Situation, World.Inputs());
	TestTrue(TEXT("Unter Zeitdruck und Stress gewinnt der Impuls"), Pressured.Evaluations[1].Total > Pressured.Evaluations[0].Total);

	// Spieler lässt die Zeit ablaufen → Impuls entscheidet
	FGenesisDecisionResult Timeout = Pressured;
	UGenesisDecisionEngine::ApplyPlayerChoice(Timeout, INDEX_NONE);
	TestEqual(TEXT("Zeit abgelaufen: Impuls"), Timeout.ChosenIndex, 1);
	TestTrue(TEXT("Dem Impuls gefolgt"), Timeout.bFollowedImpulse);

	FGenesisDecisionResult Deliberate = Calm;
	UGenesisDecisionEngine::ApplyPlayerChoice(Deliberate, 0);
	TestFalse(TEXT("Bewusst gegen den Impuls"), Deliberate.bFollowedImpulse);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionSoulEchoTest, "Genesis.Decision.SoulEchoPull", GenesisDecisionTests::Flags)
bool FGenesisDecisionSoulEchoTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Honest);

	// Seele mit offenem Thema "Verlassen" aus einem früheren Leben
	FGenesisSoulSeed Soul = GenesisSoulLogic::CreateSoulSeed(7, {});
	GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
	FGenesisLifeClosure Closure;
	FGenesisLifeClosureTheme Theme;
	Theme.Theme = GenesisTags::Theme_Abandonment;
	Theme.Intensity = 0.9f;
	Closure.CourtThemes.Add(Theme);
	GenesisSoulLogic::CloseIncarnation(Soul, Closure, FGenesisSoulCarryOverParams());

	UGenesisLifeActionDefinition* Leave = NewObject<UGenesisLifeActionDefinition>();
	Leave->ActionType = GenesisTags::Theme_Abandonment;
	Leave->Themes.AddTag(GenesisTags::Theme_Abandonment);
	UGenesisLifeActionDefinition* Stay = NewObject<UGenesisLifeActionDefinition>();
	Stay->ActionType = GenesisTags::Theme_Belonging;
	Stay->Themes.AddTag(GenesisTags::Theme_Belonging);

	FGenesisDecisionSituation Situation;
	Situation.DeciderId = Honest;
	FGenesisDecisionOption LeaveOption;
	LeaveOption.Action = Leave;
	Situation.Options.Add(LeaveOption);
	FGenesisDecisionOption StayOption;
	StayOption.Action = Stay;
	Situation.Options.Add(StayOption);

	const FGenesisDecisionResult WithoutSoul = World.Decision->Evaluate(Situation, World.Inputs());
	const FGenesisDecisionResult WithSoul = World.Decision->Evaluate(Situation, World.Inputs(&Soul));
	TestTrue(TEXT("Offenes Seelenthema zieht unbewusst an"), WithSoul.Evaluations[0].Subconscious > WithoutSoul.Evaluations[0].Subconscious);
	TestEqual(TEXT("Anderes Thema unberührt"), WithSoul.Evaluations[1].Subconscious, WithoutSoul.Evaluations[1].Subconscious);
	TestEqual(TEXT("Bewusste Abwägung unberührt"), WithSoul.Evaluations[0].Conscious, WithoutSoul.Evaluations[0].Conscious);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionHesitationTest, "Genesis.Decision.Hesitation", GenesisDecisionTests::Flags)
bool FGenesisDecisionHesitationTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Honest).Karma.Values.Honesty = 90.0f;
	World.Register(Deceiver);
	World.Register(Victim);

	// Klare Wahl für eine sehr ehrliche Person
	const FGenesisDecisionResult Clear = World.Decision->Evaluate(MakeDilemma(Honest), World.Inputs());

	// Zwei gleichwertige Optionen für eine neutrale Person
	FGenesisDecisionSituation Balanced;
	Balanced.DeciderId = Deceiver;
	FGenesisDecisionOption A;
	A.Action = MakeConfess();
	Balanced.Options.Add(A);
	Balanced.Options.Add(A);
	const FGenesisDecisionResult Torn = World.Decision->Evaluate(Balanced, World.Inputs());

	TestTrue(FString::Printf(TEXT("Gleichwertige Optionen: starkes Zögern (%.2f)"), Torn.Hesitation), Torn.Hesitation > 0.95f);
	TestTrue(FString::Printf(TEXT("Klare Überzeugung: weniger Zögern (%.2f < %.2f)"), Clear.Hesitation, Torn.Hesitation), Clear.Hesitation < Torn.Hesitation);

	// Ungültige Option wird nie gewählt
	FGenesisDecisionSituation WithInvalid = MakeDilemma(Honest);
	WithInvalid.Options.AddDefaulted();
	FGenesisDecisionResult Result = World.Decision->Evaluate(WithInvalid, World.Inputs());
	TestEqual(TEXT("Ungültige Option ohne Wahrscheinlichkeit"), Result.Evaluations[2].Probability, 0.0f);
	UGenesisDecisionEngine::ApplyPlayerChoice(Result, 2);
	TestTrue(TEXT("Ungültige Spielerwahl fällt auf Impuls zurück"), Result.ChosenIndex == Result.ImpulseIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisDecisionPerformanceTest, "Genesis.Decision.Performance.LongLifeMemory", GenesisDecisionTests::Flags)
bool FGenesisDecisionPerformanceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisDecisionTests;
	FWorld World;
	World.Register(Honest);
	World.Register(Victim);

	// Ein langes Leben: 5000 Erinnerungen zu gemischten Themen
	static const FGameplayTag Themes[] = { GenesisTags::Theme_Deception, GenesisTags::Theme_Honesty, GenesisTags::Theme_Care, GenesisTags::Theme_Fear, GenesisTags::Theme_Love };
	FGenesisRandomStream Rng(3);
	for (int32 Index = 0; Index < 5000; ++Index)
	{
		FGenesisCausalEvent Event;
		Event.Time = FGenesisTimestamp::FromCalendar(1500) + Index * FGenesisTimestamp::SecondsPerDay;
		Event.ActorId = Victim;
		Event.TargetIds.Add(Honest);
		Event.Themes.AddTag(Themes[Rng.RandRange(0, UE_ARRAY_COUNT(Themes) - 1)]);
		Event.Valence = Rng.FRandRange(-1.0f, 1.0f);
		const FGuid EventId = World.Memory.RecordEvent(Event)->EventId;
		FGenesisEncodingContext Encoding;
		Encoding.Perspective = EGenesisMemoryPerspective::Target;
		World.Memory.EncodeMemory(Honest, EventId, Encoding, Event.Time);
	}

	const FGenesisDecisionSituation Situation = MakeDilemma(Honest);
	const int32 Runs = 200;
	const double Start = FPlatformTime::Seconds();
	for (int32 Run = 0; Run < Runs; ++Run)
	{
		World.Decision->Evaluate(Situation, World.Inputs());
	}
	const double PerDecisionMs = (FPlatformTime::Seconds() - Start) * 1000.0 / Runs;

	AddInfo(FString::Printf(TEXT("Entscheidung mit 2 Optionen, %d Erinnerungen: %.3f ms"), World.Memory.FindStore(Honest)->Traces.Num(), PerDecisionMs));
	TestTrue(FString::Printf(TEXT("Entscheidung unter 2 ms (%.3f ms)"), PerDecisionMs), PerDecisionMs < 2.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
