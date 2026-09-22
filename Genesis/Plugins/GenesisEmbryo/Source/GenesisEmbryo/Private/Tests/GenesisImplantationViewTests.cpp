// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoLogic.h"
#include "GenesisImplantationSite.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FGenesisEmbryoState MakeHatchedForView(float AppositionAtHours)
	{
		const FGenesisEmbryoTuning Tuning;
		FGenesisEmbryoState State = GenesisEmbryoLogic::CreateZygote(FGuid(7, 1, 2, 3), FGuid(7, 4, 5, 6), 0.8f, 0.6f, FGenesisTimestamp(), Tuning);
		State.Cells.Reset();
		State.Stage = EGenesisEmbryoStage::Implanting;
		State.ZonaThicknessUm = 0.0f;
		State.Cavity = 1.0f;
		State.HoursSinceFusion = AppositionAtHours - Tuning.FloatAfterHatchingHours;
		State.Nidation.AppositionAtHours = AppositionAtHours;
		return State;
	}

	FGenesisImplantationView ViewAt(FGenesisEmbryoState& State, double Hours)
	{
		const FGenesisEmbryoTuning Tuning;
		GenesisEmbryoLogic::Advance(State, Tuning, Hours - State.HoursSinceFusion);
		return GenesisImplantationView::Compute(State, Tuning);
	}
}

/**
 * Was man von der Einnistung sieht, folgt der Simulation: Der Keim sinkt auf die Schleimhaut, dreht den Embryoblasten
 * nach unten, versinkt, darüber erst ein Pfropf, dann geschlossenes Epithel; das Blut der Mutter scheint durch,
 * die Stelle wölbt sich, die Kamera weicht mit dem wachsenden Keim zurück.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisImplantationViewTest, "Genesis.Embryo.ImplantationView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisImplantationViewTest::RunTest(const FString& Parameters)
{
	FGenesisEmbryoState State = MakeHatchedForView(144.0f);

	const FGenesisImplantationView Floating = ViewAt(State, 140.5);
	TestTrue(TEXT("Frisch geschlüpft: frei über der Schleimhaut"), Floating.CenterHeightUm > Floating.RadiusUm + 20.0f);
	TestEqual(TEXT("Frei: noch nicht gedreht"), Floating.Orientation, 0.0f);

	const FGenesisImplantationView Resting = ViewAt(State, 150.0);
	TestTrue(TEXT("Angelagert: liegt auf"), FMath::Abs(Resting.CenterHeightUm - Resting.RadiusUm) < 5.0f);
	TestTrue(TEXT("Angelagert: Embryonalpol dreht sich nach unten"), Resting.Orientation > 0.2f);
	TestEqual(TEXT("Angelagert: noch kein Wulst"), Resting.Collar, 0.0f);

	const FGenesisImplantationView Sinking = ViewAt(State, 190.0);
	TestTrue(TEXT("Invasion: halb versunken, Schnittlinie mit der Oberfläche"), Sinking.WaterlineRadiusUm > 0.5f * Sinking.RadiusUm);
	TestEqual(TEXT("Invasion: ganz gedreht"), Sinking.Orientation, 1.0f);
	TestTrue(TEXT("Invasion: Epithelwulst um den Keim"), Sinking.Collar > 0.9f);
	TestTrue(TEXT("Invasion: noch zu sehen"), Sinking.bEmbryoVisible);

	const FGenesisImplantationView Plugged = ViewAt(State, 236.0);
	TestFalse(TEXT("Tag 10: nicht mehr zu sehen"), Plugged.bEmbryoVisible);
	TestTrue(TEXT("Tag 10: Fibrinpfropf über dem Keim"), Plugged.PlugRadiusUm > 30.0f);

	const FGenesisImplantationView Closed = ViewAt(State, 300.0);
	TestEqual(TEXT("Tag 12,5: Epithel geschlossen"), Closed.PlugRadiusUm, 0.0f, 0.5f);
	TestTrue(TEXT("Tag 12,5: Blut scheint durch"), Closed.BloodShowing > 0.7f);
	TestTrue(TEXT("Tag 12,5: Stelle gewölbt"), Closed.DomeHeightUm > 10.0f && Closed.DomeHeightUm < 0.12f * Closed.RadiusUm);
	TestTrue(TEXT("Tag 12,5: Hyperämie"), Closed.Hyperemia > 0.8f);
	TestTrue(TEXT("Die Kamera zeigt mehr, wenn der Keim wächst"), Closed.FramingWidthUm > 2.0f * Floating.FramingWidthUm);
	TestTrue(TEXT("Der ganze Keim ist im Bild"), Closed.FramingWidthUm >= 4.0f * Closed.RadiusUm);
	return true;
}

#endif
