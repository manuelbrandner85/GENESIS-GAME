// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherDay.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Der Tag der Mutter gegen die Recherche (Docs/34): Puls in der Schwangerschaft +10–20/min, Licht im Mutterleib
 * zur Mitte 0,2 %, am Ende rund 5 % des Außenlichts und nur am Tag; abends spricht sie mit dem Kind, sobald sie es spürt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMotherDayTest, "Genesis.People.MotherDay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisMotherDayTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMotherDay;
	const FGenesisMotherDayTuning Tuning;

	// Puls: in Ruhe im dritten Drittel 10–20/min über dem Wert außerhalb der Schwangerschaft
	const FGenesisMotherMoment EarlyRest = Evaluate(Tuning, 14.0, 3, 6.0f, 1);
	const FGenesisMotherMoment LateRest = Evaluate(Tuning, 22.0, 3, 36.0f, 1);
	AddInfo(FString::Printf(TEXT("Puls in Ruhe: SSW 6 %.0f/min, SSW 36 %.0f/min"), EarlyRest.HeartRateBpm, LateRest.HeartRateBpm));
	TestTrue(TEXT("Drittes Drittel: +10–20/min"), LateRest.Activity == EGenesisMotherActivity::Resting
		&& LateRest.HeartRateBpm - Tuning.RestingHeartRate >= 10.0f && LateRest.HeartRateBpm - Tuning.RestingHeartRate <= 20.0f);
	const FGenesisMotherMoment Night = Evaluate(Tuning, 3.0, 3, 36.0f, 1);
	TestEqual(TEXT("Nachts schläft sie"), Night.Activity, EGenesisMotherActivity::Sleeping);
	TestTrue(TEXT("Im Schlaf ist der Puls niedriger"), Night.HeartRateBpm < LateRest.HeartRateBpm);
	TestEqual(TEXT("Nachts ist es im Mutterleib dunkel"), Night.WombLux, 0.0f);

	// Licht: Anteil im Mutterleib wächst mit der Schwangerschaft
	TestEqual(TEXT("SSW 20: 0,2 % (Mitte)"), WombTransmission(Tuning, 20.0f), 0.002f, 0.0001f);
	TestEqual(TEXT("SSW 40: 5 %"), WombTransmission(Tuning, 40.0f), 0.05f, 0.001f);
	TestTrue(TEXT("Früh noch weniger"), WombTransmission(Tuning, 14.0f) < WombTransmission(Tuning, 20.0f));
	TestEqual(TEXT("Kein Tageslicht um Mitternacht"), DaylightLux(Tuning, 0.0), 0.0f);

	// Ein Spaziergang am Nachmittag: draußen, bewegt, und im Mutterleib wird es rot hell
	bool bFoundWalk = false;
	for (int32 DayIndex = 0; DayIndex < 20 && !bFoundWalk; ++DayIndex)
	{
		for (double Hour = 16.0; Hour < 18.5 && !bFoundWalk; Hour += 0.1)
		{
			const FGenesisMotherMoment Walk = Evaluate(Tuning, Hour, DayIndex, 28.0f, 7);
			if (Walk.Activity == EGenesisMotherActivity::Walking)
			{
				bFoundWalk = true;
				AddInfo(FString::Printf(TEXT("Spaziergang SSW 28, %.1f Uhr: Bauch %.0f lx, Mutterleib %.1f lx, Puls %.0f"),
					Hour, Walk.BellyLux, Walk.WombLux, Walk.HeartRateBpm));
				TestTrue(TEXT("draußen"), Walk.bOutdoors);
				TestTrue(TEXT("sie wiegt das Kind"), Walk.Rocking > 0.5f);
				// Reid et al. 2017 maßen mit kräftigem Rotlicht 16–36 lx im Mutterleib; Tageslicht liegt darunter
				TestTrue(TEXT("im Mutterleib einige lx (rot)"), Walk.WombLux > 0.5f && Walk.WombLux < 40.0f);
				TestTrue(TEXT("Puls beim Gehen höher"), Walk.HeartRateBpm > LateRest.HeartRateBpm);
			}
		}
	}
	TestTrue(TEXT("An mindestens einem Tag geht sie spazieren"), bFoundWalk);

	// Abends spricht sie mit dem Kind – ab SSW 16, oft schon bevor sie es spürt
	auto TalksToBellyAt = [&](float Weeks)
	{
		for (double Hour = 20.0; Hour < 22.0; Hour += 0.05)
		{
			if (Evaluate(Tuning, Hour, 5, Weeks, 3).bTalkingToBelly)
			{
				return true;
			}
		}
		return false;
	};
	TestFalse(TEXT("SSW 14: noch nicht"), TalksToBellyAt(14.0f));
	TestTrue(TEXT("SSW 24: ja"), TalksToBellyAt(24.0f));

	// Nach dem Essen arbeitet der Darm
	TestTrue(TEXT("Verdauung nach dem Mittagessen"), Evaluate(Tuning, 13.5, 2, 30.0f, 1).Digestion > 0.3f);

	// Reproduzierbar: gleicher Tag, gleicher Augenblick
	const FGenesisMotherMoment A = Evaluate(Tuning, 17.3, 9, 30.0f, 11);
	const FGenesisMotherMoment B = Evaluate(Tuning, 17.3, 9, 30.0f, 11);
	TestTrue(TEXT("Deterministisch"), A.Activity == B.Activity && A.WombLux == B.WombLux && A.HeartRateBpm == B.HeartRateBpm);
	return true;
}

/** Geschmack im Fruchtwasser (Docs/37): erst ~45 min nach dem Essen deutlich (Mennella 1995), dann abklingend. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMotherFlavorTest, "Genesis.People.MotherFlavor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisMotherFlavorTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMotherDay;
	const FGenesisMotherDayTuning Tuning;
	int32 Flavored = 0;
	int32 AtMeal = 0;
	int32 LongAfter = 0;
	TSet<EGenesisFlavor> Kinds;
	for (int32 Day = 0; Day < 60; ++Day)
	{
		const double Lunch = Tuning.MealHours[1];
		const FGenesisMotherMoment Now = Evaluate(Tuning, Lunch + 0.1, Day, 30.0f, 7);
		const FGenesisMotherMoment Later = Evaluate(Tuning, Lunch + 1.5, Day, 30.0f, 7);
		const FGenesisMotherMoment Long = Evaluate(Tuning, Lunch + 6.0 - 0.01, Day, 30.0f, 7);
		AtMeal += Now.FlavorStrength > 0.1f && Now.Flavor != EGenesisFlavor::None ? 1 : 0;
		if (Later.FlavorStrength > 0.5f)
		{
			++Flavored;
			Kinds.Add(Later.Flavor);
		}
		LongAfter += Long.FlavorStrength > Later.FlavorStrength ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("Nach dem Mittagessen an %d von 60 Tagen ein deutlicher Geschmack, %d Sorten"), Flavored, Kinds.Num()));
	TestTrue(TEXT("Nicht jede Mahlzeit schmeckt nach etwas, aber viele"), Flavored > 20 && Flavored < 55);
	TestTrue(TEXT("Süß, Knoblauch und Karotte kommen vor"), Kinds.Num() == 3);
	TestTrue(TEXT("Beim Essen selbst noch nichts im Fruchtwasser – außer vom Frühstück"), AtMeal < 60);
	TestEqual(TEXT("Nach Stunden klingt es ab"), LongAfter, 0);
	TestEqual(TEXT("Namen für die Anzeige"), GetFlavorName(EGenesisFlavor::Garlic).Contains(TEXT("Knoblauch")), true);
	return true;
}

#endif
