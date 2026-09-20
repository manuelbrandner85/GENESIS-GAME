// GENESIS: Der Kreislauf des Lebens

#include "GenesisBirthLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags BirthTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGenesisBirthState MakeLabor(uint64 Seed, float Weeks = 40.0f, float LungMaturity = 1.0f)
	{
		const FGenesisBirthTuning Tuning;
		return GenesisBirthLogic::BeginLabor(FGuid(1, 2, 3, static_cast<uint32>(Seed)), Seed, Weeks, LungMaturity, FGenesisTimestamp(), Tuning);
	}

	/** Führt die Geburt in Minutenschritten weiter, bis das Kind geboren ist oder die Zeit abläuft. */
	double RunToBirth(FGenesisBirthState& State, const FGenesisBirthTuning& Tuning, double MaxHours = 30.0)
	{
		double Minutes = 0.0;
		while (!State.IsBorn() && Minutes < MaxHours * 60.0)
		{
			GenesisBirthLogic::Advance(State, Tuning, 1.0);
			Minutes += 1.0;
		}
		return Minutes;
	}
}

/** Eine Geburt am Termin dauert Stunden, nicht Minuten – und läuft die Abschnitte der Reihe nach durch. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBirthCourseTest, "Genesis.Birth.Course", BirthTestFlags)

bool FGenesisBirthCourseTest::RunTest(const FString& Parameters)
{
	const FGenesisBirthTuning Tuning;
	FGenesisBirthState State = MakeLabor(4);
	TestEqual(TEXT("Beginnt in der frühen Eröffnungsphase"), static_cast<int32>(State.Stage), static_cast<int32>(EGenesisLaborStage::Latent));

	GenesisBirthLogic::Advance(State, Tuning, 60.0);
	AddInfo(FString::Printf(TEXT("Nach 1 h: %s, Muttermund %.1f cm, %d Wehen"),
		*GenesisBirthLogic::GetStageName(State.Stage), State.DilationCm, State.ContractionCount));
	TestTrue(TEXT("Nach einer Stunde hat sich der Muttermund geöffnet"), State.DilationCm > 0.1f);
	TestTrue(TEXT("Nach einer Stunde noch nicht geboren"), !State.IsBorn());

	const double Minutes = RunToBirth(State, Tuning);
	AddInfo(FString::Printf(TEXT("Geburt nach %.1f h, %d Wehen, Erschwernis: %s, Apgar %d"),
		Minutes / 60.0, State.ContractionCount, *GenesisBirthLogic::GetComplicationName(State.Complication), State.ApgarScore));

	TestTrue(TEXT("Das Kind wird geboren"), State.IsBorn());
	TestTrue(TEXT("Die Geburt dauert mindestens vier Stunden"), Minutes > 240.0);
	TestTrue(TEXT("Die Geburt dauert höchstens 24 Stunden"), Minutes < 1440.0);
	TestTrue(TEXT("Es gab viele Wehen"), State.ContractionCount > 40);

	// Nach der Geburt: der erste Atemzug
	GenesisBirthLogic::Advance(State, Tuning, 2.0);
	AddInfo(FString::Printf(TEXT("2 min nach der Geburt: Atemzug %s, Sauerstoff %.2f, Herz %.0f/min, Apgar %d"),
		State.bFirstBreath ? TEXT("ja") : TEXT("nein"), State.Oxygen, State.HeartRateBpm, State.ApgarScore));
	TestTrue(TEXT("Das Kind hat geatmet"), State.bFirstBreath);
	TestTrue(TEXT("Der Sauerstoff steigt nach dem ersten Atemzug"), State.Oxygen > 0.85f);

	return true;
}

/**
 * Der Kern der Geburt aus Sicht des Kindes: Jede Wehe drückt den Sauerstoff herunter,
 * zwischen den Wehen kommt er zurück. Ohne dieses Auf und Ab wäre es keine Geburt, sondern ein Balken.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBirthContractionTest, "Genesis.Birth.ContractionsDipOxygen", BirthTestFlags)

bool FGenesisBirthContractionTest::RunTest(const FString& Parameters)
{
	const FGenesisBirthTuning Tuning;
	FGenesisBirthState State = MakeLabor(9);

	// In die aktive Phase vorspulen
	while (State.Stage < EGenesisLaborStage::Active)
	{
		GenesisBirthLogic::Advance(State, Tuning, 5.0);
	}

	float MinOxygen = 1.0f;
	float MaxOxygen = 0.0f;
	float MaxPressure = 0.0f;
	float MinHeartRate = 200.0f;
	// Feine Abtastung: In Minutenschritten träfe man bei einem Wehenabstand von vier Minuten
	// immer denselben Punkt im Zyklus und sähe die Wehe nie (Aliasing).
	for (int32 Step = 0; Step < 720; ++Step) // zwei Stunden in Zehn-Sekunden-Schritten
	{
		GenesisBirthLogic::Advance(State, Tuning, 1.0 / 6.0);
		const FGenesisBirthPerception Perception = GenesisBirthLogic::GetPerception(State);
		MinOxygen = FMath::Min(MinOxygen, Perception.Oxygen);
		MaxOxygen = FMath::Max(MaxOxygen, Perception.Oxygen);
		MaxPressure = FMath::Max(MaxPressure, Perception.Pressure);
		MinHeartRate = FMath::Min(MinHeartRate, Perception.HeartRateBpm);
	}

	AddInfo(FString::Printf(TEXT("Sauerstoff schwankt zwischen %.2f und %.2f, Druckspitze %.2f, Herz minimal %.0f/min"),
		MinOxygen, MaxOxygen, MaxPressure, MinHeartRate));
	TestTrue(TEXT("Der Sauerstoff fällt unter den Wehen"), MinOxygen < 0.8f);
	TestTrue(TEXT("Der Sauerstoff erholt sich zwischen den Wehen"), MaxOxygen > 0.95f);
	TestTrue(TEXT("Die Wehen erreichen volle Stärke"), MaxPressure > 0.5f);
	TestTrue(TEXT("Der Herzschlag fällt mit ab"), MinHeartRate < 135.0f);

	return true;
}

/** Was das Kind wahrnimmt, muss zum Ort passen: dunkel und dumpf im Kanal, hell und klar danach. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBirthPerceptionTest, "Genesis.Birth.PerceptionFollowsPlace", BirthTestFlags)

bool FGenesisBirthPerceptionTest::RunTest(const FString& Parameters)
{
	const FGenesisBirthTuning Tuning;
	FGenesisBirthState State = MakeLabor(21);

	const FGenesisBirthPerception Early = GenesisBirthLogic::GetPerception(State);
	TestTrue(TEXT("Im Kanal ist es dunkel"), Early.Light < 0.01f);
	TestTrue(TEXT("Im Mutterleib klingt alles dumpf"), Early.SoundMuffling > 0.9f);
	TestTrue(TEXT("Noch keine Kälte"), Early.Cold < 0.01f);

	RunToBirth(State, Tuning);
	GenesisBirthLogic::Advance(State, Tuning, 1.0);
	const FGenesisBirthPerception After = GenesisBirthLogic::GetPerception(State);
	AddInfo(FString::Printf(TEXT("Nach der Geburt: Licht %.2f, Dumpfheit %.2f, Kälte %.2f, Druck %.2f"),
		After.Light, After.SoundMuffling, After.Cold, After.Pressure));

	TestTrue(TEXT("Draußen ist es hell"), After.Light > 0.9f);
	TestTrue(TEXT("Draußen klingt alles klar"), After.SoundMuffling < 0.2f);
	TestTrue(TEXT("Draußen ist es kalt"), After.Cold > 0.9f);
	TestTrue(TEXT("Der Druck ist weg"), After.Pressure < 0.01f);

	return true;
}

/** Eine schwere Geburt kostet: Sauerstoffmangel senkt das erste Zustandsbild. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBirthOutcomeTest, "Genesis.Birth.HardBirthCosts", BirthTestFlags)

bool FGenesisBirthOutcomeTest::RunTest(const FString& Parameters)
{
	FGenesisBirthTuning Easy;
	FGenesisBirthTuning Hard;
	// Eine Nabelschnur unter Druck bedeutet: tiefere Einbrüche, langsamere Erholung
	Hard.OxygenDipPerMinute = Easy.OxygenDipPerMinute * 1.8f;
	Hard.OxygenRecoveryPerMinute = Easy.OxygenRecoveryPerMinute * 0.55f;

	FGenesisBirthState Gentle = MakeLabor(31);
	FGenesisBirthState Severe = MakeLabor(31);

	RunToBirth(Gentle, Easy);
	GenesisBirthLogic::Advance(Gentle, Easy, 2.0);
	RunToBirth(Severe, Hard);
	GenesisBirthLogic::Advance(Severe, Hard, 2.0);

	AddInfo(FString::Printf(TEXT("ruhige Geburt: %.0f min Sauerstoffmangel, Apgar %d | schwere Geburt: %.0f min, Apgar %d"),
		Gentle.HypoxiaMinutes, Gentle.ApgarScore, Severe.HypoxiaMinutes, Severe.ApgarScore));

	TestTrue(TEXT("Die schwere Geburt bedeutet mehr Sauerstoffmangel"), Severe.HypoxiaMinutes > Gentle.HypoxiaMinutes);
	TestTrue(TEXT("Die ruhige Geburt endet besser"), Gentle.ApgarScore >= Severe.ApgarScore);
	TestTrue(TEXT("Auch die ruhige Geburt ist Arbeit"), Gentle.HypoxiaMinutes > 0.0f);

	return true;
}

/** Gleiches Kind, gleicher Verlauf – unabhängig davon, in welchen Schritten die Zeit vergeht. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBirthDeterminismTest, "Genesis.Birth.Determinism", BirthTestFlags)

bool FGenesisBirthDeterminismTest::RunTest(const FString& Parameters)
{
	const FGenesisBirthTuning Tuning;

	FGenesisBirthState Fine = MakeLabor(77);
	for (int32 Step = 0; Step < 180; ++Step)
	{
		GenesisBirthLogic::Advance(Fine, Tuning, 1.0);
	}

	FGenesisBirthState Coarse = MakeLabor(77);
	for (int32 Step = 0; Step < 6; ++Step)
	{
		GenesisBirthLogic::Advance(Coarse, Tuning, 30.0);
	}

	AddInfo(FString::Printf(TEXT("fein: %.2f cm, %d Wehen | grob: %.2f cm, %d Wehen"),
		Fine.DilationCm, Fine.ContractionCount, Coarse.DilationCm, Coarse.ContractionCount));
	TestTrue(TEXT("Gleiche Öffnung"), FMath::IsNearlyEqual(Fine.DilationCm, Coarse.DilationCm, 0.01f));
	TestEqual(TEXT("Gleiche Wehenzahl"), Coarse.ContractionCount, Fine.ContractionCount);
	TestTrue(TEXT("Gleicher Sauerstoff"), FMath::IsNearlyEqual(Fine.Oxygen, Coarse.Oxygen, 0.01f));

	return true;
}

#endif
