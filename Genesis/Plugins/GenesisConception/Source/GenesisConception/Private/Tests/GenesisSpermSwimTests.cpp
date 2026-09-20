// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisSpermSwimLogic.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSpermSwimTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FKinematics
	{
		double StraightLineVelocity = 0.0; // VSL
		double CurvilinearVelocity = 0.0;  // VCL
		double Linearity = 0.0;            // LIN = VSL / VCL
		double BeatHz = 0.0;
	};

	/** Misst CASA-Kenngrößen über eine Messdauer (Kopfbahn mit 240 Hz abgetastet, wie Hochgeschwindigkeits-CASA). */
	FKinematics Measure(FGenesisSpermCell Cell, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning, float Seconds)
	{
		const FVector StartHead = GenesisSpermSwimLogic::ComputeHeadPosition(Cell);
		FVector PreviousHead = StartHead;
		double PathLength = 0.0;
		const int32 Steps = FMath::RoundToInt(Seconds / Tuning.FixedStepSeconds);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			GenesisSpermSwimLogic::Step(Cell, Channel, Tuning, Tuning.FixedStepSeconds);
			const FVector Head = GenesisSpermSwimLogic::ComputeHeadPosition(Cell);
			PathLength += FVector::Dist(Head, PreviousHead);
			PreviousHead = Head;
		}
		FKinematics Result;
		Result.StraightLineVelocity = FVector::Dist(PreviousHead, StartHead) / Seconds;
		Result.CurvilinearVelocity = PathLength / Seconds;
		Result.Linearity = Result.CurvilinearVelocity > 0.0 ? Result.StraightLineVelocity / Result.CurvilinearVelocity : 0.0;
		Result.BeatHz = Cell.BeatFrequencyHz;
		return Result;
	}

	/** Offener Raum ohne Wand und Strömung. */
	FGenesisOviductChannel OpenWater()
	{
		FGenesisOviductChannel Channel;
		Channel.LumenRadiusUm = 100000.0f;
		Channel.LengthUm = 1000000.0f;
		Channel.WallFlowSpeedUm = 0.0f;
		return Channel;
	}

	FGenesisSpermSwimTuning WithoutSwitching()
	{
		FGenesisSpermSwimTuning Tuning;
		Tuning.HyperactivationRate = 0.0f;
		Tuning.DeactivationRate = 0.0f;
		return Tuning;
	}

	double RadialDistance(const FGenesisSpermCell& Cell)
	{
		return FVector2D(Cell.Position.Y, Cell.Position.Z).Size();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermKinematicsTest, "Genesis.Conception.Swim.CasaKinematics", GenesisSpermSwimTests::Flags)
bool FGenesisSpermKinematicsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermSwimTests;
	const FGenesisOviductChannel Channel = OpenWater();
	const FGenesisSpermSwimTuning Tuning = WithoutSwitching();

	FKinematics Progressive;
	FKinematics Hyper;
	const int32 Count = 60;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FGenesisSpermCell ProgressiveCell = GenesisSpermSwimLogic::CreateCell(1000 + Index, 1.0f, Channel, Tuning);
		GenesisSpermSwimLogic::ApplyMotility(ProgressiveCell, EGenesisSpermMotility::Progressive, Tuning);
		const FKinematics P = Measure(ProgressiveCell, Channel, Tuning, 2.0f);
		Progressive.StraightLineVelocity += P.StraightLineVelocity / Count;
		Progressive.CurvilinearVelocity += P.CurvilinearVelocity / Count;
		Progressive.Linearity += P.Linearity / Count;
		Progressive.BeatHz += P.BeatHz / Count;

		FGenesisSpermCell HyperCell = ProgressiveCell;
		GenesisSpermSwimLogic::ApplyMotility(HyperCell, EGenesisSpermMotility::Hyperactivated, Tuning);
		const FKinematics H = Measure(HyperCell, Channel, Tuning, 2.0f);
		Hyper.StraightLineVelocity += H.StraightLineVelocity / Count;
		Hyper.CurvilinearVelocity += H.CurvilinearVelocity / Count;
		Hyper.Linearity += H.Linearity / Count;
		Hyper.BeatHz += H.BeatHz / Count;
	}

	AddInfo(FString::Printf(TEXT("Progressiv: VSL %.1f µm/s, VCL %.1f µm/s, LIN %.2f, %.1f Hz"), Progressive.StraightLineVelocity, Progressive.CurvilinearVelocity, Progressive.Linearity, Progressive.BeatHz));
	AddInfo(FString::Printf(TEXT("Hyperaktiviert: VSL %.1f µm/s, VCL %.1f µm/s, LIN %.2f, %.1f Hz"), Hyper.StraightLineVelocity, Hyper.CurvilinearVelocity, Hyper.Linearity, Hyper.BeatHz));

	// WHO: schnell progressiv VSL ≥ 25 µm/s; CASA-Hyperaktivierung: VCL ≥ 150 µm/s, LIN < 0,5
	TestTrue(TEXT("Progressiv schnell (VSL 25–60 µm/s)"), Progressive.StraightLineVelocity >= 25.0 && Progressive.StraightLineVelocity <= 60.0);
	TestTrue(TEXT("Kurvengeschwindigkeit über der Geradeaus-Geschwindigkeit"), Progressive.CurvilinearVelocity > Progressive.StraightLineVelocity);
	TestTrue(TEXT("Progressive VCL plausibel (60–250 µm/s)"), Progressive.CurvilinearVelocity >= 60.0 && Progressive.CurvilinearVelocity <= 250.0);
	TestTrue(TEXT("Schlagfrequenz progressiv 12–18 Hz"), Progressive.BeatHz >= 12.0 && Progressive.BeatHz <= 18.0);
	TestTrue(TEXT("Hyperaktiviert: VCL ≥ 150 µm/s"), Hyper.CurvilinearVelocity >= 150.0);
	TestTrue(TEXT("Hyperaktiviert: LIN < 0,5"), Hyper.Linearity < 0.5);
	TestTrue(TEXT("Hyperaktiviert: weniger Vortrieb"), Hyper.StraightLineVelocity < Progressive.StraightLineVelocity);
	TestTrue(TEXT("Hyperaktiviert: langsamerer Schlag"), Hyper.BeatHz < Progressive.BeatHz);

	// Determinismus
	FGenesisSpermCell A = GenesisSpermSwimLogic::CreateCell(77, 0.8f, Channel, Tuning);
	FGenesisSpermCell B = GenesisSpermSwimLogic::CreateCell(77, 0.8f, Channel, Tuning);
	GenesisSpermSwimLogic::Advance(A, Channel, Tuning, 3.0f);
	GenesisSpermSwimLogic::Advance(B, Channel, Tuning, 3.0f);
	TestTrue(TEXT("Deterministisch"), A.Position.Equals(B.Position, 1e-6) && A.Heading.Equals(B.Heading, 1e-9));

	FGenesisSpermCell Weak = GenesisSpermSwimLogic::CreateCell(78, 0.1f, Channel, Tuning);
	TestTrue(TEXT("Geringe Vitalität: träge"), Weak.Motility == EGenesisSpermMotility::Sluggish && Weak.Speed <= Tuning.SluggishSpeedUm.Max);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermWallTest, "Genesis.Conception.Swim.WallAccumulationAndContainment", GenesisSpermSwimTests::Flags)
bool FGenesisSpermWallTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermSwimTests;
	FGenesisOviductChannel Channel;
	Channel.LumenRadiusUm = 200.0f;
	Channel.LengthUm = 2000.0f;
	Channel.WallFlowSpeedUm = 0.0f;
	const FGenesisSpermSwimTuning Tuning = WithoutSwitching();

	auto WallFraction = [&](EGenesisSpermMotility Motility, bool& bOutContained)
	{
		const int32 Count = 200;
		int32 NearWall = 0;
		bOutContained = true;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(5000 + Index, 1.0f, Channel, Tuning);
			GenesisSpermSwimLogic::ApplyMotility(Cell, Motility, Tuning);
			const int32 Steps = FMath::RoundToInt(40.0f / Tuning.FixedStepSeconds);
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				GenesisSpermSwimLogic::Step(Cell, Channel, Tuning, Tuning.FixedStepSeconds);
				bOutContained &= RadialDistance(Cell) <= Channel.LumenRadiusUm - Tuning.WallMarginUm + 1e-3;
			}
			NearWall += Channel.LumenRadiusUm - RadialDistance(Cell) < Tuning.WallAttractionDistanceUm ? 1 : 0;
		}
		return static_cast<double>(NearWall) / Count;
	};

	bool bProgressiveContained = true;
	bool bHyperContained = true;
	const double ProgressiveAtWall = WallFraction(EGenesisSpermMotility::Progressive, bProgressiveContained);
	const double HyperAtWall = WallFraction(EGenesisSpermMotility::Hyperactivated, bHyperContained);
	const double UniformFraction = 1.0 - FMath::Square((Channel.LumenRadiusUm - Tuning.WallAttractionDistanceUm) / Channel.LumenRadiusUm);
	AddInfo(FString::Printf(TEXT("An der Wand nach 40 s: progressiv %.0f %%, hyperaktiviert %.0f %% (Gleichverteilung wäre %.0f %%)"),
		100.0 * ProgressiveAtWall, 100.0 * HyperAtWall, 100.0 * UniformFraction));

	TestTrue(TEXT("Progressive Zellen sammeln sich an der Wand"), ProgressiveAtWall > 2.0 * UniformFraction);
	TestTrue(TEXT("Hyperaktivierte lösen sich leichter"), HyperAtWall < ProgressiveAtWall);
	TestTrue(TEXT("Nie durch die Wand"), bProgressiveContained && bHyperContained);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermRheotaxisTest, "Genesis.Conception.Swim.Rheotaxis", GenesisSpermSwimTests::Flags)
bool FGenesisSpermRheotaxisTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermSwimTests;
	FGenesisOviductChannel Channel;
	Channel.LumenRadiusUm = 200.0f;
	Channel.LengthUm = 100000.0f;
	Channel.WallFlowSpeedUm = 25.0f;

	auto Run = [&](float TurnRate, double& OutUpstreamHeading, double& OutNetDisplacement)
	{
		FGenesisSpermSwimTuning Tuning = WithoutSwitching();
		Tuning.RheotaxisTurnRate = TurnRate;
		const int32 Count = 150;
		OutUpstreamHeading = 0.0;
		OutNetDisplacement = 0.0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(9000 + Index, 1.0f, Channel, Tuning);
			GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Progressive, Tuning);
			// Start an der Wand, Kanal lang genug, dass nichts umläuft
			Cell.Position = FVector(50000.0, Channel.LumenRadiusUm - 10.0, 0.0);
			const double StartX = Cell.Position.X;
			GenesisSpermSwimLogic::Advance(Cell, Channel, Tuning, 30.0f);
			OutUpstreamHeading += Cell.Heading.X / Count;
			OutNetDisplacement += (Cell.Position.X - StartX) / Count;
		}
	};

	double WithHeading = 0.0, WithDisplacement = 0.0, WithoutHeading = 0.0, WithoutDisplacement = 0.0;
	Run(0.8f, WithHeading, WithDisplacement);
	Run(0.0f, WithoutHeading, WithoutDisplacement);
	AddInfo(FString::Printf(TEXT("Rheotaxis: Ausrichtung stromaufwärts %.2f, Netto %+.0f µm in 30 s | ohne: %.2f, %+.0f µm"),
		WithHeading, WithDisplacement, WithoutHeading, WithoutDisplacement));

	TestTrue(TEXT("Mit Rheotaxis gegen den Strom ausgerichtet"), WithHeading > 0.5);
	TestTrue(TEXT("Mit Rheotaxis Richtung Eierstock unterwegs"), WithDisplacement > 150.0);
	TestTrue(TEXT("Ohne Rheotaxis keine Ausrichtung"), WithoutHeading < 0.3);
	TestTrue(TEXT("Ohne Rheotaxis treibt der Strom zurück"), WithoutDisplacement < WithDisplacement);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermVisualTest, "Genesis.Conception.Swim.VisualAndPerformance", GenesisSpermSwimTests::Flags)
