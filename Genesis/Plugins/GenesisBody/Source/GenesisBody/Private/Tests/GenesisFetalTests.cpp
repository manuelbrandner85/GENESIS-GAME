// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisBodyLogic.h"
#include "GenesisFetalLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Die Fetalzeit gegen die Recherche (Docs/34): Wachstum nach WHO und Robinson & Fleming, Herzfrequenz, Hören nach
 * Hepper & Shahidullah, Augen, Bewusstsein, Verhaltenszustände nach Nijhuis, Kindslage.
 */
namespace GenesisFetalTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const FGenesisFetalReference Reference;

	FGenesisFetalView At(float Weeks) { return GenesisFetalLogic::Evaluate(Reference, Weeks); }

	bool Hears(const FGenesisFetalView& View, float Hz) { return View.HearingHighHz > 0.0f && Hz >= View.HearingLowHz && Hz <= View.HearingHighHz; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFetalGrowthTest, "Genesis.Body.Fetal.Growth", GenesisFetalTests::Flags)
bool FGenesisFetalGrowthTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFetalTests;
	TestEqual(TEXT("SSW 6: 4,6 mm – dasselbe Maß wie der Embryo in der Fruchthöhle"), At(6.0f).CrownRumpCm, 0.46f, 0.01f);
	TestEqual(TEXT("SSW 12: Scheitel-Steiß 5,4 cm"), At(12.0f).CrownRumpCm, 5.4f, 0.05f);
	TestEqual(TEXT("SSW 20: 330 g (WHO)"), At(20.0f).WeightGrams, 330.0f, 1.0f);
	TestEqual(TEXT("SSW 20: Scheitel-Ferse ~25,7 cm"), At(20.0f).CrownHeelCm, 25.7f, 0.1f);
	TestEqual(TEXT("SSW 40: 3617 g (WHO)"), At(40.0f).WeightGrams, 3617.0f, 1.0f);
	TestEqual(TEXT("SSW 40: 51 cm"), At(40.0f).CrownHeelCm, 51.0f, 0.1f);
	TestEqual(TEXT("Vor SSW 14 keine Scheitel-Fersen-Länge"), At(10.0f).CrownHeelCm, 0.0f);

	float Peak = 0.0f;
	float PeakWeek = 0.0f;
	for (float Weeks = 6.0f; Weeks <= 40.0f; Weeks += 0.25f)
	{
		if (At(Weeks).HeartRateBpm > Peak)
		{
			Peak = At(Weeks).HeartRateBpm;
			PeakWeek = Weeks;
		}
	}
	AddInfo(FString::Printf(TEXT("Herz am schnellsten: %.0f/min in SSW %.1f; Termin %.0f/min"), Peak, PeakWeek, At(40.0f).HeartRateBpm));
	TestTrue(TEXT("Herz am schnellsten in SSW 9–10, 150–170/min"), PeakWeek >= 9.0f && PeakWeek <= 10.0f && Peak >= 150.0f && Peak <= 170.0f);
	TestTrue(TEXT("Zum Termin 130–140/min"), At(40.0f).HeartRateBpm >= 130.0f && At(40.0f).HeartRateBpm <= 140.0f);

	// Der Körper rechnet mit der Referenz (Wochen seit Befruchtung = SSW − 2)
	const FGenesisTimestamp Conception = FGenesisTimestamp::FromCalendar(2000, 10, 8);
	FGenesisBodyState Body = GenesisBodyLogic::CreateAtConception(FGuid(9, 0, 0, 1), FGuid(), FGenesisBodyGenetics(), FGenesisConceptionVitality(), Conception, EGenesisSimulationLevel::Full);
	const FGenesisTimestamp Week18 = Conception + FGenesisTimestamp::DaysToSeconds(18.0 * 7.0);
	GenesisBodyLogic::AdvanceDays(Body, Week18, 18.0 * 7.0, 0.0f, FGenesisBodyTuning());
	GenesisBodyLogic::AdvanceHours(Body, Week18, 1.0, 0.0f);
	TestEqual(TEXT("Körper 18 Wochen nach Befruchtung (SSW 20): 0,33 kg"), Body.WeightKg, 0.33f, 0.005f);
	TestEqual(TEXT("… und 25,7 cm"), Body.HeightCm, 25.7f, 0.1f);
	TestEqual(TEXT("… Herz nach der Referenz"), Body.Vitals.HeartRate, At(20.0f).HeartRateBpm, 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFetalSensesTest, "Genesis.Body.Fetal.Senses", GenesisFetalTests::Flags)
