// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisFertilizationLogic.h"
#include "GenesisSpermRace.h"
#include "GenesisSpermSwimLogic.h"
#include "GenesisRandom.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSpermRaceTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float StepSeconds = 1.0f / 120.0f;
	/**
	 * So viele Zellen wie im Spiel. Mit 2000 gewann der Testfahrer 4 von 5 – im Spiel mit 6000 verlor
	 * der Autopilot gegen 73 Konkurrentinnen an der Zona. Ein Test, der ein leichteres Rennen misst als
	 * das, das man spielt, beweist nichts.
	 */
	constexpr int32 FieldSize = 6000;

	enum class EDriver { Passive, Skilled };

	struct FRace
	{
		EGenesisRaceOutcome Outcome = EGenesisRaceOutcome::Running;
		float Seconds = 0.0f;
		int32 BestPlace = TNumericLimits<int32>::Max();
		/** Zeitpunkte der eigenen Zelle: Cumulus erreicht, hyperaktiviert, gebunden, bohrt (s, −1 = nie). */
		float AtCumulus = -1.0f;
		float Hyper = -1.0f;
		float Bound = -1.0f;
		float Drilling = -1.0f;
		float Depth = 0.0f;
		float ClosestZona = TNumericLimits<float>::Max();
		float ContactSeconds = 0.0f;
		float HyperContactSeconds = 0.0f;

		FString Timeline() const
		{
			return FString::Printf(TEXT("Cumulus %.1f s, hyper %.1f s, gebunden %.1f s, bohrt %.1f s, Tiefe %.1f µm | nächste Zona %.1f µm, Kontakt %.1f s (hyper %.1f s)"),
				AtCumulus, Hyper, Bound, Drilling, Depth, ClosestZona, ContactSeconds, HyperContactSeconds);
		}
	};

	FRace Run(uint64 Seed, EDriver Driver)
	{
		FGenesisOviductChannel Channel;
		Channel.LumenRadiusUm = 450.0f;
		Channel.LengthUm = 3000.0f;
		Channel.WallFlowSpeedUm = 25.0f;
		const FGenesisSpermSwimTuning SwimTuning;
		const FGenesisFertilizationTuning Tuning;
		const FGenesisRaceTuning Race;

		FGenesisOocyteState Oocyte;
		Oocyte.Position = FVector(1500.0, 0.0, 0.0);

		// Der Pulk wie im Schwarm-Actor: Vitalität normalverteilt, Band vor der Eizelle
		FGenesisRandomStream Random(Seed);
		TArray<FGenesisSpermCell> Cells;
		Cells.Reserve(FieldSize);
		for (int32 Index = 0; Index < FieldSize; ++Index)
		{
			const float Vitality = FMath::Clamp(Random.Gaussian(0.75f, 0.18f), 0.0f, 1.0f);
			FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(Seed * 7919 + Index, Vitality, Channel, SwimTuning);
			Cell.Position.X = Oocyte.Position.X - Race.FieldDistanceUm + Random.Gaussian(0.0f, Race.FieldSpreadUm);
			Cells.Add(MoveTemp(Cell));
		}

		// Die eigene Zelle: an der Spitze des Pulks, gute Vitalität, progressiv, Richtung Eizelle
		const int32 Player = 0;
		{
			FGenesisSpermCell Mine = GenesisSpermSwimLogic::CreateCell(Seed * 104729, Race.Vitality, Channel, SwimTuning);
			GenesisSpermSwimLogic::ApplyMotility(Mine, EGenesisSpermMotility::Progressive, SwimTuning);
			Mine.Position = FVector(Oocyte.Position.X - Race.StartDistanceUm, 60.0, -40.0);
			Mine.Heading = FVector::ForwardVector;
			Cells[Player] = MoveTemp(Mine);
		}

		FRace Result;
		FGenesisFertilizationResult Fusion;
		const int32 Steps = FMath::RoundToInt(180.0f / StepSeconds);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			FGenesisSpermCell& Mine = Cells[Player];
			if (Driver == EDriver::Skilled)
			{
				// Ein guter Spieler hält auf die Eizelle zu und gibt beim Bohren alles
				Mine.SteerDirection = GenesisFertilizationLogic::GetPhase(Mine) == EGenesisSpermPhase::Swimming
					? (Oocyte.Position - Mine.Position).GetSafeNormal()
					: FVector::ZeroVector;
				Mine.Vigor = 1.0f;
			}
			if (GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, StepSeconds, Fusion))
			{
				Result.Outcome = GenesisSpermRace::OutcomeAfterFusion(Player, Fusion.CellIndex);
				Result.Seconds = Step * StepSeconds;
				break;
			}
			const float Now = Step * StepSeconds;
			const FGenesisSpermCell& Tracked = Cells[Player];
			const EGenesisSpermPhase Phase = GenesisFertilizationLogic::GetPhase(Tracked);
			if (Result.AtCumulus < 0.0f && FVector::Dist(Tracked.Position, Oocyte.Position) < Oocyte.CumulusRadiusUm) { Result.AtCumulus = Now; }
			if (Result.Hyper < 0.0f && Tracked.Motility == EGenesisSpermMotility::Hyperactivated) { Result.Hyper = Now; }
			if (Result.Bound < 0.0f && Phase == EGenesisSpermPhase::Bound) { Result.Bound = Now; }
			if (Result.Drilling < 0.0f && Phase == EGenesisSpermPhase::Penetrating) { Result.Drilling = Now; }
			Result.Depth = Tracked.PenetrationDepthUm;
			if (Phase == EGenesisSpermPhase::Swimming && Result.AtCumulus >= 0.0f)
			{
				const float Gap = GenesisFertilizationLogic::DistanceToZona(Tracked, Oocyte);
				Result.ClosestZona = FMath::Min(Result.ClosestZona, Gap);
				Result.ContactSeconds += Gap < Tuning.BindingDistanceUm ? StepSeconds : 0.0f;
				Result.HyperContactSeconds += Gap < Tuning.BindingDistanceUm && Tracked.Motility == EGenesisSpermMotility::Hyperactivated ? StepSeconds : 0.0f;
			}
			if (Step % 120 == 0)
			{
				Result.BestPlace = FMath::Min(Result.BestPlace, GenesisSpermRace::CountCellsAhead(Cells, Player, Oocyte) + 1);
			}
		}
		return Result;
	}
}

