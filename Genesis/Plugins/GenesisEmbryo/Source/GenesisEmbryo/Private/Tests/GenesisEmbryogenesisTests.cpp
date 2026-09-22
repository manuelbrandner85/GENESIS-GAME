// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryogenesisLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FGenesisEmbryogenesisState At(float Day, float Nutrition = 0.7f, bool bPlayer = true, uint64 Seed = 42)
	{
		FGenesisEmbryogenesisState State;
		const FGenesisEmbryogenesisTuning Tuning;
		// In Schritten wie im Spiel, damit auch der Würfelwurf an seiner Stelle fällt
		for (float Step = 13.0f; Step <= Day + 0.001f; Step += 0.1f)
		{
			GenesisEmbryogenesisLogic::Advance(State, Tuning, FMath::Min(Step, Day), Nutrition, bPlayer, Seed);
		}
		return State;
	}
}

/**
 * Die dritte und vierte Woche gegen die Embryologie (Langman, Moore, O'Rahilly & Müller, Carnegie 7–13):
 * jede Stufe an ihrem Tag, Somiten im klinischen Takt, der erste Herzschlag an Tag 22.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryogenesisTimelineTest, "Genesis.Embryo.BodyPlanTimeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryogenesisTimelineTest::RunTest(const FString& Parameters)
{
	using namespace GenesisEmbryogenesisLogic;

	const FGenesisEmbryogenesisState Day14 = At(14.0f);
	TestTrue(TEXT("Tag 14: noch die zweiblättrige Scheibe"), Day14.Stage == EGenesisEmbryogenesisStage::None);

	const FGenesisEmbryogenesisState Day15 = At(15.2f);
	TestTrue(TEXT("Tag 15: Primitivstreifen"), Day15.Stage == EGenesisEmbryogenesisStage::PrimitiveStreak);
	TestTrue(TEXT("Tag 15: noch keine drei Blätter"), Day15.ThreeLayers < 0.1f);

	const FGenesisEmbryogenesisState Day17 = At(17.0f);
	TestTrue(TEXT("Tag 17: Gastrulation läuft"), Day17.Stage == EGenesisEmbryogenesisStage::Gastrulation && Day17.ThreeLayers > 0.3f);

	const FGenesisEmbryogenesisState Day19 = At(19.5f);
	TestTrue(TEXT("Tag 19: Neuralplatte"), Day19.Stage == EGenesisEmbryogenesisStage::NeuralPlate);
	TestTrue(TEXT("Tag 19: Chorda steht"), Day19.Notochord > 0.7f);
	TestEqual(TEXT("Tag 19: noch keine Somiten"), Day19.Somites, 0);

	const FGenesisEmbryogenesisState Day21 = At(21.0f);
	AddInfo(FString::Printf(TEXT("Tag 21: %s, %.1f mm, %d Somiten, Neuralrohr %.0f %%"),
		*GetStageName(Day21.Stage), Day21.LengthMm, Day21.Somites, 100.0f * Day21.TubeClosure));
	TestTrue(TEXT("Tag 21: erste Somitenpaare"), Day21.Somites >= 3 && Day21.Somites <= 5);
	TestFalse(TEXT("Tag 21: das Herz schlägt noch nicht"), Day21.bHeartBeating);

	const FGenesisEmbryogenesisState Day22 = At(22.1f);
	TestTrue(TEXT("Tag 22: das Herz schlägt"), Day22.bHeartBeating && Day22.Stage == EGenesisEmbryogenesisStage::HeartBeats);
	TestTrue(TEXT("Tag 22: erster Schlag langsam (70–85/min)"), Day22.HeartRateBpm >= 70.0f && Day22.HeartRateBpm <= 85.0f);
	TestTrue(TEXT("Tag 22: Herzschlauch verschmolzen"), Day22.HeartTube > 0.99f);

	const FGenesisEmbryogenesisState Day25 = At(25.0f);
	TestTrue(TEXT("Tag 25: vorderer Neuroporus zu"), Day25.RostralNeuropore < 0.05f);
	TestTrue(TEXT("Tag 25: hinterer noch offen"), Day25.CaudalNeuropore > 0.1f);
	TestTrue(TEXT("Tag 25: erste Kiemenbögen"), Day25.PharyngealArches >= 1);

	const FGenesisEmbryogenesisState Day28 = At(28.0f);
	AddInfo(FString::Printf(TEXT("Tag 28: %s, %.1f mm, %d Somiten, Herz %.0f/min, Bögen %d, Knospen %.0f %%"),
		*GetStageName(Day28.Stage), Day28.LengthMm, Day28.Somites, Day28.HeartRateBpm, Day28.PharyngealArches, 100.0f * Day28.LimbBuds));
	TestTrue(TEXT("Ende der vierten Woche erreicht"), Day28.Stage == EGenesisEmbryogenesisStage::Complete);
	TestTrue(TEXT("Neuralrohr geschlossen"), Day28.TubeClosure > 0.99f && Day28.CaudalNeuropore < 0.05f);
	TestTrue(TEXT("Rund 30 Somitenpaare"), Day28.Somites >= 26 && Day28.Somites <= 30);
	TestTrue(TEXT("4–5 mm lang"), Day28.LengthMm >= 4.0f && Day28.LengthMm <= 5.0f);
	TestTrue(TEXT("Herz schneller geworden (100–140/min)"), Day28.HeartRateBpm >= 100.0f && Day28.HeartRateBpm <= 140.0f);
	TestTrue(TEXT("Extremitätenknospen angelegt"), Day28.LimbBuds > 0.3f);
	return true;
}

/**
 * Das Neuralrohr schließt von der Mitte nach vorn und hinten – und es kann offen bleiben.
 * Wie oft, hängt an der Ernährung der Mutter (Folat). Der Keim des Spielers ist davon ausgenommen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisNeuralTubeTest, "Genesis.Embryo.NeuralTube",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisNeuralTubeTest::RunTest(const FString& Parameters)
{
	using namespace GenesisEmbryogenesisLogic;
	const FGenesisEmbryogenesisTuning Tuning;

	// Das Risiko folgt der Ernährung: schlecht ernährt vielfach höher als gut ernährt
	const float Poor = DefectRiskFor(0.0f, Tuning);
	const float Good = DefectRiskFor(1.0f, Tuning);
	AddInfo(FString::Printf(TEXT("Risiko: schlecht ernährt %.2f %%, gut ernährt %.2f %%"), 100.0f * Poor, 100.0f * Good));
	TestTrue(TEXT("Schlechte Ernährung erhöht das Risiko deutlich"), Poor > 5.0f * Good);
	TestTrue(TEXT("Auch gut ernährt bleibt ein Restrisiko"), Good > 0.0f);

	// Anteil über viele Keime: nahe am Risiko
	auto Share = [&](float Nutrition)
	{
		int32 Defects = 0;
		const int32 Count = 4000;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FGenesisEmbryogenesisState State;
			Advance(State, Tuning, 22.0f, Nutrition, false, 7000 + Index);
			Defects += State.Defect != EGenesisNeuralTubeDefect::None ? 1 : 0;
		}
		return static_cast<float>(Defects) / Count;
	};
	const float PoorShare = Share(0.1f);
	AddInfo(FString::Printf(TEXT("4000 Keime bei schlechter Ernährung: %.2f %% mit offenem Neuralrohr (Risiko %.2f %%)"),
		100.0f * PoorShare, 100.0f * DefectRiskFor(0.1f, Tuning)));
	TestTrue(TEXT("Der Anteil liegt in der Nähe des Risikos"), FMath::Abs(PoorShare - DefectRiskFor(0.1f, Tuning)) < 0.004f);

	// Der Keim des Spielers bekommt keinen Defekt
	int32 PlayerDefects = 0;
	for (int32 Index = 0; Index < 500; ++Index)
	{
		FGenesisEmbryogenesisState State;
		Advance(State, Tuning, 28.0f, 0.0f, true, 500 + Index);
		PlayerDefects += State.Defect != EGenesisNeuralTubeDefect::None ? 1 : 0;
	}
	TestEqual(TEXT("Der Keim des Spielers schließt sein Neuralrohr immer"), PlayerDefects, 0);

	// Bleibt es offen, bleibt es offen: ein offener Rücken schließt sich nicht nachträglich
	FGenesisEmbryogenesisState Open;
	for (float Day = 21.0f; Day <= 28.0f; Day += 0.25f)
	{
		Advance(Open, Tuning, Day, 0.0f, false, 12345);
	}
	if (Open.Defect == EGenesisNeuralTubeDefect::SpinaBifida)
	{
		TestTrue(TEXT("Offener Rücken bleibt offen"), Open.CaudalNeuropore > 0.4f);
	}
	return true;
}

/** Eine Rechnung, egal in welchen Schritten: Die Woche 3–4 folgt dem Tag, nicht der Schrittweite. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryogenesisStepTest, "Genesis.Embryo.BodyPlanStepIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryogenesisStepTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryogenesisTuning Tuning;
	FGenesisEmbryogenesisState Once;
	GenesisEmbryogenesisLogic::Advance(Once, Tuning, 26.0f, 0.7f, true, 3);

	FGenesisEmbryogenesisState Many;
	for (float Day = 13.0f; Day <= 26.0f; Day += 0.037f)
	{
		GenesisEmbryogenesisLogic::Advance(Many, Tuning, FMath::Min(Day, 26.0f), 0.7f, true, 3);
	}
	GenesisEmbryogenesisLogic::Advance(Many, Tuning, 26.0f, 0.7f, true, 3);

	TestEqual(TEXT("Länge"), Once.LengthMm, Many.LengthMm, 0.001f);
	TestEqual(TEXT("Somiten"), Once.Somites, Many.Somites);
	TestEqual(TEXT("Herzfrequenz"), Once.HeartRateBpm, Many.HeartRateBpm, 0.001f);
	TestEqual(TEXT("Neuralrohr"), Once.TubeClosure, Many.TubeClosure, 0.001f);
	TestTrue(TEXT("Stufe"), Once.Stage == Many.Stage);
	return true;
}

#endif
