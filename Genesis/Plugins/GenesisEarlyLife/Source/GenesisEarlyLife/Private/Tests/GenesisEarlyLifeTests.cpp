// GENESIS: Der Kreislauf des Lebens

#include "GenesisEarlyLifeLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags EarlyLifeFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGenesisNewbornState MakeNewborn(uint64 Seed = 1)
	{
		return GenesisEarlyLifeLogic::BeginNewborn(FGuid(1, 2, 3, static_cast<uint32>(Seed)), FGuid(9, 9, 9, 9), Seed, FGenesisTimestamp());
	}
}

/**
 * Die erste Stunde entscheidet sich an der Wärme. Ohne Hilfe kühlt ein Neugeborenes aus,
 * auf der Haut der Mutter hält es seine Temperatur – das ist kein Komfort, sondern Medizin.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEarlyLifeWarmthTest, "Genesis.EarlyLife.WarmthDecidesTheFirstHour", EarlyLifeFlags)

bool FGenesisEarlyLifeWarmthTest::RunTest(const FString& Parameters)
{
	const FGenesisEarlyLifeTuning Tuning;

	FGenesisNewbornState Alone = MakeNewborn(3);
	GenesisEarlyLifeLogic::Advance(Alone, Tuning, 20.0);

	FGenesisNewbornState Held = MakeNewborn(3);
	Held.bSkinToSkin = true;
	GenesisEarlyLifeLogic::Advance(Held, Tuning, 20.0);

	AddInfo(FString::Printf(TEXT("Nach 20 min: allein %.1f °C (%s), auf der Haut %.1f °C (%s)"),
		Alone.BodyTemperature, *GenesisEarlyLifeLogic::GetStageName(Alone.Stage),
		Held.BodyTemperature, *GenesisEarlyLifeLogic::GetStageName(Held.Stage)));

	TestTrue(TEXT("Allein kühlt das Kind aus"), Alone.BodyTemperature < 35.0f);
	TestEqual(TEXT("Allein wird es unterkühlt"), static_cast<int32>(Alone.Stage), static_cast<int32>(EGenesisNewbornStage::Hypothermic));
	TestTrue(TEXT("Auf der Haut hält es seine Wärme"), Held.BodyTemperature > 36.8f);
	TestTrue(TEXT("Auf der Haut ist es ruhig"), Held.Calm > Alone.Calm + 0.3f);

	// Wieder aufgewärmt kommt das Kind zurück – das dauert länger als das Auskühlen,
	// weil die Erwärmung dem kleiner werdenden Gefälle zur Mutter folgt
	Alone.bSkinToSkin = true;
	GenesisEarlyLifeLogic::Advance(Alone, Tuning, 45.0);
	AddInfo(FString::Printf(TEXT("Nach dem Aufwärmen: %.1f °C, %s"), Alone.BodyTemperature, *GenesisEarlyLifeLogic::GetStageName(Alone.Stage)));
	TestTrue(TEXT("Aufwärmen hilft"), Alone.BodyTemperature > 36.0f);
	AddInfo(FString::Printf(TEXT("Auskühlen ging schneller als Aufwärmen: 20 min hinunter, 45 min hinauf")));
	TestTrue(TEXT("Das Kind ist nicht mehr unterkühlt"), Alone.Stage != EGenesisNewbornStage::Hypothermic);

	return true;
}

/** Der Brustkrabbelgang: Liegt das Kind warm und ruhig, findet es die Brust in etwa einer halben Stunde selbst. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEarlyLifeFeedTest, "Genesis.EarlyLife.FindsTheBreast", EarlyLifeFlags)

bool FGenesisEarlyLifeFeedTest::RunTest(const FString& Parameters)
{
	const FGenesisEarlyLifeTuning Tuning;

	FGenesisNewbornState State = MakeNewborn(7);
	State.bSkinToSkin = true;
	State.bMotherSpeaking = true;

	double Minutes = 0.0;
	while (!State.bHasFed && Minutes < 120.0)
	{
		GenesisEarlyLifeLogic::Advance(State, Tuning, 1.0);
		Minutes += 1.0;
	}

	AddInfo(FString::Printf(TEXT("Erstes Anlegen nach %.0f min, Ruhe %.2f, Bindung %.2f"), Minutes, State.Calm, State.Bonding));
	TestTrue(TEXT("Das Kind trinkt in der ersten Stunde"), State.bHasFed && Minutes <= 60.0);
	TestTrue(TEXT("Nicht sofort – es braucht Zeit"), Minutes >= 25.0);

	// Ohne Hautkontakt findet es die Brust nicht von selbst
	FGenesisNewbornState Apart = MakeNewborn(7);
	GenesisEarlyLifeLogic::Advance(Apart, Tuning, 60.0);
	TestFalse(TEXT("Ohne Hautkontakt kein Anlegen von selbst"), Apart.bHasFed);

	return true;
}

/** Bindung entsteht aus dem, was geschieht – nicht daraus, dass Zeit vergeht. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEarlyLifeBondingTest, "Genesis.EarlyLife.BondingNeedsContact", EarlyLifeFlags)

bool FGenesisEarlyLifeBondingTest::RunTest(const FString& Parameters)
{
	const FGenesisEarlyLifeTuning Tuning;

	FGenesisNewbornState Warm = MakeNewborn(11);
	Warm.bSkinToSkin = true;
	Warm.bMotherSpeaking = true;

	FGenesisNewbornState Alone = MakeNewborn(11);

	GenesisEarlyLifeLogic::Advance(Warm, Tuning, 45.0);
	GenesisEarlyLifeLogic::Advance(Alone, Tuning, 45.0);

	AddInfo(FString::Printf(TEXT("Nach 45 min: mit Haut und Stimme Bindung %.2f (%.0f min geschrien), allein %.2f (%.0f min geschrien)"),
		Warm.Bonding, Warm.CryingMinutes, Alone.Bonding, Alone.CryingMinutes));

	TestTrue(TEXT("Nähe bindet"), Warm.Bonding > 0.6f);
	TestTrue(TEXT("Allein bindet sich niemand"), Alone.Bonding < 0.05f);
	TestTrue(TEXT("Allein schreit das Kind"), Alone.CryingMinutes > Warm.CryingMinutes + 20.0f);

	return true;
}

/** Was das Kind sieht und hört: auf Armlänge scharf, anfangs geblendet, die Stimme der Mutter vertraut. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEarlyLifePerceptionTest, "Genesis.EarlyLife.PerceptionOfANewborn", EarlyLifeFlags)

bool FGenesisEarlyLifePerceptionTest::RunTest(const FString& Parameters)
{
	const FGenesisEarlyLifeTuning Tuning;
	FGenesisNewbornState State = MakeNewborn(5);
	State.bSkinToSkin = true;

	const FGenesisNewbornPerception First = GenesisEarlyLifeLogic::GetPerception(State, Tuning);
	AddInfo(FString::Printf(TEXT("Erste Sekunde: Blendung %.2f, Sehschärfe %.3f, Schärfe auf %.0f mm"),
		First.Glare, First.VisualAcuity, First.FocusDistanceMm));
	TestTrue(TEXT("Anfangs geblendet"), First.Glare > 0.9f);
	TestTrue(TEXT("Sehschärfe wie 20/400"), First.VisualAcuity < 0.1f);
	TestTrue(TEXT("Schärfe auf Armlänge"), FMath::Abs(First.FocusDistanceMm - 250.0f) < 60.0f);
	TestTrue(TEXT("Ohne Stimme keine Vertrautheit"), First.VoiceFamiliarity < 0.01f);

	GenesisEarlyLifeLogic::Advance(State, Tuning, 10.0);
	State.bMotherSpeaking = true;
	const FGenesisNewbornPerception Later = GenesisEarlyLifeLogic::GetPerception(State, Tuning);
	AddInfo(FString::Printf(TEXT("Nach 10 min: Blendung %.2f, Wärme %.2f, Stimme vertraut %.2f"),
		Later.Glare, Later.Warmth, Later.VoiceFamiliarity));

	TestTrue(TEXT("Das Auge gewöhnt sich"), Later.Glare < First.Glare);
	TestTrue(TEXT("Die Stimme ist von Anfang an vertraut"), Later.VoiceFamiliarity > 0.5f);
	TestTrue(TEXT("Auf der Haut ist dem Kind warm"), Later.Warmth > 0.8f);

	return true;
}

/** Gleiche Bedingungen, gleicher Verlauf – unabhängig von der Schrittweite. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEarlyLifeDeterminismTest, "Genesis.EarlyLife.Determinism", EarlyLifeFlags)

bool FGenesisEarlyLifeDeterminismTest::RunTest(const FString& Parameters)
{
	const FGenesisEarlyLifeTuning Tuning;

	FGenesisNewbornState Fine = MakeNewborn(23);
	Fine.bSkinToSkin = true;
	for (int32 Step = 0; Step < 40; ++Step)
	{
		GenesisEarlyLifeLogic::Advance(Fine, Tuning, 1.0);
	}

	FGenesisNewbornState Coarse = MakeNewborn(23);
	Coarse.bSkinToSkin = true;
	GenesisEarlyLifeLogic::Advance(Coarse, Tuning, 40.0);

	AddInfo(FString::Printf(TEXT("fein: %.2f °C, Bindung %.3f | grob: %.2f °C, Bindung %.3f"),
		Fine.BodyTemperature, Fine.Bonding, Coarse.BodyTemperature, Coarse.Bonding));
	TestTrue(TEXT("Gleiche Temperatur"), FMath::IsNearlyEqual(Fine.BodyTemperature, Coarse.BodyTemperature, 0.01f));
	TestTrue(TEXT("Gleiche Bindung"), FMath::IsNearlyEqual(Fine.Bonding, Coarse.Bonding, 0.01f));
	TestEqual(TEXT("Gleiche Stufe"), static_cast<int32>(Coarse.Stage), static_cast<int32>(Fine.Stage));

	return true;
}

#endif