/**
 * Das Rennen muss gewinnbar sein – und es muss auf den Spieler ankommen.
 * Wer gut lenkt und beim Bohren Kraft gibt, soll meistens gewinnen; dieselbe Zelle ohne Führung meistens nicht.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermRaceFairnessTest, "Genesis.Conception.Race.SkillDecides", GenesisSpermRaceTests::Flags)
bool FGenesisSpermRaceFairnessTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermRaceTests;
	int32 SkilledWins = 0;
	int32 PassiveWins = 0;
	const uint64 Seeds[] = { 11, 23, 37, 51, 73 };
	for (const uint64 Seed : Seeds)
	{
		const FRace Skilled = Run(Seed, EDriver::Skilled);
		const FRace Passive = Run(Seed, EDriver::Passive);
		SkilledWins += Skilled.Outcome == EGenesisRaceOutcome::Won ? 1 : 0;
		PassiveWins += Passive.Outcome == EGenesisRaceOutcome::Won ? 1 : 0;
		AddInfo(FString::Printf(TEXT("Seed %llu: gelenkt %s nach %.1f s (bester Platz %d) | ohne Führung %s nach %.1f s (bester Platz %d)"),
			Seed,
			*UEnum::GetValueAsString(Skilled.Outcome), Skilled.Seconds, Skilled.BestPlace,
			*UEnum::GetValueAsString(Passive.Outcome), Passive.Seconds, Passive.BestPlace));
		AddInfo(TEXT("    gelenkt: ") + Skilled.Timeline());
	}
	AddInfo(FString::Printf(TEXT("Siege: gelenkt %d von 5, ohne Führung %d von 5"), SkilledWins, PassiveWins));
	TestTrue(TEXT("Gut gelenkt gewinnt man meistens"), SkilledWins >= 3);
	TestTrue(TEXT("Ohne Führung gewinnt man selten"), PassiveWins <= 1);
	return true;
}

/** Anstrengung: Tastendrücke heben sie, sie fällt von selbst ab; Lenken bleibt innerhalb dessen, was eine Zelle kann. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermRaceInputTest, "Genesis.Conception.Race.Input", GenesisSpermRaceTests::Flags)
bool FGenesisSpermRaceInputTest::RunTest(const FString& Parameters)
{
	const FGenesisRaceTuning Race;

	// Vier Drücke je Sekunde halten die Kraft oben, einer je Sekunde nicht
	float Fast = 0.0f;
	float Slow = 0.0f;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		Fast = GenesisSpermRace::UpdateVigor(Fast, Frame % 15 == 0 ? 1 : 0, 1.0f / 60.0f, Race);
		Slow = GenesisSpermRace::UpdateVigor(Slow, Frame % 60 == 0 ? 1 : 0, 1.0f / 60.0f, Race);
	}
	AddInfo(FString::Printf(TEXT("Kraft nach 10 s: 4 Drücke/s %.2f, 1 Druck/s %.2f"), Fast, Slow));
	TestTrue(TEXT("Schnelles Schlagen hält die Kraft hoch"), Fast > 0.8f);
	TestTrue(TEXT("Langsames Schlagen reicht nicht"), Slow < 0.3f);
	TestTrue(TEXT("Nie über 1"), Fast <= 1.0f);

	// Ohne Eingabe keine Lenkung – die Physik bleibt allein
	TestTrue(TEXT("Keine Eingabe, keine Richtung"), GenesisSpermRace::SteerFromInput(FVector::ForwardVector, FVector2D::ZeroVector, Race).IsNearlyZero());

	// Voll nach rechts: um die Hochachse gedreht, im Rahmen des Lenkwinkels
	const FVector Right = GenesisSpermRace::SteerFromInput(FVector::ForwardVector, FVector2D(1.0, 0.0), Race);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Right, FVector::ForwardVector)));
	TestTrue(TEXT("Lenkwinkel wie eingestellt"), FMath::IsNearlyEqual(Angle, Race.SteerYawDegrees, 0.5f));
	TestTrue(TEXT("Bleibt waagerecht"), FMath::Abs(Right.Z) < 0.01);

	// Die Drehrate begrenzt die Kurve: Eine Zelle wendet nicht auf der Stelle
	FGenesisOviductChannel Channel;
	const FGenesisSpermSwimTuning SwimTuning;
	FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(5, 0.9f, Channel, SwimTuning);
	GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Progressive, SwimTuning);
	Cell.Position = FVector(1500.0, 0.0, 0.0);
	Cell.Heading = FVector::ForwardVector;
	Cell.SteerDirection = -FVector::ForwardVector;
	GenesisSpermSwimLogic::Advance(Cell, Channel, SwimTuning, 0.5f);
	const float Turned = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Cell.Heading, FVector::ForwardVector), -1.0, 1.0)));
	AddInfo(FString::Printf(TEXT("Wende nach 0,5 s: %.0f° (Drehrate %.1f rad/s ≈ %.0f°)"), Turned, SwimTuning.SteerTurnRate, FMath::RadiansToDegrees(SwimTuning.SteerTurnRate * 0.5f)));
	TestTrue(TEXT("Keine Wende auf der Stelle"), Turned < 70.0f);
	TestTrue(TEXT("Aber sie dreht"), Turned > 25.0f);
	return true;
}

/** Eine hyperaktivierte Zelle, die an der Zona anliegt, bindet – im Mittel nach gut einer Sekunde (0,85 je Sekunde). */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermRaceBindingTest, "Genesis.Conception.Race.ContactBinds", GenesisSpermRaceTests::Flags)
bool FGenesisSpermRaceBindingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSpermRaceTests;
	FGenesisOviductChannel Channel;
	const FGenesisSpermSwimTuning SwimTuning;
	const FGenesisFertilizationTuning Tuning;
	int32 Bound = 0;
	float Total = 0.0f;
	for (int32 Trial = 0; Trial < 40; ++Trial)
	{
		FGenesisOocyteState Oocyte;
		Oocyte.Position = FVector(1500.0, 0.0, 0.0);
		TArray<FGenesisSpermCell> Cells;
		FGenesisSpermCell Cell = GenesisSpermSwimLogic::CreateCell(900 + Trial, 0.9f, Channel, SwimTuning);
		GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Hyperactivated, SwimTuning);
		Cell.Position = Oocyte.Position - FVector(Oocyte.ZonaOuterRadiusUm + 1.0, 0.0, 0.0);
		Cell.Heading = FVector::ForwardVector;
		Cells.Add(Cell);
		FGenesisFertilizationResult Result;
		for (int32 Step = 0; Step < 600; ++Step)
		{
			Cells[0].SteerDirection = (Oocyte.Position - Cells[0].Position).GetSafeNormal();
			GenesisFertilizationLogic::Step(Cells, Oocyte, Channel, SwimTuning, Tuning, StepSeconds, Result);
			if (GenesisFertilizationLogic::GetPhase(Cells[0]) != EGenesisSpermPhase::Swimming)
			{
				++Bound;
				Total += Step * StepSeconds;
				break;
			}
		}
	}
	AddInfo(FString::Printf(TEXT("Gebunden in 5 s: %d von 40, im Mittel nach %.2f s"), Bound, Bound > 0 ? Total / Bound : 0.0f));
	TestTrue(TEXT("Fast alle binden"), Bound >= 36);
	return true;
}

/** Nur eine Zelle verschmilzt – wer sie nicht ist, hat verloren. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSpermRaceOutcomeTest, "Genesis.Conception.Race.Outcome", GenesisSpermRaceTests::Flags)
bool FGenesisSpermRaceOutcomeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Ohne Spieler kein Rennen"), static_cast<int32>(GenesisSpermRace::OutcomeAfterFusion(INDEX_NONE, 4)), static_cast<int32>(EGenesisRaceOutcome::None));
	TestEqual(TEXT("Noch keine Verschmelzung"), static_cast<int32>(GenesisSpermRace::OutcomeAfterFusion(3, INDEX_NONE)), static_cast<int32>(EGenesisRaceOutcome::Running));
	TestEqual(TEXT("Eigene Zelle verschmolzen"), static_cast<int32>(GenesisSpermRace::OutcomeAfterFusion(3, 3)), static_cast<int32>(EGenesisRaceOutcome::Won));
	TestEqual(TEXT("Eine andere war schneller"), static_cast<int32>(GenesisSpermRace::OutcomeAfterFusion(3, 7)), static_cast<int32>(EGenesisRaceOutcome::Lost));
	return true;
}

#endif
