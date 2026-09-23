// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceLogic.h"
#include "GenesisEmbryoLogic.h"
#include "GenesisEmbryogenesisLogic.h"
#include "Engine/FontFace.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSliceTests
{
	constexpr EAutomationTestFlags SliceFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Führt die Regie so lange weiter, wie sich etwas ändert, und schreibt den Weg mit. */
	EGenesisSlicePhase Advance(EGenesisSlicePhase Phase, const FGenesisSliceSignals& Signals,
		const FGenesisSliceTuning& Tuning, TArray<EGenesisSlicePhase>& OutPath, EGenesisSliceEnding& OutEnding)
	{
		for (int32 Guard = 0; Guard < 16; ++Guard)
		{
			EGenesisSliceEnding Ending = EGenesisSliceEnding::None;
			const EGenesisSlicePhase Next = GenesisSliceLogic::NextPhase(Phase, Signals, Tuning, Ending);
			if (Next == Phase)
			{
				return Phase;
			}
			Phase = Next;
			OutEnding = Ending;
			OutPath.Add(Phase);
		}
		return Phase;
	}
}

/**
 * Ein Leben am Stück: Keine Phase darf übersprungen werden, und keine darf zu früh kommen.
 * Das ist die Kernbehauptung dieses Blocks.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceOrderTest, "Genesis.Slice.PhasesInOrder", GenesisSliceTests::SliceFlags)

bool FGenesisSliceOrderTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSliceTests;

	const FGenesisSliceTuning Tuning;
	FGenesisSliceSignals Signals;
	TArray<EGenesisSlicePhase> Path;
	EGenesisSliceEnding Ending = EGenesisSliceEnding::None;

	// Nichts ist geschehen: Die Regie wartet im Eileiter
	EGenesisSlicePhase Phase = Advance(EGenesisSlicePhase::Idle, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Der Durchlauf beginnt mit der Befruchtung"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Conception));

	// Die Verschmelzung
	Signals.bConceived = true;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Nach der Verschmelzung kommt die erste Woche"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Embryo));

	// Eingenistet allein reicht nicht: Erst kommen Keimblätter, Neuralrohr und der erste Herzschlag (GENESIS-041)
	Signals.bImplanted = true;
	Signals.GestationalWeeks = 2.0f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Eingenistet: die ersten Wochen laufen weiter"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Embryo));

	// Ende der vierten Woche: Der Bauplan steht, die Körpersimulation übernimmt
	Signals.bBodyPlanDone = true;
	Signals.GestationalWeeks = 4.0f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Nach dem Bauplan kommt die Schwangerschaft"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Gestation));

	// Mitten in der Schwangerschaft passiert nichts Neues
	Signals.GestationalWeeks = 24.0f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Mit 24 Wochen bleibt es bei der Schwangerschaft"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Gestation));

	// Am Termin
	Signals.GestationalWeeks = 39.2f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Am Termin beginnt die Geburt"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Birth));

	// Geboren
	Signals.bBorn = true;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Nach der Geburt kommt die erste Stunde"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::FirstHour));

	// Das Kind schläft ein
	Signals.bAsleep = true;
	Signals.MinutesSinceBirth = 48.0f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Der Durchlauf endet, wenn das Kind schläft"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Complete));
	TestEqual(TEXT("Und zwar mit dem vorgesehenen Schluss"), static_cast<int32>(Ending), static_cast<int32>(EGenesisSliceEnding::Asleep));

	FString Names;
	for (EGenesisSlicePhase Step : Path)
	{
		Names += (Names.IsEmpty() ? TEXT("") : TEXT(" → ")) + GenesisSliceLogic::GetPhaseName(Step);
	}
	AddInfo(FString::Printf(TEXT("Weg: %s"), *Names));
	TestEqual(TEXT("Sechs Übergänge, keiner übersprungen"), Path.Num(), 6);

	return true;
}

/** Ein Keim, der sich nicht weiterentwickelt, beendet den Durchlauf – ohne Beschönigung. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceArrestTest, "Genesis.Slice.ArrestEndsTheRun", GenesisSliceTests::SliceFlags)

bool FGenesisSliceArrestTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSliceTests;

	const FGenesisSliceTuning Tuning;
	FGenesisSliceSignals Signals;
	Signals.bConceived = true;
	Signals.bEmbryoArrested = true;

	EGenesisSliceEnding Ending = EGenesisSliceEnding::None;
	const EGenesisSlicePhase Phase = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::Embryo, Signals, Tuning, Ending);

	AddInfo(FString::Printf(TEXT("Aus der ersten Woche: %s (%s)"),
		*GenesisSliceLogic::GetPhaseName(Phase), *GenesisSliceLogic::GetEndingName(Ending)));

	TestEqual(TEXT("Der Durchlauf endet"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Ended));
	TestEqual(TEXT("Und sagt warum"), static_cast<int32>(Ending), static_cast<int32>(EGenesisSliceEnding::EmbryoArrested));

	// Ein Kind, das die Geburt nicht überlebt, ebenso
	FGenesisSliceSignals Stillborn;
	Stillborn.bConceived = true;
	Stillborn.bImplanted = true;
	Stillborn.bBodyPlanDone = true;
	Stillborn.bBorn = true;
	Stillborn.bAlive = false;
	EGenesisSliceEnding BirthEnding = EGenesisSliceEnding::None;
	const EGenesisSlicePhase AfterBirth = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::Birth, Stillborn, Tuning, BirthEnding);
	TestEqual(TEXT("Auch die Geburt kann enden"), static_cast<int32>(AfterBirth), static_cast<int32>(EGenesisSlicePhase::Ended));
	TestEqual(TEXT("Mit dem richtigen Grund"), static_cast<int32>(BirthEnding), static_cast<int32>(EGenesisSliceEnding::NotAlive));

	return true;
}

/**
 * Die erste Stunde braucht ein Ende – auch dann, wenn niemand das Kind auf die Haut legt
 * und es deshalb nicht zur Ruhe kommt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceFirstHourTest, "Genesis.Slice.FirstHourAlwaysEnds", GenesisSliceTests::SliceFlags)

bool FGenesisSliceFirstHourTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSliceTests;

	const FGenesisSliceTuning Tuning;

	FGenesisSliceSignals Restless;
	Restless.bConceived = true;
	Restless.bImplanted = true;
	Restless.bBodyPlanDone = true;
	Restless.bBorn = true;
	Restless.bAsleep = false;
	Restless.MinutesSinceBirth = 40.0f;

	EGenesisSliceEnding Ending = EGenesisSliceEnding::None;
	EGenesisSlicePhase Phase = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::FirstHour, Restless, Tuning, Ending);
	TestEqual(TEXT("Nach 40 Minuten läuft die erste Stunde noch"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::FirstHour));

	Restless.MinutesSinceBirth = Tuning.FirstHourLimitMinutes + 1.0f;
	Phase = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::FirstHour, Restless, Tuning, Ending);
	AddInfo(FString::Printf(TEXT("Nach %.0f Minuten ohne Schlaf: %s"), Restless.MinutesSinceBirth, *GenesisSliceLogic::GetPhaseName(Phase)));
	TestEqual(TEXT("Spätestens nach der vorgesehenen Zeit ist Schluss"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Complete));

	return true;
}

/** Ein früher Termin ist keiner: Die Geburt beginnt nicht in der 30. Woche. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceTermTest, "Genesis.Slice.BirthWaitsForTerm", GenesisSliceTests::SliceFlags)

bool FGenesisSliceTermTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSliceTests;

	FGenesisSliceTuning Tuning;
	FGenesisSliceSignals Signals;
	Signals.bConceived = true;
	Signals.bImplanted = true;
	Signals.bBodyPlanDone = true;

	EGenesisSliceEnding Ending = EGenesisSliceEnding::None;
	for (const float Weeks : { 8.0f, 20.0f, 30.0f, 36.9f })
	{
		Signals.GestationalWeeks = Weeks;
		const EGenesisSlicePhase Phase = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::Gestation, Signals, Tuning, Ending);
		TestEqual(FString::Printf(TEXT("Woche %.1f: noch keine Geburt"), Weeks),
			static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Gestation));
	}

	// Wer eine Frühgeburt erzählen will, stellt den Termin um – die Regel bleibt dieselbe
	Tuning.BirthAtWeeks = 32.0f;
	Signals.GestationalWeeks = 32.5f;
	const EGenesisSlicePhase Early = GenesisSliceLogic::NextPhase(EGenesisSlicePhase::Gestation, Signals, Tuning, Ending);
	AddInfo(TEXT("Mit umgestelltem Termin (32 Wochen) beginnt die Geburt in Woche 32,5"));
	TestEqual(TEXT("Der Termin ist eine Stellschraube, keine feste Zahl"),
		static_cast<int32>(Early), static_cast<int32>(EGenesisSlicePhase::Birth));

	return true;
}

/**
 * Die erste Woche als Zeitraffer (GENESIS-038): sichtbar, aber nicht zäh. Vorher liefen zehn Tage in
 * zehn Sekunden – jede Teilung war nach einem Bild vorbei. Jetzt soll man die Teilungen sehen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceEmbryoTimeLapseTest, "Genesis.Slice.EmbryoTimeLapse", GenesisSliceTests::SliceFlags)
bool FGenesisSliceEmbryoTimeLapseTest::RunTest(const FString& Parameters)
{
	const FGenesisSliceTuning Slice;
	const FGenesisEmbryoTuning EmbryoTuning;
	FGenesisEmbryoState State = GenesisEmbryoLogic::CreateZygote(FGuid(1, 2, 3, 4), FGuid(5, 6, 7, 8), 0.85f, 0.6f, FGenesisTimestamp(), EmbryoTuning);

	const float Frame = 1.0f / 60.0f;
	float RealSeconds = 0.0f;
	float HatchedAt = -1.0f;
	float LastDivision = 0.0f;
	float ShortestGap = TNumericLimits<float>::Max();
	int32 Cells = State.GetCellCount();
	while (State.Stage != EGenesisEmbryoStage::Implanted && RealSeconds < 600.0f)
	{
		const float Hours = GenesisSliceLogic::EmbryoHoursPerSecond(State.Stage, Slice) * Frame;
		GenesisEmbryoLogic::Advance(State, EmbryoTuning, Hours);
		RealSeconds += Frame;
		if (State.GetCellCount() != Cells && State.GetCellCount() <= 8)
		{
			// Bis zum 8-Zell-Stadium ist jede einzelne Teilung ein Ereignis, das man sehen soll
			if (Cells > 1)
			{
				ShortestGap = FMath::Min(ShortestGap, RealSeconds - LastDivision);
			}
			LastDivision = RealSeconds;
		}
		Cells = State.GetCellCount();
		if (HatchedAt < 0.0f && State.Stage >= EGenesisEmbryoStage::Implanting)
		{
			HatchedAt = RealSeconds;
		}
	}
	AddInfo(FString::Printf(TEXT("Bis zur Einnistung %.0f s, bis eingenistet %.0f s, kürzester Abstand zweier sichtbarer Teilungsereignisse %.1f s"),
		HatchedAt, RealSeconds, ShortestGap));
	TestTrue(TEXT("Die Woche ist zu sehen (mindestens eine Minute bis zur Einnistung)"), HatchedAt >= 60.0f);
	TestTrue(TEXT("Und nicht zäh (höchstens zwei Minuten bis eingenistet)"), RealSeconds <= 120.0f);
	TestTrue(TEXT("Eingenistet"), State.Stage == EGenesisEmbryoStage::Implanted);
	return true;
}

/**
 * Der Ort folgt dem Keim (GENESIS-040): bis zum Schlüpfen der Eileiter, danach die Gebärmutter, eingenistet bleibt er dort.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceEmbryoMapTest, "Genesis.Slice.EmbryoMap", GenesisSliceTests::SliceFlags)
bool FGenesisSliceEmbryoMapTest::RunTest(const FString& Parameters)
{
	const FGenesisSliceTuning Tuning;
	TestEqual(TEXT("Morula im Eileiter"), GenesisSliceLogic::MapForEmbryoStage(EGenesisEmbryoStage::Morula, Tuning), Tuning.ConceptionMap);
	TestEqual(TEXT("Beim Schlüpfen noch im Eileiter"), GenesisSliceLogic::MapForEmbryoStage(EGenesisEmbryoStage::Hatching, Tuning), Tuning.ConceptionMap);
	TestEqual(TEXT("Geschlüpft: Gebärmutter"), GenesisSliceLogic::MapForEmbryoStage(EGenesisEmbryoStage::Implanting, Tuning), Tuning.ImplantationMap);
	TestEqual(TEXT("Eingenistet: Gebärmutter"), GenesisSliceLogic::MapForEmbryoStage(EGenesisEmbryoStage::Implanted, Tuning), Tuning.ImplantationMap);
	TestEqual(TEXT("Stillstand: kein Ortswechsel"), GenesisSliceLogic::MapForEmbryoStage(EGenesisEmbryoStage::Arrested, Tuning), Tuning.ConceptionMap);
	return true;
}

/**
 * Die Titelschrift des Covers (Cinzel, SIL OFL) muss im Projekt liegen: Sie haengt nur am HUD-Code, kein Asset verweist
 * auf sie. Fehlt sie, faellt der Startbildschirm still auf die Engine-Schrift zurueck (Docs/31_Bildsprache.md).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFrontendTitleFontTest, "Genesis.Frontend.TitleFont", GenesisSliceTests::SliceFlags)
bool FGenesisFrontendTitleFontTest::RunTest(const FString& Parameters)
{
	const UFontFace* Face = LoadObject<UFontFace>(nullptr, TEXT("/Game/Genesis/UI/Fonts/FF_GEN_Cinzel.FF_GEN_Cinzel"));
	TestNotNull(TEXT("Schriftschnitt der Titelschrift liegt im Projekt"), Face);
	if (Face)
	{
		AddInfo(FString::Printf(TEXT("Titelschrift: %s"), *Face->GetPathName()));
	}
	return true;
}

/**
 * Die dritte und vierte Woche im Zeitraffer (GENESIS-041): Vom eingenisteten Keim bis zum fertigen Bauplan
 * sollen es gut dreißig Sekunden sein – lang genug, um den ersten Herzschlag zu erleben, kurz genug, um nicht zu zerren.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceBodyPlanPaceTest, "Genesis.Slice.BodyPlanPace", GenesisSliceTests::SliceFlags)
bool FGenesisSliceBodyPlanPaceTest::RunTest(const FString& Parameters)
{
	const FGenesisSliceTuning Slice;
	const FGenesisEmbryogenesisTuning Tuning;
	const float HoursPerSecond = GenesisSliceLogic::EmbryoHoursPerSecond(EGenesisEmbryoStage::Implanted, Slice);
	TestEqual(TEXT("Eigenes Tempo für die dritte und vierte Woche"), HoursPerSecond, Slice.BodyPlanHoursPerSecond);

	FGenesisEmbryogenesisState State;
	const float Frame = 1.0f / 60.0f;
	float Seconds = 0.0f;
	float HeartAt = -1.0f;
	float Day = 13.0f;
	while (State.Stage != EGenesisEmbryogenesisStage::Complete && Seconds < 300.0f)
	{
		Day += HoursPerSecond * Frame / 24.0f;
		GenesisEmbryogenesisLogic::Advance(State, Tuning, Day, 0.7f, true, 1);
		Seconds += Frame;
		if (HeartAt < 0.0f && State.bHeartBeating)
		{
			HeartAt = Seconds;
		}
	}
	AddInfo(FString::Printf(TEXT("Erster Herzschlag nach %.0f s, Bauplan fertig nach %.0f s, danach %.0f s Innehalten"),
		HeartAt, Seconds, Slice.BodyPlanHoldSeconds));
	TestTrue(TEXT("Der erste Herzschlag kommt nicht sofort"), HeartAt >= 12.0f);
	TestTrue(TEXT("Und der Bauplan steht in gut einer halben Minute"), Seconds >= 25.0f && Seconds <= 45.0f);
	TestTrue(TEXT("Am Ende schlägt das Herz"), State.bHeartBeating && State.HeartRateBpm > 100.0f);
	// Der Moment des Innehaltens: lang genug für einige Herzschläge in Echtzeit, kurz genug, um nicht zu stehen
	TestTrue(TEXT("Innehalten 4–20 s"), Slice.BodyPlanHoldSeconds >= 4.0f && Slice.BodyPlanHoldSeconds <= 20.0f);
	return true;
}

/**
 * Der Ort folgt dem Keim (GENESIS-040/041): Eileiter bis zum Schlüpfen, Gebärmutter ab der Einnistung, und ab dem
 * Ende der vierten Woche die Fruchthöhle mit dem Embryo selbst – nicht früher, denn der Körper ist für Tag 28 gebaut.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSliceEmbryoSceneMapTest, "Genesis.Slice.EmbryoSceneMap", GenesisSliceTests::SliceFlags)
bool FGenesisSliceEmbryoSceneMapTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSliceLogic;
	FGenesisSliceTuning Tuning;
	TestEqual(TEXT("Furchung: Eileiter"), MapForEmbryo(EGenesisEmbryoStage::Morula, 3.5f, Tuning), Tuning.ConceptionMap);
	TestEqual(TEXT("Einnistung: Gebärmutter"), MapForEmbryo(EGenesisEmbryoStage::Implanting, 8.0f, Tuning), Tuning.ImplantationMap);
	TestEqual(TEXT("Dritte Woche: noch Gebärmutter"), MapForEmbryo(EGenesisEmbryoStage::Implanted, 20.0f, Tuning), Tuning.ImplantationMap);
	TestEqual(TEXT("Kurz vor dem Ende der vierten Woche: noch Gebärmutter"),
		MapForEmbryo(EGenesisEmbryoStage::Implanted, Tuning.EmbryoSceneFromDay - 0.1f, Tuning), Tuning.ImplantationMap);
	TestEqual(TEXT("Ende der vierten Woche: Fruchthöhle"), MapForEmbryo(EGenesisEmbryoStage::Implanted, 26.5f, Tuning), Tuning.EmbryoMap);
	TestEqual(TEXT("Ein Keim im Stillstand bleibt, wo er ist"), MapForEmbryo(EGenesisEmbryoStage::Arrested, 27.0f, Tuning), Tuning.ConceptionMap);
	Tuning.EmbryoMap = NAME_None;
	TestEqual(TEXT("Ohne Fruchthöhle bleibt es bei der Gebärmutter"), MapForEmbryo(EGenesisEmbryoStage::Implanted, 27.0f, Tuning), Tuning.ImplantationMap);
	return true;
}

#endif
