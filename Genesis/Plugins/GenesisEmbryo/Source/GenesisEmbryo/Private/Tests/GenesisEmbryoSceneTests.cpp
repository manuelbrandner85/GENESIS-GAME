// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoScene.h"
#include "GenesisEmbryogenesisLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Die Fruchthöhle zeigt, was die Simulation rechnet (GENESIS-041 Teil 5): Der Körper hat die simulierte Länge, das Herz
 * schlägt im simulierten Takt – und zwar als echter Schlag (rasch zusammen, langsam erschlaffen, Ruhe).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoSceneViewTest, "Genesis.Embryo.SceneView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoSceneViewTest::RunTest(const FString& Parameters)
{
	using namespace GenesisEmbryoSceneView;

	// Der Herzschlag als Verlauf: entspannt am Anfang, voll zusammengezogen früh im Zyklus, danach Ruhe
	TestEqual(TEXT("Zu Beginn des Zyklus entspannt"), HeartContraction(0.0), 0.0f, 0.001f);
	float Peak = 0.0f;
	double PeakAt = 0.0;
	for (int32 Step = 0; Step <= 100; ++Step)
	{
		const double Phase = Step / 100.0;
		const float Value = HeartContraction(Phase);
		if (Value > Peak)
		{
			Peak = Value;
			PeakAt = Phase;
		}
	}
	AddInfo(FString::Printf(TEXT("Stärkste Kontraktion %.2f bei %.0f %% des Zyklus"), Peak, 100.0 * PeakAt));
	TestTrue(TEXT("Das Herz zieht sich ganz zusammen"), Peak > 0.9f);
	TestTrue(TEXT("Die Kontraktion kommt früh (Systole kürzer als Diastole)"), PeakAt < 0.25);
	TestEqual(TEXT("In der zweiten Zyklushälfte ist Ruhe"), HeartContraction(0.7), 0.0f, 0.001f);
	TestEqual(TEXT("Der nächste Zyklus beginnt gleich"), HeartContraction(3.05), HeartContraction(0.05), 0.001f);

	// Aus der Simulation: Tag 28 hat die Länge des Modells, Tag 26 ist kleiner
	FGenesisEmbryogenesisState State;
	const FGenesisEmbryogenesisTuning Tuning;
	for (float Day = 13.0f; Day <= 26.0f; Day += 0.1f)
	{
		GenesisEmbryogenesisLogic::Advance(State, Tuning, Day, 0.7f, true, 1);
	}
	const FGenesisEmbryoSceneView Day26 = Compute(State, 26.0 * 24.0, 0.1);
	for (float Day = 26.0f; Day <= 28.0f; Day += 0.1f)
	{
		GenesisEmbryogenesisLogic::Advance(State, Tuning, Day, 0.7f, true, 1);
	}
	const FGenesisEmbryoSceneView Day28 = Compute(State, 28.0 * 24.0, 0.1);
	AddInfo(FString::Printf(TEXT("Maßstab Tag 26: %.2f, Tag 28: %.2f; Herz %.0f/min"), Day26.Scale, Day28.Scale, Day28.HeartRateBpm));
	TestTrue(TEXT("Tag 28 hat die Länge des Modells"), FMath::IsNearlyEqual(Day28.Scale, 1.0f, 0.02f));
	TestTrue(TEXT("Tag 26 ist kleiner"), Day26.Scale < Day28.Scale && Day26.Scale > 0.6f);
	TestTrue(TEXT("Das Herz schlägt im Takt der Simulation"), Day28.bHeartBeating && FMath::IsNearlyEqual(Day28.HeartRateBpm, State.HeartRateBpm));
	TestEqual(TEXT("Der Tag stimmt"), Day28.Day, 28.0f, 0.001f);

	// Ohne Herzschlag keine Kontraktion
	FGenesisEmbryogenesisState Silent = State;
	Silent.bHeartBeating = false;
	TestEqual(TEXT("Ein stilles Herz zieht sich nicht zusammen"), Compute(Silent, 21.0 * 24.0, 0.1).HeartContraction, 0.0f);
	return true;
}

#endif
