// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceLogic.h"
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

	// Die Einnistung – aber die Schwangerschaft hat noch nicht einmal begonnen
	Signals.bImplanted = true;
	Signals.GestationalWeeks = 1.0f;
	Phase = Advance(Phase, Signals, Tuning, Path, Ending);
	TestEqual(TEXT("Nach der Einnistung kommt die Schwangerschaft"), static_cast<int32>(Phase), static_cast<int32>(EGenesisSlicePhase::Gestation));

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

#endif