bool FGenesisSpermVisualTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermSwimTests;
	FGenesisOviductChannel Channel;
	FGenesisSpermSwimTuning Tuning;
	Tuning.HyperactivationRate = 0.5f;
	Tuning.DeactivationRate = 0.5f;

	// Wechsel zwischen den Bewegungsarten findet statt, Gleichgewicht ~50 %
	int32 HyperCount = 0;
	const int32 Count = 200;
	TArray<FGenesisSpermCell> Cells;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Cells.Add(GenesisSpermSwimLogic::CreateCell(20000 + Index, 0.9f, Channel, Tuning));
	}
	for (FGenesisSpermCell& Cell : Cells)
	{
		GenesisSpermSwimLogic::Advance(Cell, Channel, Tuning, 20.0f);
		HyperCount += Cell.Motility == EGenesisSpermMotility::Hyperactivated ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("Wechsel im Gleichgewicht (%d von %d hyperaktiviert)"), HyperCount, Count), HyperCount > 60 && HyperCount < 140);

	// Darstellung: Kopf innerhalb der halben Auslenkung, Ausrichtung folgt der Schwimmrichtung, Materialdaten plausibel
	bool bHeadNear = true;
	bool bAligned = true;
	bool bDataValid = true;
	for (const FGenesisSpermCell& Cell : Cells)
	{
		const FTransform Visual = GenesisSpermSwimLogic::ComputeVisualTransform(Cell);
		bHeadNear &= FVector::Dist(Visual.GetLocation(), Cell.Position) <= 0.5 * Cell.HeadAmplitudeUm + 1e-3;
		bAligned &= FVector::DotProduct(Visual.GetUnitAxis(EAxis::X), Cell.Heading) > FMath::Cos(Cell.HeadAmplitudeUm / 12.0 + 0.25 * Cell.Asymmetry + 0.01);
		float Data[4];
		GenesisSpermSwimLogic::ComputeMaterialData(Cell, Data);
		bDataValid &= Data[0] >= 0.0f && Data[0] < 1.0f && Data[1] > 0.0f && Data[2] >= 0.0f && Data[2] <= 1.0f && Data[3] > 0.0f;
	}
	TestTrue(TEXT("Kopf pendelt höchstens um die halbe Auslenkung"), bHeadNear);
	TestTrue(TEXT("Zelle zeigt in Schwimmrichtung"), bAligned);
	TestTrue(TEXT("Materialdaten gültig"), bDataValid);

	// Performance: 1000 Zellen, 1 s Simulationszeit (240 Schritte)
	TArray<FGenesisSpermCell> Swarm;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		Swarm.Add(GenesisSpermSwimLogic::CreateCell(30000 + Index, 0.8f, Channel, Tuning));
	}
	const double Start = FPlatformTime::Seconds();
	for (int32 Step = 0; Step < 240; ++Step)
	{
		for (FGenesisSpermCell& Cell : Swarm)
		{
			GenesisSpermSwimLogic::Step(Cell, Channel, Tuning, Tuning.FixedStepSeconds);
		}
	}
	const double NanosecondsPerStep = (FPlatformTime::Seconds() - Start) * 1e9 / (1000.0 * 240.0);
	AddInfo(FString::Printf(TEXT("Performance: %.0f ns je Zellschritt → 1000 Zellen je Frame (Zeitlupe 1/4, 60 fps): %.3f ms"), NanosecondsPerStep, NanosecondsPerStep * 1000.0 / 1e6));
	TestTrue(TEXT("Unter 1000 ns je Zellschritt"), NanosecondsPerStep < 1000.0);
	return true;
}

/**
 * Der ganze Schwarm muss in eine Richtung ziehen – auch mitten im Lumen, wo keine Wand hilft.
 * Genau das fehlte: Ohne Strömung in der Kanalmitte hatten die Zellen dort keinen Hinweis,
 * wohin, und die Hälfte schwamm rückwärts.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermSwarmDirectionTest, "Genesis.Conception.Swim.SwarmHeadsUpstream", GenesisSpermSwimTests::Flags)
bool FGenesisSpermSwarmDirectionTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermSwimTests;

	FGenesisOviductChannel Channel;
	Channel.LumenRadiusUm = 450.0f;
	Channel.LengthUm = 200000.0f;
	Channel.WallFlowSpeedUm = 25.0f;

	FGenesisSpermSwimTuning Tuning = WithoutSwitching();

	const int32 Count = 200;
	double MeanHeading = 0.0;
	double MeanDisplacement = 0.0;
	int32 Backwards = 0;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(12000 + Index, 0.85f, Channel, Tuning);
		GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Progressive, Tuning);
		// Mitten im Lumen, weit weg von jeder Wand
		Cell.Position = FVector(100000.0, 0.0, 0.0);
		const double StartX = Cell.Position.X;
		GenesisSpermSwimLogic::Advance(Cell, Channel, Tuning, 30.0f);

		MeanHeading += Cell.Heading.X / Count;
		MeanDisplacement += (Cell.Position.X - StartX) / Count;
		if (Cell.Heading.X < 0.0)
		{
			++Backwards;
		}
	}

	AddInfo(FString::Printf(TEXT("Mitte des Lumens nach 30 s: Ausrichtung %.2f, Netto %+.0f µm, %d von %d rückwärts"),
		MeanHeading, MeanDisplacement, Backwards, Count));

	TestTrue(TEXT("Der Schwarm zieht flussaufwärts"), MeanHeading > 0.75);
	TestTrue(TEXT("Und kommt dabei voran"), MeanDisplacement > 300.0);
	TestTrue(TEXT("Kaum noch jemand schwimmt rückwärts"), Backwards < Count / 20);

	return true;
}

#endif