bool FGenesisFetalSensesTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFetalTests;
	// Hören: nichts vor SSW 19; dann zuerst 500 Hz; SSW 27 tief (250, 500), noch nicht 1000/3000; 1000 Hz ab 33, 3000 ab 35
	TestFalse(TEXT("SSW 18: noch kein Ton"), Hears(At(18.5f), 500.0f));
	TestEqual(TEXT("SSW 18: nichts kommt an"), At(18.5f).HearingSensitivity, 0.0f);
	TestTrue(TEXT("SSW 19: 500 Hz"), Hears(At(19.0f), 500.0f));
	TestFalse(TEXT("SSW 19: noch nicht 250 Hz"), Hears(At(19.0f), 250.0f));
	TestFalse(TEXT("SSW 19: noch nicht 1000 Hz"), Hears(At(19.0f), 1000.0f));
	TestTrue(TEXT("SSW 27: 250 und 500 Hz"), Hears(At(27.0f), 250.0f) && Hears(At(27.0f), 500.0f));
	TestFalse(TEXT("SSW 27: noch nicht 1000 Hz"), Hears(At(27.0f), 1000.0f));
	TestTrue(TEXT("SSW 33: 1000 Hz"), Hears(At(33.0f), 1000.0f));
	TestFalse(TEXT("SSW 33: noch nicht 3000 Hz"), Hears(At(33.0f), 3000.0f));
	TestTrue(TEXT("SSW 35: 3000 Hz"), Hears(At(35.0f), 3000.0f));
	float Previous = 0.0f;
	bool bRising = true;
	for (float Weeks = 19.0f; Weeks <= 40.0f; Weeks += 0.5f)
	{
		bRising &= At(Weeks).HearingSensitivity >= Previous - 0.0001f;
		Previous = At(Weeks).HearingSensitivity;
	}
	TestTrue(TEXT("Die Schwelle sinkt stetig (Empfindlichkeit steigt)"), bRising);
	AddInfo(FString::Printf(TEXT("Empfindlichkeit: SSW 19 %.2f, 28 %.2f, 40 %.2f"), At(19.0f).HearingSensitivity, At(28.0f).HearingSensitivity, At(40.0f).HearingSensitivity));
	TestTrue(TEXT("Zum Termin voll"), At(40.0f).HearingSensitivity > 0.99f);

	// Sehen, Tasten, Bewusstsein
	TestEqual(TEXT("SSW 25: Lider noch verwachsen"), At(25.0f).EyesOpen, 0.0f);
	TestEqual(TEXT("SSW 28: Augen offen"), At(28.0f).EyesOpen, 1.0f);
	TestTrue(TEXT("Ab SSW 32 volle Lichtwahrnehmung"), At(32.0f).LightPerception > 0.95f);
	TestEqual(TEXT("SSW 7: noch kein Tasten"), At(7.0f).Touch, 0.0f);
	TestTrue(TEXT("SSW 14: Tasten am ganzen Körper"), At(14.0f).Touch > 0.95f);
	TestEqual(TEXT("SSW 22: noch kein bewusster Zugang (Thalamus–Rinde)"), At(22.0f).ConsciousAccess, 0.0f);
	TestEqual(TEXT("SSW 26: bewusster Zugang"), At(26.0f).ConsciousAccess, 1.0f);
	TestFalse(TEXT("SSW 19: Die Mutter spürt noch nichts (Erstgebärende)"), At(19.0f).bMotherFeelsMovement);
	TestTrue(TEXT("SSW 20: Die Mutter spürt die Bewegungen"), At(20.0f).bMotherFeelsMovement);
	TestEqual(TEXT("SSW 28: ~20 % Beckenendlage"), At(28.0f).BreechShare, 0.20f, 0.001f);
	TestTrue(TEXT("Termin: 3–4 % Beckenendlage"), At(40.0f).BreechShare >= 0.03f && At(40.0f).BreechShare <= 0.04f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFetalBehaviourTest, "Genesis.Body.Fetal.Behaviour", GenesisFetalTests::Flags)
bool FGenesisFetalBehaviourTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFetalTests;
	const FGenesisFetalMilestones& M = Reference.Milestones;

	auto Simulate = [&](float Weeks, float Hours, int32 Seed, TMap<EGenesisFetalEvent, int32>& Counts)
	{
		FGenesisFetalBehaviour Behaviour;
		FRandomStream Random(Seed);
		const FGenesisFetalView View = At(Weeks);
		for (int32 Second = 0; Second < static_cast<int32>(Hours * 3600.0f); ++Second)
		{
			for (EGenesisFetalEvent Event : GenesisFetalLogic::AdvanceBehaviour(Behaviour, View, M, 1.0f, 0.0f, Random))
			{
				Counts.FindOrAdd(Event)++;
			}
		}
		return Behaviour;
	};
	auto Share = [](const FGenesisFetalBehaviour& Behaviour, EGenesisFetalState State)
	{
		float Total = 0.0f;
		for (float Seconds : Behaviour.SecondsInState) { Total += Seconds; }
		return Total > 0.0f ? Behaviour.SecondsInState[static_cast<int32>(State)] / Total : 0.0f;
	};

	// Vor den ersten Bewegungen (SSW 7,5) geschieht nichts
	TMap<EGenesisFetalEvent, int32> Early;
	Simulate(7.0f, 6.0f, 1, Early);
	TestEqual(TEXT("SSW 7: keine Bewegungen"), Early.Num(), 0);

	// Schluckauf erst ab SSW 9
	TMap<EGenesisFetalEvent, int32> Before;
	Simulate(8.5f, 24.0f, 2, Before);
	TestEqual(TEXT("SSW 8,5: kein Schluckauf"), Before.FindRef(EGenesisFetalEvent::Hiccup), 0);
	TMap<EGenesisFetalEvent, int32> Twelve;
	Simulate(12.0f, 24.0f, 3, Twelve);
	TestTrue(TEXT("SSW 12: Schluckauf in Serien"), Twelve.FindRef(EGenesisFetalEvent::Hiccup) > 20);
	TestTrue(TEXT("SSW 12: schluckt Fruchtwasser"), Twelve.FindRef(EGenesisFetalEvent::Swallow) > 0);

	// Vor SSW 32: Ruhe 58 %, Aktivität 42 %
	TMap<EGenesisFetalEvent, int32> Mid;
	const FGenesisFetalBehaviour At28 = Simulate(28.0f, 72.0f, 4, Mid);
	AddInfo(FString::Printf(TEXT("SSW 28: Ruhe %.0f %%, Aktivität %.0f %%"), 100.0f * Share(At28, EGenesisFetalState::Quiet), 100.0f * Share(At28, EGenesisFetalState::Active)));
	TestEqual(TEXT("SSW 28: Ruhe ~58 %"), Share(At28, EGenesisFetalState::Quiet), 0.58f, 0.07f);
	TestEqual(TEXT("SSW 28: noch keine Wachzustände"), Share(At28, EGenesisFetalState::ActiveAwake), 0.0f);

	// Ab SSW 36–38: ruhiger Schlaf 24 %, aktiver Schlaf 65 %, aktiv wach 11 %
	TMap<EGenesisFetalEvent, int32> Late;
	const FGenesisFetalBehaviour At38 = Simulate(38.0f, 240.0f, 5, Late);
	AddInfo(FString::Printf(TEXT("SSW 38: ruhiger Schlaf %.0f %%, aktiver Schlaf %.0f %%, wach %.0f %%"), 100.0f * Share(At38, EGenesisFetalState::Quiet),
		100.0f * Share(At38, EGenesisFetalState::Active), 100.0f * Share(At38, EGenesisFetalState::ActiveAwake)));
	TestEqual(TEXT("SSW 38: ruhiger Schlaf ~24 %"), Share(At38, EGenesisFetalState::Quiet), 0.24f, 0.07f);
	TestEqual(TEXT("SSW 38: aktiver Schlaf ~65 %"), Share(At38, EGenesisFetalState::Active), 0.65f, 0.08f);
	TestEqual(TEXT("SSW 38: aktiv wach ~11 %"), Share(At38, EGenesisFetalState::ActiveAwake), 0.11f, 0.05f);
	TestTrue(TEXT("SSW 38: Tritte"), Late.FindRef(EGenesisFetalEvent::Kick) > 100);

	// Deterministisch: gleicher Keim, gleiches Verhalten
	TMap<EGenesisFetalEvent, int32> A;
	TMap<EGenesisFetalEvent, int32> B;
	Simulate(24.0f, 4.0f, 42, A);
	Simulate(24.0f, 4.0f, 42, B);
	TestTrue(TEXT("Deterministisch"), A.OrderIndependentCompareEqual(B));
	return true;
}

#endif
