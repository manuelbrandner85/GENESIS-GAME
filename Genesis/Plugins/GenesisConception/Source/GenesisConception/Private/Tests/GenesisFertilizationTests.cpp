// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisFertilizationLogic.h"
#include "GenesisSpermSwimLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisFertilizationTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float StepSeconds = 1.0f / 120.0f;
	/** Sobald eine Zelle an der Eizelle hängt, rechnet die Szene im Zeitraffer gröber (AGenesisSpermSwarm::TimeLapseStepSeconds). */
	constexpr float TimeLapseStepSeconds = 1.0f / 30.0f;

	FGenesisOviductChannel MakeChannel()
	{
		FGenesisOviductChannel Channel;
		Channel.LumenRadiusUm = 900.0f;
		Channel.LengthUm = 3000.0f;
		Channel.WallFlowSpeedUm = 25.0f;
		return Channel;
	}

	FGenesisOocyteState MakeOocyte()
	{
		FGenesisOocyteState Oocyte;
		Oocyte.Position = FVector(1500.0, 0.0, 0.0);
		// Derselbe Cumulus wie im Spiel: 13.400 Zellen als Hindernisse (GENESIS-047 Teil 2)
		Oocyte.Cumulus = GenesisFertilizationLogic::BuildCumulus(Oocyte, FGenesisCumulusTuning());
		return Oocyte;
	}

	/** Zellen rund um die Eizelle verteilen (wie kurz vor der Begegnung im Eileiter). */
	TArray<FGenesisSpermCell> MakeCellsAround(int32 Count, const FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& Tuning, uint64 SeedBase, float SpreadUm, EGenesisSpermMotility* ForceMotility = nullptr, bool bForceCapacitated = false)
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
			const double Radius = Oocyte.MatrixRadiusUm + 20.0 + Placement.NextDouble() * SpreadUm;
			Cell.Position = Oocyte.Position + FVector(CosTheta, SinTheta * FMath::Cos(Phi), SinTheta * FMath::Sin(Phi)) * Radius;
			if (bForceCapacitated)
			{
				Cell.bCapacitated = true;
			}
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
		int32 Perivitelline = 0;
		int32 BoundEver = 0;
		FGenesisFertilizationResult Result;
	};

	FRunOutcome Run(TArray<FGenesisSpermCell>& Cells, FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& SwimTuning, const FGenesisFertilizationTuning& Tuning, float Seconds)
	{
		FRunOutcome Outcome;
		TArray<bool> EverBound;
		EverBound.Init(false, Cells.Num());
		float Time = 0.0f;
		while (Time < Seconds)
		{
			// Wie im Spiel: fein, solange nur geschwommen wird; gerafft, sobald jemand an der Eizelle hängt
			const bool bAtEgg = Oocyte.BoundCells + Oocyte.PerivitellineCells > 0 || Oocyte.IsFertilized();
			const float Dt = bAtEgg ? TimeLapseStepSeconds : StepSeconds;
			if (GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, Dt, Outcome.Result) && !Outcome.bFertilized)
			{
				Outcome.bFertilized = true;
				Outcome.Seconds = Time;
			}
			for (int32 Index = 0; Index < Cells.Num(); ++Index)
			{
				const EGenesisSpermPhase Phase = GenesisFertilizationLogic::GetPhase(Cells[Index]);
				EverBound[Index] |= Phase == EGenesisSpermPhase::Bound || Phase == EGenesisSpermPhase::Penetrating;
			}
			Time += Dt;
		}
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			Outcome.BoundEver += EverBound[Index] ? 1 : 0;
			switch (GenesisFertilizationLogic::GetPhase(Cells[Index]))
			{
			case EGenesisSpermPhase::Fused: ++Outcome.Fused; break;
			case EGenesisSpermPhase::Blocked: ++Outcome.Blocked; break;
			case EGenesisSpermPhase::Perivitelline: ++Outcome.Perivitelline; break;
			case EGenesisSpermPhase::Bound:
			case EGenesisSpermPhase::Penetrating: ++Outcome.Bound; break;
			default: break;
			}
		}
		return Outcome;
	}

	/** Eine kapazitierte, hyperaktivierte Zelle direkt vor der Zona, zur Eizelle gerichtet. */
	FGenesisSpermCell MakeCellAtZona(uint64 Seed, const FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& SwimTuning, const FVector& Direction)
	{
		FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(Seed, 0.85f, Channel, SwimTuning);
		Cell.bCapacitated = true;
		GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Hyperactivated, SwimTuning);
		Cell.Position = Oocyte.Position - Direction * (Oocyte.ZonaOuterRadiusUm + 1.0);
		Cell.Heading = Direction;
		return Cell;
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
	const FRunOutcome Outcome = Run(Cells, Oocyte, Channel, SwimTuning, Tuning, 4800.0f);

	AddInfo(FString::Printf(TEXT("Befruchtet: %s nach %.1f min | an der Zona gewesen %d | verschmolzen %d | im Spalt liegen geblieben %d | abgewiesen %d | Mitbewerberinnen (max) %d"),
		Outcome.bFertilized ? TEXT("ja") : TEXT("nein"), Outcome.Seconds / 60.0f, Outcome.BoundEver, Outcome.Fused, Outcome.Perivitelline,
		Outcome.Blocked, Outcome.Result.CompetingCells));

	TestTrue(TEXT("Die Eizelle wird befruchtet"), Outcome.bFertilized && Oocyte.IsFertilized());
	TestEqual(TEXT("Genau eine Zelle verschmilzt (kein Polyspermie-Fehler)"), Outcome.Fused, 1);
	TestTrue(TEXT("Mehrere kommen an – nicht nur eine (Docs/38)"), Outcome.BoundEver >= 2);
	TestTrue(TEXT("Weitere Zellen werden abgewiesen oder bleiben im Spalt liegen"), Outcome.Blocked + Outcome.Perivitelline > 0);
	TestTrue(TEXT("Nach der Cortikalreaktion ist die Zona verändert"), Oocyte.bZonaHardened && Oocyte.CorticalReaction > 0.9f);
	// Zona rund 13 min, Spalt 16 ± 6 min: unter einer Viertelstunde ist keine Befruchtung möglich
	TestTrue(TEXT("Dauer biologisch (15–70 min)"), Outcome.Seconds > 15.0f * 60.0f && Outcome.Seconds < 70.0f * 60.0f);
	TestTrue(TEXT("Keine Zelle hängt nach dem Block noch gebunden an der Zona"), Outcome.Bound == 0);

	// Determinismus
	FGenesisOocyteState SecondOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> SecondCells = MakeCellsAround(200, SecondOocyte, Channel, SwimTuning, 4000, 160.0f);
	const FRunOutcome Again = Run(SecondCells, SecondOocyte, Channel, SwimTuning, Tuning, 4800.0f);
	TestEqual(TEXT("Deterministisch: derselbe Sieger"), Again.Result.CellIndex, Outcome.Result.CellIndex);
	TestEqual(TEXT("Deterministisch: dieselbe Zeit"), Again.Seconds, Outcome.Seconds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationCapacitationTest, "Genesis.Conception.Fertilization.CapacitationMatters", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationCapacitationTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	FGenesisFertilizationTuning Tuning;

	// Kapazitiert und hyperaktiviert, ohne Wechsel
	FGenesisSpermSwimTuning Fixed;
	Fixed.HyperactivationRate = 0.0f;
	Fixed.DeactivationRate = 0.0f;
	EGenesisSpermMotility Hyper = EGenesisSpermMotility::Hyperactivated;
	FGenesisOocyteState ReadyOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> ReadyCells = MakeCellsAround(120, ReadyOocyte, Channel, Fixed, 5000, 120.0f, &Hyper, true);
	const FRunOutcome ReadyRun = Run(ReadyCells, ReadyOocyte, Channel, Fixed, Tuning, 3600.0f);

	// Nicht kapazitiert, mit den normalen Wechselraten: Sie könnten hyperaktivieren – wenn sie bereit wären
	FGenesisSpermSwimTuning Normal;
	Normal.CapacitatedFraction = 0.0f;
	FGenesisOocyteState UnreadyOocyte = MakeOocyte();
	TArray<FGenesisSpermCell> UnreadyCells = MakeCellsAround(120, UnreadyOocyte, Channel, Normal, 5000, 120.0f);
	const FRunOutcome UnreadyRun = Run(UnreadyCells, UnreadyOocyte, Channel, Normal, Tuning, 1200.0f);
	int32 UnreadyHyper = 0;
	for (const FGenesisSpermCell& Cell : UnreadyCells)
	{
		UnreadyHyper += Cell.Motility == EGenesisSpermMotility::Hyperactivated ? 1 : 0;
	}

	// Der Anteil kapazitierter Zellen im Schwarm: 2–14 % (Cohen-Dayag 1995)
	const FGenesisSpermSwimTuning Default;
	int32 Capacitated = 0;
	constexpr int32 Sample = 2000;
	for (int32 Index = 0; Index < Sample; ++Index)
	{
		Capacitated += GenesisSpermSwimLogic::CreateCell(90000 + Index, 0.75f, Channel, Default).bCapacitated ? 1 : 0;
	}

	AddInfo(FString::Printf(TEXT("Kapazitiert: %s nach %.1f min | nicht kapazitiert: %s, gebunden %d, hyperaktiviert %d | im Schwarm kapazitiert %.1f %%"),
		ReadyRun.bFertilized ? TEXT("befruchtet") : TEXT("nicht befruchtet"), ReadyRun.Seconds / 60.0f,
		UnreadyRun.bFertilized ? TEXT("befruchtet") : TEXT("nicht befruchtet"), UnreadyRun.BoundEver, UnreadyHyper, 100.0f * Capacitated / Sample));

	TestTrue(TEXT("Kapazitierte Zellen befruchten"), ReadyRun.bFertilized);
	TestFalse(TEXT("Ohne Kapazitation keine Befruchtung"), UnreadyRun.bFertilized);
	TestEqual(TEXT("Ohne Kapazitation bindet keine Zelle"), UnreadyRun.BoundEver, 0);
	TestEqual(TEXT("Ohne Kapazitation keine Hyperaktivierung"), UnreadyHyper, 0);
	TestTrue(TEXT("Im Schwarm ist nur ein kleiner Teil kapazitiert (2–14 %)"), Capacitated > Sample * 2 / 100 && Capacitated < Sample * 14 / 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationLateComerTest, "Genesis.Conception.Fertilization.LateComersStayOutside", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationLateComerTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	FGenesisSpermSwimTuning SwimTuning;
	FGenesisFertilizationTuning Tuning;

	// Eizelle ist bereits befruchtet und die Zona verändert
	FGenesisOocyteState Oocyte = MakeOocyte();
	Oocyte.FertilizedByCell = 999;
	Oocyte.SecondsSinceFusion = Tuning.CorticalReactionSeconds;
	Oocyte.CorticalReaction = 1.0f;
	Oocyte.bZonaHardened = true;

	// Alle bereit: Es geht darum, dass selbst bereite Zellen nicht mehr hineinkommen
	TArray<FGenesisSpermCell> Cells = MakeCellsAround(120, Oocyte, Channel, SwimTuning, 6000, 100.0f, nullptr, true);
	const FRunOutcome Outcome = Run(Cells, Oocyte, Channel, SwimTuning, Tuning, 400.0f);

	AddInfo(FString::Printf(TEXT("Nachzügler: verschmolzen %d | abgewiesen %d | gebunden %d"), Outcome.Fused, Outcome.Blocked, Outcome.BoundEver));
	TestEqual(TEXT("Keine zweite Verschmelzung"), Outcome.Fused, 0);
	TestTrue(TEXT("Nachzügler werden abgewiesen"), Outcome.Blocked > 0);
	TestEqual(TEXT("Keine bindet mehr"), Outcome.BoundEver, 0);
	TestEqual(TEXT("Die befruchtende Zelle bleibt eingetragen"), Oocyte.FertilizedByCell, 999);
	return true;
}

/**
 * Die Zeiten an der Eizelle wie gemessen (Docs/38): rund 13 Minuten durch die Zona (Jin 2011), 16 ± 6 Minuten
 * im Spalt darunter bis zur Verschmelzung (Dubois 2025), der Schutz vor einer zweiten Zelle über Minuten –
 * und wer zu spät in den Spalt kommt, bleibt dort liegen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationTimelineTest, "Genesis.Conception.Fertilization.RealTimeline", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationTimelineTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	const FGenesisSpermSwimTuning SwimTuning;
	FGenesisFertilizationTuning Tuning;
	Tuning.PenetrationFailureRate = 0.0f;

	// 1. Eine Zelle nach der anderen: Zeit in der Zona und im Spalt
	double ZonaMinutes = 0.0;
	double SpaltMinutes = 0.0;
	double ShortestMinutes = TNumericLimits<double>::Max();
	int32 Fertilized = 0;
	bool bOblique = true;
	constexpr int32 Trials = 24;
	for (int32 Trial = 0; Trial < Trials; ++Trial)
	{
		FGenesisOocyteState Oocyte = MakeOocyte();
		TArray<FGenesisSpermCell> Cells;
		Cells.Add(MakeCellAtZona(700 + Trial, Oocyte, Channel, SwimTuning, FVector::ForwardVector));
		FGenesisFertilizationResult Result;
		float Time = 0.0f;
		float EnteredZona = -1.0f;
		float EnteredSpalt = -1.0f;
		while (Time < 3.0f * 3600.0f)
		{
			Cells[0].SteerDirection = GenesisFertilizationLogic::GetPhase(Cells[0]) == EGenesisSpermPhase::Swimming
				? (Oocyte.Position - Cells[0].Position).GetSafeNormal() : FVector::ZeroVector;
			const float Dt = GenesisFertilizationLogic::IsAttached(Cells[0]) ? TimeLapseStepSeconds : StepSeconds;
			const bool bFused = GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, Dt, Result);
			Time += Dt;
			const EGenesisSpermPhase Phase = GenesisFertilizationLogic::GetPhase(Cells[0]);
			if (EnteredZona < 0.0f && Phase == EGenesisSpermPhase::Penetrating)
			{
				EnteredZona = Time;
			}
			if (Phase == EGenesisSpermPhase::Penetrating && Cells[0].PenetrationDepthUm > 3.0f)
			{
				// Schräg, nicht senkrecht: Die Zelle zeigt deutlich an der Einwärtsrichtung vorbei
				const FVector Inward = (Oocyte.Position - Cells[0].Position).GetSafeNormal();
				bOblique &= FVector::DotProduct(Cells[0].Heading, Inward) < FMath::Cos(FMath::DegreesToRadians(25.0f));
			}
			if (EnteredSpalt < 0.0f && Phase == EGenesisSpermPhase::Perivitelline)
			{
				EnteredSpalt = Time;
			}
			if (bFused)
			{
				++Fertilized;
				ZonaMinutes += (EnteredSpalt - EnteredZona) / 60.0 / Trials;
				SpaltMinutes += (Time - EnteredSpalt) / 60.0 / Trials;
				ShortestMinutes = FMath::Min(ShortestMinutes, (Time - EnteredZona) / 60.0);
				break;
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d von %d befruchtet | Zona im Mittel %.1f min | Spalt im Mittel %.1f min | kürzester Weg Zona → Verschmelzung %.1f min"),
		Fertilized, Trials, ZonaMinutes, SpaltMinutes, ShortestMinutes));
	TestEqual(TEXT("Jede bereite Zelle kommt durch"), Fertilized, Trials);
	TestTrue(TEXT("Durch die Zona rund 13 Minuten (9–17)"), ZonaMinutes > 9.0 && ZonaMinutes < 17.0);
	TestTrue(TEXT("Im Spalt 16 ± 6 Minuten (12–20 im Mittel)"), SpaltMinutes > 12.0 && SpaltMinutes < 20.0);
	TestTrue(TEXT("Keine Befruchtung in Sekunden"), ShortestMinutes > 10.0);
	TestTrue(TEXT("Die Zelle dringt schräg ein"), bOblique);

	// 2. Der Schutz wirkt über Minuten, nicht in Sekunden
	{
		FGenesisOocyteState Oocyte = MakeOocyte();
		Oocyte.FertilizedByCell = 999;
		TArray<FGenesisSpermCell> None;
		FGenesisFertilizationResult Result;
		float Time = 0.0f;
		float HardenedAt = -1.0f;
		while (Time < 3600.0f)
		{
			GenesisFertilizationLogic::Step(None, Oocyte, Channel, SwimTuning, Tuning, 1.0f, Result);
			Time += 1.0f;
			if (HardenedAt < 0.0f && Oocyte.bZonaHardened)
			{
				HardenedAt = Time;
			}
		}
		AddInfo(FString::Printf(TEXT("Zona-Block nach %.1f min, Cortikalreaktion vollständig nach %.0f min"), HardenedAt / 60.0f, Tuning.CorticalReactionSeconds / 60.0f));
		TestTrue(TEXT("Der Zona-Block kommt nach Minuten (2–15 min)"), HardenedAt > 120.0f && HardenedAt < 900.0f);
	}

	// 3. Zwei kommen zugleich in den Spalt: Eine verschmilzt, die andere bleibt liegen
	{
		FGenesisOocyteState Oocyte = MakeOocyte();
		TArray<FGenesisSpermCell> Cells;
		Cells.Add(MakeCellAtZona(810, Oocyte, Channel, SwimTuning, FVector::ForwardVector));
		Cells.Add(MakeCellAtZona(811, Oocyte, Channel, SwimTuning, -FVector::ForwardVector));
		for (FGenesisSpermCell& Cell : Cells)
		{
			Cell.Vigor = 1.0f;
		}
		FGenesisFertilizationResult Result;
		for (float Time = 0.0f; Time < 3600.0f;)
		{
			for (FGenesisSpermCell& Cell : Cells)
			{
				Cell.SteerDirection = GenesisFertilizationLogic::GetPhase(Cell) == EGenesisSpermPhase::Swimming
					? (Oocyte.Position - Cell.Position).GetSafeNormal() : FVector::ZeroVector;
			}
			const float Dt = GenesisFertilizationLogic::IsAttached(Cells[0]) && GenesisFertilizationLogic::IsAttached(Cells[1]) ? TimeLapseStepSeconds : StepSeconds;
			GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, Dt, Result);
			Time += Dt;
		}
		int32 Fused = 0;
		int32 Waiting = 0;
		for (const FGenesisSpermCell& Cell : Cells)
		{
			Fused += GenesisFertilizationLogic::GetPhase(Cell) == EGenesisSpermPhase::Fused ? 1 : 0;
			Waiting += GenesisFertilizationLogic::GetPhase(Cell) == EGenesisSpermPhase::Perivitelline ? 1 : 0;
		}
		AddInfo(FString::Printf(TEXT("Zwei im Spalt: verschmolzen %d, liegen geblieben %d (Eizelle: %d im Spalt)"), Fused, Waiting, Oocyte.PerivitellineCells));
		TestEqual(TEXT("Eine verschmilzt"), Fused, 1);
		TestEqual(TEXT("Die andere bleibt im Spalt liegen"), Waiting, 1);
	}
	return true;
}

/**
 * Der Cumulus in echter Größe (GENESIS-047 Teil 2): gut 13.000 Zellen zwischen Corona und 550 µm, innen dicht, außen
 * locker; eine Zelle, die hindurchschwimmt, steckt nie in einer Cumuluszelle und braucht dafür Zeit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisFertilizationCumulusTest, "Genesis.Conception.Fertilization.CumulusField", GenesisFertilizationTests::Flags)
bool FGenesisFertilizationCumulusTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFertilizationTests;
	const FGenesisOviductChannel Channel = MakeChannel();
	const FGenesisCumulusTuning CumulusTuning;
	const FGenesisOocyteState Egg = MakeOocyte();
	const FGenesisCumulusField& Field = *Egg.Cumulus;

	// 1. Aufbau: Zahl, Grenzen, Abstand, Dichte innen gegen außen, Determinismus
	int32 Inner = 0;
	int32 Outer = 0;
	bool bInside = true;
	float Nearest = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Field.Cells.Num(); ++Index)
	{
		const float Radius = static_cast<float>(Field.Cells[Index].Center.Size());
		bInside &= Radius >= Egg.CoronaRadiusUm && Radius <= Egg.CumulusRadiusUm;
		Inner += Radius < 250.0f ? 1 : 0;
		Outer += Radius > 450.0f ? 1 : 0;
		if (Index < 400)
		{
			for (int32 Other = Index + 1; Other < Field.Cells.Num(); ++Other)
			{
				Nearest = FMath::Min(Nearest, static_cast<float>(FVector::Dist(Field.Cells[Index].Center, Field.Cells[Other].Center)));
			}
		}
	}
	// Zellen je Volumen: Schale 118–250 µm gegen 450–550 µm
	const double InnerDensity = Inner / (4.0 / 3.0 * UE_DOUBLE_PI * (FMath::Pow(250.0, 3.0) - FMath::Pow(118.0, 3.0)));
	const double OuterDensity = Outer / (4.0 / 3.0 * UE_DOUBLE_PI * (FMath::Pow(550.0, 3.0) - FMath::Pow(450.0, 3.0)));
	const TSharedPtr<const FGenesisCumulusField> Again = GenesisFertilizationLogic::BuildCumulus(Egg, CumulusTuning);
	AddInfo(FString::Printf(TEXT("Cumuluszellen %d (mit Corona gut %d) | innen %.0f, außen %.0f je Kubikmillimeter | engster Abstand %.1f µm"),
		Field.Cells.Num(), Field.Cells.Num() + 2600, InnerDensity * 1.0e9, OuterDensity * 1.0e9, Nearest));
	TestTrue(TEXT("Zahl wie beim Menschen (mit Corona 13.600–20.000)"), Field.Cells.Num() + 2600 >= 13600 && Field.Cells.Num() + 2600 <= 20000);
	TestTrue(TEXT("Alle zwischen Corona und Rand"), bInside);
	TestTrue(TEXT("Innen dichter als außen"), InnerDensity > 3.0 * OuterDensity);
	TestTrue(TEXT("Mindestabstand eingehalten"), Nearest >= CumulusTuning.MinSpacingUm - 0.01f);
	TestTrue(TEXT("Deterministisch"), Again->Cells.Num() == Field.Cells.Num() && Again->Cells.Last().Center.Equals(Field.Cells.Last().Center));

	// 2. Hindurch: kapazitierte Zellen von außen auf die Eizelle zu – keine steckt je in einer Zelle
	FGenesisSpermSwimTuning SwimTuning;
	SwimTuning.CapacitatedFraction = 1.0f;
	FGenesisFertilizationTuning Tuning;
	FGenesisOocyteState Oocyte = MakeOocyte();
	TArray<FGenesisSpermCell> Cells = MakeCellsAround(40, Oocyte, Channel, SwimTuning, 7000, 40.0f, nullptr, true);
	TArray<float> Reached;
	Reached.Init(-1.0f, Cells.Num());
	int32 Penetrations = 0;
	FGenesisFertilizationResult Result;
	for (float Time = 0.0f; Time < 60.0f; Time += StepSeconds)
	{
		for (FGenesisSpermCell& Cell : Cells)
		{
			Cell.SteerDirection = (Oocyte.Position - Cell.Position).GetSafeNormal();
		}
		GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, StepSeconds, Result);
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			const FGenesisSpermCell& Cell = Cells[Index];
			if (GenesisFertilizationLogic::GetPhase(Cell) != EGenesisSpermPhase::Swimming)
			{
				Reached[Index] = Reached[Index] < 0.0f ? Time : Reached[Index];
				continue;
			}
			// Steckte die Spitze nach dem Schritt noch in einer Zelle, müsste die Auflösung sie merklich verschieben
			FGenesisSpermCell Probe = Cell;
			GenesisFertilizationLogic::ResolveCumulusContact(Probe, Oocyte);
			Penetrations += FVector::Dist(Probe.Position, Cell.Position) > 0.3 ? 1 : 0;
		}
	}
	int32 Arrived = 0;
	float Fastest = TNumericLimits<float>::Max();
	for (const float Arrival : Reached)
	{
		Arrived += Arrival >= 0.0f ? 1 : 0;
		Fastest = Arrival >= 0.0f ? FMath::Min(Fastest, Arrival) : Fastest;
	}
	AddInfo(FString::Printf(TEXT("Durch den Cumulus: %d von %d an der Zona binnen 60 s, schnellste nach %.0f s | Kopf in einer Zelle: %d Schritte"),
		Arrived, Cells.Num(), Arrived > 0 ? Fastest : -1.0f, Penetrations));
	TestEqual(TEXT("Keine Spermienspitze steckt in einer Cumuluszelle"), Penetrations, 0);
	TestTrue(TEXT("Der Weg durch die Gallerte braucht Zeit (keine in unter 20 s)"), Arrived == 0 || Fastest > 20.0f);
	return true;
}

#endif
