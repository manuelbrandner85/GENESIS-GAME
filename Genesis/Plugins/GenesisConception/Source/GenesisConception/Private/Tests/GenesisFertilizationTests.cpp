// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisFertilizationLogic.h"
#include "GenesisSpermSwimLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisFertilizationTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float StepSeconds = 1.0f / 120.0f;

	FGenesisOviductChannel MakeChannel()
	{
		FGenesisOviductChannel Channel;
		Channel.LumenRadiusUm = 450.0f;
		Channel.LengthUm = 3000.0f;
		Channel.WallFlowSpeedUm = 25.0f;
		return Channel;
	}

	FGenesisOocyteState MakeOocyte()
	{
		FGenesisOocyteState Oocyte;
		Oocyte.Position = FVector(1500.0, 0.0, 0.0);
		return Oocyte;
	}

	/** Zellen rund um die Eizelle verteilen (wie kurz vor der Begegnung im Eileiter). */
	TArray<FGenesisSpermCell> MakeCellsAround(int32 Count, const FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& Tuning, uint64 SeedBase, float SpreadUm, EGenesisSpermMotility* ForceMotility = nullptr)
	{
		TArray<FGenesisSpermCell> Cells;
		Cells.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(SeedBase + Index, 0.85f, Channel, Tuning);
			FGenesisRandomStream Placement(SeedBase * 31 + Index);
			const double CosTheta = 2.0 * Placement.NextDouble() - 1.0;
			const double SinTheta = FMath::Sqrt(FMath::Max(0.0, 1.0 - CosTheta * CosTheta));
			const double Phi = 2.0 * UE_DOUBLE_PI * Placement.NextDouble();
			const double Radius = Oocyte.CumulusRadiusUm + 20.0 + Placement.NextDouble() * SpreadUm;
			Cell.Position = Oocyte.Position + FVector(CosTheta, SinTheta * FMath::Cos(Phi), SinTheta * FMath::Sin(Phi)) * Radius;
			if (ForceMotility)
			{
				GenesisSpermSwimLogic::ApplyMotility(Cell, *ForceMotility, Tuning);
			}
			Cells.Add(Cell);
		}
		return Cells;
	}

	struct FRunOutcome
	{
		bool bFertilized = false;
		float Seconds = 0.0f;
		int32 Fused = 0;
		int32 Blocked = 0;
		int32 Bound = 0;
		FGenesisFertilizationResult Result;
	};

	FRunOutcome Run(TArray<FGenesisSpermCell>& Cells, FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& SwimTuning, const FGenesisFertilizationTuning& Tuning, float Seconds)
	{
		FRunOutcome Outcome;
		const int32 Steps = FMath::RoundToInt(Seconds / StepSeconds);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			if (GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, StepSeconds, Outcome.Result) && !Outcome.bFertilized)
			{
				Outcome.bFertilized = true;
				Outcome.Seconds = Step * StepSeconds;
			}
		}
		for (const FGenesisSpermCell& Cell : Cells)
		{
			switch (GenesisFertilizationLogic::GetPhase(Cell))
			{
			case EGenesisSpermPhase::Fused: ++Outcome.Fused; break;
			case EGenesisSpermPhase::Blocked: ++Outcome.Blocked; break;
			case EGenesisSpermPhase::Bound:
			case EGenesisSpermPhase::Penetrating: ++Outcome.Bound; break;
			default: break;
			}
		}
		return Outcome;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationWinnerTest, "Genesis.Conception.Fertilization.SingleWinnerAndBlock", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationWinnerTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	FGenesisSpermSwimTuning SwimTuning;
	FGenesisFertilizationTuning Tuning;
	FGenesisOocyteState Oocyte = MakeOocyte();

	TArray<FGenesisSpermCell> Cells = MakeCellsAround(200, Oocyte, Channel, SwimTuning, 4000, 160.0f);
	const FRunOutcome Outcome = Run(Cells, Oocyte, Channel, SwimTuning, Tuning, 900.0f);

	AddInfo(FString::Printf(TEXT("Befruchtet: %s nach %.0f s | verschmolzen %d | abgewiesen %d | gleichzeitig gebunden (max) %d | Vitalität des Siegers %.2f"),
		Outcome.bFertilized ? TEXT("ja") : TEXT("nein"), Outcome.Seconds, Outcome.Fused, Outcome.Blocked, Outcome.Result.CompetingCells, Outcome.Result.Vitality));

	TestTrue(TEXT("Die Eizelle wird befruchtet"), Outcome.bFertilized && Oocyte.IsFertilized());
	TestEqual(TEXT("Genau eine Zelle verschmilzt (kein Polyspermie-Fehler)"), Outcome.Fused, 1);
	TestTrue(TEXT("Weitere Zellen erreichen die Zona und werden abgewiesen"), Outcome.Blocked > 0);
	TestTrue(TEXT("Nach der Cortikalreaktion ist die Zona verhärtet"), Oocyte.bZonaHardened && Oocyte.CorticalReaction > 0.9f);
	TestTrue(TEXT("Dauer plausibel (10–900 s Simulationszeit)"), Outcome.Seconds > 10.0f && Outcome.Seconds < 900.0f);
	TestTrue(TEXT("Keine Zelle hängt nach dem Block noch in der Zona"), Outcome.Bound == 0);

	// Determinismus
	FGenesisOocyteState SecondOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> SecondCells = MakeCellsAround(200, SecondOocyte, Channel, SwimTuning, 4000, 160.0f);
	const FRunOutcome Again = Run(SecondCells, SecondOocyte, Channel, SwimTuning, Tuning, 900.0f);
	TestEqual(TEXT("Deterministisch: derselbe Sieger"), Again.Result.CellIndex, Outcome.Result.CellIndex);
	TestEqual(TEXT("Deterministisch: dieselbe Zeit"), Again.Seconds, Outcome.Seconds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationCapacitationTest, "Genesis.Conception.Fertilization.CapacitationMatters", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationCapacitationTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	FGenesisSpermSwimTuning SwimTuning;
	// Keine Wechsel: Jede Gruppe bleibt in ihrer Bewegungsart
	SwimTuning.HyperactivationRate = 0.0f;
	SwimTuning.DeactivationRate = 0.0f;
	FGenesisFertilizationTuning Tuning;

	EGenesisSpermMotility Hyper = EGenesisSpermMotility::Hyperactivated;
	EGenesisSpermMotility Progressive = EGenesisSpermMotility::Progressive;

	FGenesisOocyteState HyperOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> HyperCells = MakeCellsAround(120, HyperOocyte, Channel, SwimTuning, 5000, 120.0f, &Hyper);
	const FRunOutcome HyperRun = Run(HyperCells, HyperOocyte, Channel, SwimTuning, Tuning, 600.0f);

	FGenesisOocyteState ProgressiveOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> ProgressiveCells = MakeCellsAround(120, ProgressiveOocyte, Channel, SwimTuning, 5000, 120.0f, &Progressive);
	const FRunOutcome ProgressiveRun = Run(ProgressiveCells, ProgressiveOocyte, Channel, SwimTuning, Tuning, 600.0f);

	AddInfo(FString::Printf(TEXT("Hyperaktiviert: %s nach %.0f s | nur progressiv: %s nach %.0f s"),
		HyperRun.bFertilized ? TEXT("befruchtet") : TEXT("nicht befruchtet"), HyperRun.Seconds,
		ProgressiveRun.bFertilized ? TEXT("befruchtet") : TEXT("nicht befruchtet"), ProgressiveRun.Seconds));

	TestTrue(TEXT("Hyperaktivierte Zellen befruchten"), HyperRun.bFertilized);
	TestFalse(TEXT("Ohne Kapazitation keine Befruchtung"), ProgressiveRun.bFertilized);
	TestEqual(TEXT("Ohne Kapazitation bindet keine Zelle"), ProgressiveRun.Bound + ProgressiveRun.Fused, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationLateComerTest, "Genesis.Conception.Fertilization.LateComersStayOutside", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationLateComerTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	FGenesisSpermSwimTuning SwimTuning;
	FGenesisFertilizationTuning Tuning;

	// Eizelle ist bereits befruchtet und die Zona verhärtet
	FGenesisOocyteState Oocyte = MakeOocyte();
	Oocyte.FertilizedByCell = 999;
	Oocyte.SecondsSinceFusion = Tuning.CorticalReactionSeconds;
	Oocyte.CorticalReaction = 1.0f;
	Oocyte.bZonaHardened = true;

	TArray<FGenesisSpermCell> Cells = MakeCellsAround(120, Oocyte, Channel, SwimTuning, 6000, 100.0f);
	const FRunOutcome Outcome = Run(Cells, Oocyte, Channel, SwimTuning, Tuning, 400.0f);

	AddInfo(FString::Printf(TEXT("Nachzügler: verschmolzen %d | abgewiesen %d"), Outcome.Fused, Outcome.Blocked));
	TestEqual(TEXT("Keine zweite Verschmelzung"), Outcome.Fused, 0);
	TestTrue(TEXT("Nachzügler werden abgewiesen"), Outcome.Blocked > 0);
	TestEqual(TEXT("Die befruchtende Zelle bleibt eingetragen"), Oocyte.FertilizedByCell, 999);
	return true;
}

#endif
