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
	// Kompaktierung klinisch ab ~80 h, also am vierten Tag – die frühere Erwartung „Tag 3" war zu früh
	TestTrue(TEXT("Tag 3: noch keine Blastozyste"), State.Stage < EGenesisEmbryoStage::Blastocyst);

	GenesisEmbryoLogic::Advance(State, Tuning, 24.0); // 96 h = Tag 4
	TestTrue(TEXT("Tag 4: Morula"), State.Stage == EGenesisEmbryoStage::Morula || State.Stage == EGenesisEmbryoStage::Blastocyst);

	GenesisEmbryoLogic::Advance(State, Tuning, 24.0); // 120 h = Tag 5
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
	// Seit GENESIS-040 heißt „eingenistet" das Ende der zweiten Woche (Primärzotten, Tag 13); an Tag 10 liegt der Keim
	// ganz in der Schleimhaut
	TestTrue(TEXT("Tag 10: in der Schleimhaut versunken"), State.Stage == EGenesisEmbryoStage::Implanting && State.Nidation.Embedded > 0.9f);
	TestEqual(TEXT("Zona ist aufgebraucht"), State.ZonaThicknessUm, 0.0f);

	GenesisEmbryoLogic::Advance(State, Tuning, 96.0); // 336 h = Tag 14
	TestTrue(TEXT("Tag 14: eingenistet"), State.Stage == EGenesisEmbryoStage::Implanted);

	return true;
}

namespace
{
	/**
	 * Ein geschlüpfter Keim ohne Zellen – in der zweiten Woche zählen Gewebe, nicht einzelne Zellen. So lassen sich
	 * Tausende Keime rechnen, ohne jedes Mal die Furchung durchzuspielen.
	 */
	FGenesisEmbryoState MakeHatched(uint32 Seed, float AppositionAtHours, bool bPlayer)
	{
		FGenesisEmbryoState State = MakeEmbryo(Seed, 0.75f, bPlayer);
		State.Cells.Reset();
		State.Stage = EGenesisEmbryoStage::Implanting;
		State.ZonaThicknessUm = 0.0f;
		State.Cavity = 1.0f;
		State.HoursSinceFusion = AppositionAtHours - 4.0;
		State.Nidation.AppositionAtHours = AppositionAtHours;
		return State;
	}

	void AdvanceTo(FGenesisEmbryoState& State, double Hours)
	{
		GenesisEmbryoLogic::Advance(State, FGenesisEmbryoTuning(), Hours - State.HoursSinceFusion);
	}
}

