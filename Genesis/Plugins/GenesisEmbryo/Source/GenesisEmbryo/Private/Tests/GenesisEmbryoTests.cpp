// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FGenesisEmbryoState MakeEmbryo(uint32 Seed, float Vitality, bool bPlayer = true)
	{
		FGenesisEmbryoTuning Tuning;
		FGuid Entity(Seed, 1, 2, 3);
		FGuid Genome(Seed, 4, 5, 6);
		FGenesisEmbryoState State = GenesisEmbryoLogic::CreateZygote(Entity, Genome, Vitality, 0.6f, FGenesisTimestamp(), Tuning);
		State.bPlayerEmbryo = bPlayer;
		return State;
	}
}

/**
 * Die Zeiten der ersten Woche müssen stimmen: erste Teilung nach gut einem Tag,
 * Morula am dritten, Blastozyste am fünften, Einnistung in der zweiten Wochenhälfte.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoTimelineTest, "Genesis.Embryo.Timeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoTimelineTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;
	FGenesisEmbryoState State = MakeEmbryo(11, 0.8f);

	GenesisEmbryoLogic::Advance(State, Tuning, 20.0);
	TestEqual(TEXT("Nach 20 h noch eine Zelle"), State.GetCellCount(), 1);

	GenesisEmbryoLogic::Advance(State, Tuning, 14.0); // 34 h
	TestTrue(TEXT("Nach 34 h geteilt"), State.GetCellCount() >= 2);
	AddInfo(FString::Printf(TEXT("34 h: %d Zellen, Stufe %s"), State.GetCellCount(), *GenesisEmbryoLogic::GetStageName(State.Stage)));

	GenesisEmbryoLogic::Advance(State, Tuning, 38.0); // 72 h = Tag 3
	AddInfo(FString::Printf(TEXT("72 h: %d Zellen, Stufe %s, Kompaktierung %.0f %%"),
		State.GetCellCount(), *GenesisEmbryoLogic::GetStageName(State.Stage), 100.0f * State.Compaction));
	TestTrue(TEXT("Tag 3: mindestens 8 Zellen"), State.GetCellCount() >= 8);
	TestTrue(TEXT("Tag 3: Kompaktierung hat begonnen"), State.Stage == EGenesisEmbryoStage::Morula || State.Stage == EGenesisEmbryoStage::Blastocyst);

	GenesisEmbryoLogic::Advance(State, Tuning, 48.0); // 120 h = Tag 5
	AddInfo(FString::Printf(TEXT("120 h: %d Zellen, Stufe %s, Hohlraum %.0f %%, Embryoblast %d"),
		State.GetCellCount(), *GenesisEmbryoLogic::GetStageName(State.Stage), 100.0f * State.Cavity,
		GenesisEmbryoLogic::CountInnerCellMass(State)));
	TestTrue(TEXT("Tag 5: Blastozyste, noch nicht eingenistet"), State.Stage >= EGenesisEmbryoStage::Blastocyst && State.Stage <= EGenesisEmbryoStage::Hatching);
	TestTrue(TEXT("Tag 5: Embryoblast angelegt"), GenesisEmbryoLogic::CountInnerCellMass(State) >= 3);

	GenesisEmbryoLogic::Advance(State, Tuning, 24.0); // 144 h = Tag 6
	AddInfo(FString::Printf(TEXT("144 h: Stufe %s, Zona %.1f µm"), *GenesisEmbryoLogic::GetStageName(State.Stage), State.ZonaThicknessUm));
	TestTrue(TEXT("Tag 6: Schlüpfen oder Einnistung"), State.Stage >= EGenesisEmbryoStage::Hatching && State.Stage <= EGenesisEmbryoStage::Implanting);

	GenesisEmbryoLogic::Advance(State, Tuning, 96.0); // 240 h = Tag 10
	AddInfo(FString::Printf(TEXT("240 h: Stufe %s, Einnistung %.0f %%, Qualität %.2f"),
		*GenesisEmbryoLogic::GetStageName(State.Stage), 100.0f * State.Implantation, State.Quality));
	TestTrue(TEXT("Tag 10: eingenistet"), State.Stage == EGenesisEmbryoStage::Implanted);
	TestEqual(TEXT("Zona ist aufgebraucht"), State.ZonaThicknessUm, 0.0f);

	return true;
}

/** Gleiches Genom, gleicher Ablauf – auch wenn die Zeit in anderen Schritten vergeht. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoDeterminismTest, "Genesis.Embryo.Determinism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoDeterminismTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;

	FGenesisEmbryoState Single = MakeEmbryo(7, 0.7f);
	GenesisEmbryoLogic::Advance(Single, Tuning, 120.0);

	FGenesisEmbryoState Chunked = MakeEmbryo(7, 0.7f);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		GenesisEmbryoLogic::Advance(Chunked, Tuning, 12.0);
	}

	TestEqual(TEXT("Gleiche Zellzahl"), Chunked.GetCellCount(), Single.GetCellCount());
	TestEqual(TEXT("Gleiche Stufe"), static_cast<int32>(Chunked.Stage), static_cast<int32>(Single.Stage));
	TestTrue(TEXT("Gleiche Qualität"), FMath::IsNearlyEqual(Chunked.Quality, Single.Quality, 0.001f));

	double MaxDistance = 0.0;
	for (int32 Index = 0; Index < Single.Cells.Num(); ++Index)
	{
		MaxDistance = FMath::Max(MaxDistance, FVector::Dist(Single.Cells[Index].Position, Chunked.Cells[Index].Position));
	}
	AddInfo(FString::Printf(TEXT("%d Zellen, größter Lageunterschied %.4f µm"), Single.GetCellCount(), MaxDistance));
	TestTrue(TEXT("Zellen liegen gleich"), MaxDistance < 0.01);

	// Ein anderes Genom ergibt einen anderen Verlauf
	FGenesisEmbryoState Other = MakeEmbryo(8, 0.7f);
	GenesisEmbryoLogic::Advance(Other, Tuning, 120.0);
	const bool bDiffers = Other.GetCellCount() != Single.GetCellCount()
		|| FVector::Dist(Other.Cells[0].Position, Single.Cells[0].Position) > 0.5;
	TestTrue(TEXT("Anderes Genom, anderer Verlauf"), bDiffers);

	return true;
}

/** Der Keim wächst nicht: Alle Zellen bleiben in der Zona, und zusammen füllen sie dasselbe Volumen. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoGeometryTest, "Genesis.Embryo.GeometryStaysInZona",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoGeometryTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;
	FGenesisEmbryoState State = MakeEmbryo(3, 0.75f);

	double WorstOutside = 0.0;
	for (int32 Hour = 0; Hour < 120; ++Hour)
	{
		GenesisEmbryoLogic::Advance(State, Tuning, 1.0);
		// Bis zur Morula ist die Zona die Grenze; die Blastozyste dehnt sich darüber hinaus aus
		const float Available = Tuning.InnerRadiusUm * (1.0f + 0.35f * State.Cavity);
		for (const FGenesisBlastomere& Cell : State.Cells)
		{
			WorstOutside = FMath::Max(WorstOutside, Cell.Position.Size() + Cell.RadiusUm - Available);
		}
	}
	AddInfo(FString::Printf(TEXT("Größter Überstand über die Zona: %.2f µm bei %d Zellen"), WorstOutside, State.GetCellCount()));
	TestTrue(TEXT("Keine Zelle steht aus der Zona heraus"), WorstOutside < 0.5);

	// Volumenerhalt: Die Summe der Zellvolumina entspricht der ursprünglichen Eizelle (bis zur Mindestgröße)
	const double Total = State.GetCellCount() * FMath::Pow(State.Cells[0].RadiusUm, 3.0);
	const double Original = FMath::Pow(55.0, 3.0);
	AddInfo(FString::Printf(TEXT("Volumenverhältnis zur Eizelle: %.2f (Zellradius %.1f µm)"), Total / Original, State.Cells[0].RadiusUm));
	TestTrue(TEXT("Der Keim wächst nicht"), Total / Original < 1.05);

	return true;
}

/** Lebenskraft entscheidet über saubere Teilungen – und damit über die Organanlagen des späteren Körpers. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoQualityTest, "Genesis.Embryo.QualityFollowsVitality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoQualityTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;

	FGenesisEmbryoState Strong = MakeEmbryo(5, 0.95f);
	FGenesisEmbryoState Weak = MakeEmbryo(5, 0.15f);
	GenesisEmbryoLogic::Advance(Strong, Tuning, 144.0);
	GenesisEmbryoLogic::Advance(Weak, Tuning, 144.0);

	const float StrongQuality = GenesisEmbryoLogic::GetDevelopmentQuality(Strong);
	const float WeakQuality = GenesisEmbryoLogic::GetDevelopmentQuality(Weak);
	AddInfo(FString::Printf(TEXT("kräftig: Qualität %.2f, Fragmentierung %.0f %% | schwach: Qualität %.2f, Fragmentierung %.0f %%"),
		StrongQuality, 100.0f * Strong.Fragmentation, WeakQuality, 100.0f * Weak.Fragmentation));

	TestTrue(TEXT("Kräftiger Keim entwickelt sich besser"), StrongQuality > WeakQuality + 0.05f);
	TestTrue(TEXT("Schwacher Keim fragmentiert stärker"), Weak.Fragmentation > Strong.Fragmentation);

	return true;
}

/**
 * Biologisch kommt nur ein Teil aller befruchteten Eizellen bis zur Einnistung.
 * Der Keim des Spielers ist davon ausgenommen – sonst gäbe es kein Leben zu spielen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoArrestTest, "Genesis.Embryo.ArrestOnlyForOthers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoArrestTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;

	int32 Arrested = 0;
	for (uint32 Seed = 0; Seed < 60; ++Seed)
	{
		FGenesisEmbryoState Other = MakeEmbryo(Seed, 0.5f, false);
		GenesisEmbryoLogic::Advance(Other, Tuning, 168.0);
		Arrested += Other.Stage == EGenesisEmbryoStage::Arrested ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("%d von 60 fremden Keimen bleiben stehen (%.0f %%)"), Arrested, 100.0f * Arrested / 60.0f));
	TestTrue(TEXT("Manche Keime bleiben stehen"), Arrested > 0);
	TestTrue(TEXT("Nicht alle bleiben stehen"), Arrested < 60);

	int32 PlayerArrested = 0;
	for (uint32 Seed = 0; Seed < 40; ++Seed)
	{
		FGenesisEmbryoState Player = MakeEmbryo(Seed, 0.2f, true);
		GenesisEmbryoLogic::Advance(Player, Tuning, 168.0);
		PlayerArrested += Player.Stage == EGenesisEmbryoStage::Arrested ? 1 : 0;
	}
	TestEqual(TEXT("Der Keim des Spielers bleibt nie stehen"), PlayerArrested, 0);

	return true;
}

#endif