/**
 * Die zweite Woche gegen die Embryologie (Langman, Moore, Carnegie 5–6): jede Stufe an ihrem Tag, die Keimscheibe
 * zweiblättrig, die Höhlen in der richtigen Reihenfolge, das hCG erst ab Tag 9–10 messbar und dann mit klinischer Steigung.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoSecondWeekTest, "Genesis.Embryo.SecondWeek",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoSecondWeekTest::RunTest(const FString& Parameters)
{
	using namespace GenesisEmbryoLogic;
	FGenesisEmbryoState State = MakeHatched(21, 144.0f, true);
	const FGenesisImplantationState& Nid = State.Nidation;

	AdvanceTo(State, 142.0);
	TestTrue(TEXT("Vor Tag 6 treibt er frei"), Nid.Phase == EGenesisImplantationPhase::None);

	AdvanceTo(State, 150.0);
	TestTrue(TEXT("Tag 6: angelagert"), Nid.Phase == EGenesisImplantationPhase::Apposition);
	TestTrue(TEXT("Tag 6: noch an der Oberfläche"), Nid.Embedded < 0.05f);
	TestTrue(TEXT("Tag 6: kein Synzytium, kein hCG"), Nid.SyncytiumThicknessUm == 0.0f && Nid.HcgMilliIU == 0.0f);

	AdvanceTo(State, 180.0);
	TestTrue(TEXT("Tag 7,5: Invasion"), Nid.Phase == EGenesisImplantationPhase::Invasion);
	TestTrue(TEXT("Tag 7,5: Synzytium wächst"), Nid.SyncytiumThicknessUm > 5.0f);
	TestTrue(TEXT("Tag 7,5: teilweise versunken"), Nid.Embedded > 0.1f && Nid.Embedded < 0.6f);
	TestTrue(TEXT("Tag 7,5: hCG gebildet, im Blut noch nicht messbar"), Nid.HcgMilliIU > 0.0f && Nid.HcgMilliIU < 5.0f);
	TestTrue(TEXT("Tag 7,5: zweiblättrige Keimscheibe"), Nid.EpiblastCells > 0 && Nid.HypoblastCells > 0);
	TestTrue(TEXT("Tag 7,5: noch keine Lakunen"), Nid.Lacunae == 0);

	AdvanceTo(State, 216.0);
	AddInfo(FString::Printf(TEXT("Tag 9: %s, versunken %.0f %%, Lakunen %d, Amnion %.0f %%, Dottersack %.0f %%, hCG %.1f"),
		*GetImplantationPhaseName(Nid.Phase), 100.0f * Nid.Embedded, Nid.Lacunae, 100.0f * Nid.AmnioticCavity, 100.0f * Nid.PrimaryYolkSac, Nid.HcgMilliIU));
	TestTrue(TEXT("Tag 9: Lakunenstadium"), Nid.Phase == EGenesisImplantationPhase::Lacunar);
	TestTrue(TEXT("Tag 9: Lakunen offen, noch ohne Blut"), Nid.Lacunae > 0 && Nid.LacunarBlood == 0.0f);
	TestTrue(TEXT("Tag 9: Amnionhöhle offen"), Nid.AmnioticCavity > 0.99f);
	TestTrue(TEXT("Tag 9: primärer Dottersack"), Nid.PrimaryYolkSac > 0.4f);
	TestTrue(TEXT("Tag 9: fast ganz versunken"), Nid.Embedded > 0.7f && Nid.Embedded < 1.0f);

	AdvanceTo(State, 240.0);
	TestTrue(TEXT("Tag 10: ganz eingebettet"), Nid.Phase == EGenesisImplantationPhase::Embedded && Nid.Embedded > 0.99f);
	TestTrue(TEXT("Tag 10: Fibrinpfropf, Epithel noch offen"), Nid.SurfaceClosure >= 0.5f && Nid.SurfaceClosure < 0.75f);
	TestTrue(TEXT("Tag 10: hCG im Blut messbar (ab 5 mIU/ml)"), Nid.HcgMilliIU >= 5.0f);
	const float HcgDay10 = Nid.HcgMilliIU;

	AdvanceTo(State, 288.0);
	TestTrue(TEXT("Tag 12: uteroplazentarer Kreislauf"), Nid.Phase == EGenesisImplantationPhase::Uteroplacental);
	TestTrue(TEXT("Tag 12: mütterliches Blut in den Lakunen"), Nid.LacunarBlood > 0.9f);
	TestTrue(TEXT("Tag 12: Oberfläche wieder geschlossen"), Nid.SurfaceClosure > 0.99f);
	TestTrue(TEXT("Tag 12: extraembryonales Mesoderm"), Nid.ExtraembryonicMesoderm > 0.99f);
	// Barnhart 2004: Eine intakte frühe Schwangerschaft steigt in 48 h um mindestens 53 %; typisch Verdopplung in 1,3–2 Tagen
	const float Rise = Nid.HcgMilliIU / HcgDay10;
	AddInfo(FString::Printf(TEXT("hCG Tag 10 → 12: %.1f → %.1f mIU/ml (×%.2f)"), HcgDay10, Nid.HcgMilliIU, Rise));
	TestTrue(TEXT("hCG steigt in 48 h klinisch (×2–4)"), Rise >= 2.0f && Rise <= 4.0f);

	AdvanceTo(State, 312.0);
	AddInfo(FString::Printf(TEXT("Tag 13: %s, Keim %.0f µm, Tiefe %.0f µm, Scheibe %.0f µm (Epiblast %d, Hypoblast %d), Zotten %d, hCG %.1f"),
		*GetImplantationPhaseName(Nid.Phase), Nid.ConceptusDiameterUm, Nid.DepthUm, Nid.DiscDiameterUm,
		Nid.EpiblastCells, Nid.HypoblastCells, Nid.PrimaryVilli, Nid.HcgMilliIU));
	TestTrue(TEXT("Tag 13: Primärzotten, eingenistet"), Nid.Phase == EGenesisImplantationPhase::PrimaryVilli && State.Stage == EGenesisEmbryoStage::Implanted);
	TestTrue(TEXT("Tag 13: Zotten"), Nid.PrimaryVilli > 0);
	TestTrue(TEXT("Tag 13: sekundärer Dottersack ersetzt den primären"), Nid.SecondaryYolkSac > 0.99f && Nid.PrimaryYolkSac < 0.01f);
	TestTrue(TEXT("Carnegie 6: Keimscheibe 0,1–0,25 mm"), Nid.DiscDiameterUm >= 100.0f && Nid.DiscDiameterUm <= 250.0f);
	TestTrue(TEXT("Keim um 1 mm"), Nid.ConceptusDiameterUm >= 800.0f && Nid.ConceptusDiameterUm <= 1500.0f);
	TestTrue(TEXT("Urintest (25 mIU/ml) schlägt um Tag 12–13 an"), Nid.HcgMilliIU >= 25.0f && Nid.HcgMilliIU < 100.0f);
	TestTrue(TEXT("Hunderte bis wenige Tausend Epiblastzellen"), Nid.EpiblastCells >= 300 && Nid.EpiblastCells <= 5000);

	// Nach der Einnistung steigt das hCG weiter
	AdvanceTo(State, 360.0);
	TestTrue(TEXT("hCG steigt nach der Einnistung weiter"), Nid.HcgMilliIU > 60.0f);
	return true;
}

/**
 * Eine Rechnung, egal in welchen Schritten: Die zweite Woche folgt der Uhr, nicht der Schrittweite –
 * dasselbe Ergebnis in einem Rutsch, in krummen Schritten und nach dem Laden.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoSecondWeekStepTest, "Genesis.Embryo.SecondWeekStepIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoSecondWeekStepTest::RunTest(const FString& Parameters)
{
	FGenesisEmbryoState Once = MakeHatched(5, 150.0f, true);
	FGenesisEmbryoState Many = Once;
	AdvanceTo(Once, 260.0);
	while (Many.HoursSinceFusion < 259.5)
	{
		GenesisEmbryoLogic::Advance(Many, FGenesisEmbryoTuning(), 0.37);
	}
	AdvanceTo(Many, 260.0);
	TestEqual(TEXT("Tiefe"), Once.Nidation.DepthUm, Many.Nidation.DepthUm, 0.5f);
	TestEqual(TEXT("Lakunen"), Once.Nidation.Lacunae, Many.Nidation.Lacunae);
	TestEqual(TEXT("hCG"), Once.Nidation.HcgMilliIU, Many.Nidation.HcgMilliIU, 0.05f);
	TestTrue(TEXT("Stufe"), Once.Nidation.Phase == Many.Nidation.Phase);
	// Wer sich 6 h später anlegt, ist 6 h später dran
	FGenesisEmbryoState Late = MakeHatched(5, 156.0f, true);
	AdvanceTo(Late, 266.0);
	TestEqual(TEXT("Später Keim: dieselbe Tiefe 6 h später"), Late.Nidation.DepthUm, Once.Nidation.DepthUm, 0.5f);
	return true;
}

/**
 * Das Risiko der Einnistung gegen Wilcox 1999: Der Zeitpunkt entscheidet. Je 1000 Keime zu zwei Zeitpunkten;
 * der Keim des Spielers scheitert nie.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoImplantationRiskTest, "Genesis.Embryo.ImplantationRisk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoImplantationRiskTest::RunTest(const FString& Parameters)
{
	using namespace GenesisEmbryoLogic;
	TestEqual(TEXT("Tag 9: 13 %"), EarlyLossRiskForDay(9.0f), 0.13f, 0.001f);
	TestEqual(TEXT("Tag 10: 26 %"), EarlyLossRiskForDay(10.0f), 0.26f, 0.001f);
	TestEqual(TEXT("Tag 11: 52 %"), EarlyLossRiskForDay(11.0f), 0.52f, 0.001f);
	TestEqual(TEXT("ab Tag 12: 82 %"), EarlyLossRiskForDay(13.0f), 0.82f, 0.001f);

	const FGenesisEmbryoTuning Tuning;
	const FGenesisEmbryoState Median = MakeHatched(1, Tuning.NominalAppositionHours, false);
	TestEqual(TEXT("Mittlerer Keim nistet an Tag 9 nach dem Eisprung ein (Wilcox-Median)"), ImplantationDayPostOvulation(Median, Tuning), 9.0f, 0.01f);

	auto FailShare = [this](float AppositionAt, bool bPlayer, float& OutMeanRisk)
	{
		int32 Failed = 0;
		double RiskSum = 0.0;
		const int32 Count = 1000;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FGenesisEmbryoState State = MakeHatched(1000 + Index, AppositionAt, bPlayer);
			AdvanceTo(State, AppositionAt + 200.0f);
			const bool bFailed = State.Stage == EGenesisEmbryoStage::Arrested;
			Failed += bFailed ? 1 : 0;
			RiskSum += State.Nidation.EarlyLossRisk;
			if (bFailed && (State.ArrestReason != EGenesisEmbryoArrestReason::ImplantationFailed || State.Nidation.HcgMilliIU != 0.0f))
			{
				AddError(TEXT("Gescheitert, aber nicht als gescheiterte Einnistung (oder hCG bleibt)"));
			}
		}
		OutMeanRisk = static_cast<float>(RiskSum / Count);
		return static_cast<float>(Failed) / Count;
	};

	float RiskEarly = 0.0f, RiskLate = 0.0f, RiskPlayer = 0.0f;
	const float Early = FailShare(144.0f, false, RiskEarly);
	const float Late = FailShare(144.0f + 48.0f, false, RiskLate);
	const float Player = FailShare(144.0f + 48.0f, true, RiskPlayer);
	AddInfo(FString::Printf(TEXT("Anlage Tag 6: %.1f %% gescheitert (Risiko %.1f %%), Tag 8: %.1f %% (Risiko %.1f %%), Spieler: %.1f %%"),
		100.0f * Early, 100.0f * RiskEarly, 100.0f * Late, 100.0f * RiskLate, 100.0f * Player));
	TestTrue(TEXT("Rechtzeitig: um 13 % (±6)"), FMath::Abs(Early - 0.13f) <= 0.06f);
	TestTrue(TEXT("Zwei Tage zu spät (Tag 11): etwa jede zweite verloren"), Late >= 0.45f && Late <= 0.7f);
	TestTrue(TEXT("Anteil folgt dem Risiko"), FMath::Abs(Early - RiskEarly) <= 0.03f && FMath::Abs(Late - RiskLate) <= 0.04f);
	TestEqual(TEXT("Der Keim des Spielers scheitert nie"), Player, 0.0f);
	return true;
}

/**
 * Die Uhr der ersten Woche gegen die Klinik: Mediane aus IVF-Zeitraffer-Aufnahmen (340 Keime, HROpen 2024).
 * 60 Keime unterschiedlicher Lebenskraft; jeder Median muss auf 10 % (mindestens 4 h) an der Klinik liegen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoClinicalTimingsTest, "Genesis.Embryo.ClinicalTimings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoClinicalTimingsTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;
	struct FMark { const TCHAR* Name; double Clinic; TArray<double> Seen; };
	FMark Marks[] = {
		{ TEXT("t2"), 25.8 }, { TEXT("t4"), 38.3 }, { TEXT("t8"), 58.7 },
		{ TEXT("Kompaktierung"), 80.2 }, { TEXT("Blastulation"), 99.0 }, { TEXT("volle Blastozyste"), 109.9 } };

	for (int32 Seed = 0; Seed < 60; ++Seed)
	{
		FGenesisEmbryoState State = MakeEmbryo(500 + Seed, 0.5f + 0.4f * (Seed % 10) / 9.0f);
		bool bDone[UE_ARRAY_COUNT(Marks)] = {};
		for (int32 Step = 0; Step < 200 * 4; ++Step)
		{
			GenesisEmbryoLogic::Advance(State, Tuning, 0.25);
			const bool Reached[] = { State.GetCellCount() >= 2, State.GetCellCount() >= 4, State.GetCellCount() >= 8,
				State.Stage >= EGenesisEmbryoStage::Morula, State.Stage >= EGenesisEmbryoStage::Blastocyst, State.Cavity >= 0.5f };
			for (int32 Mark = 0; Mark < UE_ARRAY_COUNT(Marks); ++Mark)
			{
				if (Reached[Mark] && !bDone[Mark])
				{
					bDone[Mark] = true;
					Marks[Mark].Seen.Add(State.HoursSinceFusion);
				}
			}
		}
	}

	for (FMark& Mark : Marks)
	{
		Mark.Seen.Sort();
		const double Median = Mark.Seen.Num() > 0 ? Mark.Seen[Mark.Seen.Num() / 2] : 0.0;
		const double Tolerance = FMath::Max(4.0, 0.1 * Mark.Clinic);
		AddInfo(FString::Printf(TEXT("%s: Modell %.1f h, Klinik %.1f h"), Mark.Name, Median, Mark.Clinic));
		TestTrue(FString::Printf(TEXT("%s im klinischen Bereich"), Mark.Name), FMath::Abs(Median - Mark.Clinic) <= Tolerance);
	}
	return true;
}

/** Vorkerne erscheinen und verschwinden zur richtigen Zeit; vor jeder Teilung ist der Kern weg. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEmbryoNucleiTest, "Genesis.Embryo.NucleiVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGenesisEmbryoNucleiTest::RunTest(const FString& Parameters)
{
	const FGenesisEmbryoTuning Tuning;
	FGenesisEmbryoState State = MakeEmbryo(31, 0.8f);
	float Visibility = 0.0f;
	bool bPronuclei = false;

	GenesisEmbryoLogic::Advance(State, Tuning, 4.0);
	GenesisEmbryoLogic::GetNucleusDisplay(State, 0, Tuning, Visibility, bPronuclei);
	TestTrue(TEXT("4 h: noch keine Vorkerne"), Visibility < 0.01f);

	GenesisEmbryoLogic::Advance(State, Tuning, 12.0); // 16 h
	GenesisEmbryoLogic::GetNucleusDisplay(State, 0, Tuning, Visibility, bPronuclei);
	TestTrue(TEXT("16 h: zwei Vorkerne sichtbar"), bPronuclei && Visibility > 0.99f);

	GenesisEmbryoLogic::Advance(State, Tuning, 8.0); // 24 h
	GenesisEmbryoLogic::GetNucleusDisplay(State, 0, Tuning, Visibility, bPronuclei);
	TestTrue(TEXT("24 h: Vorkerne aufgelöst (Syngamie)"), Visibility < 0.01f);

	GenesisEmbryoLogic::Advance(State, Tuning, 8.0); // 32 h, zwei Zellen
	bool bSawHidden = false;
	bool bSawVisible = false;
	for (int32 Step = 0; Step < 4 * 40; ++Step)
	{
		GenesisEmbryoLogic::Advance(State, Tuning, 0.25);
		for (int32 Index = 0; Index < State.GetCellCount(); ++Index)
		{
			GenesisEmbryoLogic::GetNucleusDisplay(State, Index, Tuning, Visibility, bPronuclei);
			TestFalse(TEXT("Furchungszellen haben keine Vorkerne"), bPronuclei);
			bSawHidden |= Visibility < 0.01f;
			bSawVisible |= Visibility > 0.99f;
		}
	}
	TestTrue(TEXT("Kerne sichtbar und vor der Teilung aufgelöst"), bSawHidden && bSawVisible);
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
