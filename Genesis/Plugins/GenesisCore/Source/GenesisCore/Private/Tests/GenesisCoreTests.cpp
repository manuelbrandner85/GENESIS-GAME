// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisMath.h"
#include "GenesisPersistence.h"
#include "GenesisRandom.h"
#include "GenesisTypes.h"
#include "GenesisWorldClockSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisCoreTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisRandomDeterminismTest, "Genesis.Core.Random.Determinism", GenesisCoreTests::Flags)
bool FGenesisRandomDeterminismTest::RunTest(const FString& Parameters)
{
	FGenesisRandomStream A(123456789ull);
	FGenesisRandomStream B(123456789ull);
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		if (A.NextUInt64() != B.NextUInt64())
		{
			AddError(TEXT("Gleicher Seed muss identische Sequenz liefern."));
			return false;
		}
	}

	FGenesisRandomStream C(987654321ull);
	TestNotEqual(TEXT("Unterschiedliche Seeds liefern unterschiedliche Werte"), FGenesisRandomStream(1).NextUInt64(), C.NextUInt64());

	// Derive ist unabhängig vom bereits verbrauchten Zustand
	FGenesisRandomStream Fresh(42);
	FGenesisRandomStream Used(42);
	for (int32 Index = 0; Index < 50; ++Index)
	{
		Used.NextUInt64();
	}
	TestEqual(TEXT("Derive hängt nur vom Seed ab"), Fresh.Derive(7).NextUInt64(), Used.Derive(7).NextUInt64());
	TestNotEqual(TEXT("Unterschiedliche Salts liefern unterschiedliche Ströme"), Fresh.Derive(7).NextUInt64(), Fresh.Derive(8).NextUInt64());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisRandomRangesTest, "Genesis.Core.Random.Ranges", GenesisCoreTests::Flags)
bool FGenesisRandomRangesTest::RunTest(const FString& Parameters)
{
	FGenesisRandomStream Stream(2024);
	double Sum = 0.0;
	const int32 Samples = 20000;
	for (int32 Index = 0; Index < Samples; ++Index)
	{
		const double Value = Stream.NextDouble();
		if (Value < 0.0 || Value >= 1.0)
		{
			AddError(FString::Printf(TEXT("NextDouble außerhalb [0,1): %f"), Value));
			return false;
		}
		Sum += Value;

		const int32 Dice = Stream.RandRange(1, 6);
		if (Dice < 1 || Dice > 6)
		{
			AddError(FString::Printf(TEXT("RandRange außerhalb [1,6]: %d"), Dice));
			return false;
		}
	}
	TestTrue(TEXT("Mittelwert gleichverteilt ~0.5"), FMath::Abs(Sum / Samples - 0.5) < 0.02);

	double GaussSum = 0.0;
	for (int32 Index = 0; Index < Samples; ++Index)
	{
		GaussSum += Stream.Gaussian(10.0f, 2.0f);
	}
	TestTrue(TEXT("Gauß-Mittelwert ~10"), FMath::Abs(GaussSum / Samples - 10.0) < 0.1);

	TestTrue(TEXT("NewGuid ist gültig"), Stream.NewGuid().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisTimestampTest, "Genesis.Core.Time.Timestamp", GenesisCoreTests::Flags)
bool FGenesisTimestampTest::RunTest(const FString& Parameters)
{
	const FGenesisTimestamp Time = FGenesisTimestamp::FromCalendar(1400, 42, 13, 30);
	TestEqual(TEXT("Jahr"), Time.GetYear(), 1400ll);
	TestEqual(TEXT("Tag"), Time.GetDayOfYear(), 42);
	TestEqual(TEXT("Stunde"), Time.GetHourOfDay(), 13);
	TestEqual(TEXT("Minute"), Time.GetMinuteOfHour(), 30);

	// Negative Zeitpunkte (vor Epochenursprung) müssen korrekt abrunden
	const FGenesisTimestamp BeforeOrigin(-1);
	TestEqual(TEXT("Jahr vor Ursprung"), BeforeOrigin.GetYear(), -1ll);
	TestEqual(TEXT("Letzter Tag des Vorjahres"), BeforeOrigin.GetDayOfYear(), 364);
	TestEqual(TEXT("Letzte Stunde"), BeforeOrigin.GetHourOfDay(), 23);

	const FGenesisTimestamp Later = Time + FGenesisTimestamp::YearsToSeconds(12.5);
	TestTrue(TEXT("Jahresdifferenz"), FMath::IsNearlyEqual(FGenesisTimestamp::YearsBetween(Time, Later), 12.5, 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMathTest, "Genesis.Core.Math", GenesisCoreTests::Flags)
bool FGenesisMathTest::RunTest(const FString& Parameters)
{
	// Sättigung: Aus der Mitte voller Effekt, nahe am Extrem stark gedämpft, zurück zur Mitte voller Effekt
	TestTrue(TEXT("Aus der Mitte voller Effekt"), FMath::IsNearlyEqual(GenesisMath::ApplySaturatingDelta(0.0f, 10.0f, -100.0f, 100.0f), 10.0f));
	TestTrue(TEXT("Nahe am Extrem gedämpft"), GenesisMath::ApplySaturatingDelta(90.0f, 10.0f, -100.0f, 100.0f) < 92.0f);
	TestTrue(TEXT("Zur Mitte voller Effekt"), FMath::IsNearlyEqual(GenesisMath::ApplySaturatingDelta(90.0f, -10.0f, -100.0f, 100.0f), 80.0f));
	TestTrue(TEXT("Nie über Grenze"), GenesisMath::ApplySaturatingDelta(99.9f, 1000.0f, -100.0f, 100.0f, 0.0f) <= 100.0f);

	TestTrue(TEXT("Halbwertszeit"), FMath::IsNearlyEqual(GenesisMath::ExponentialDecay(8.0, 2.0, 1.0), 2.0, 1.0e-9));
	TestTrue(TEXT("Approach zeitschrittunabhängig"),
		FMath::IsNearlyEqual(
			GenesisMath::ApproachExponential(GenesisMath::ApproachExponential(0.0f, 1.0f, 0.1f, 1.0), 1.0f, 0.1f, 1.0),
			GenesisMath::ApproachExponential(0.0f, 1.0f, 0.1f, 2.0), 1.0e-5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisPersistenceRoundTripTest, "Genesis.Core.Persistence.StructRoundTrip", GenesisCoreTests::Flags)
bool FGenesisPersistenceRoundTripTest::RunTest(const FString& Parameters)
{
	FGenesisRandomStream Original(555);
	Original.NextUInt64();
	Original.NextUInt64();

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben erfolgreich"), GenesisPersistence::Write(Original, Bytes));
	TestTrue(TEXT("Payload nicht leer"), Bytes.Num() > 0);

	FGenesisRandomStream Restored;
	TestTrue(TEXT("Lesen erfolgreich"), GenesisPersistence::Read(Restored, Bytes));
	TestEqual(TEXT("Seed wiederhergestellt"), Restored.GetSeed(), Original.GetSeed());
	TestEqual(TEXT("Strom setzt exakt fort"), Restored.NextUInt64(), Original.NextUInt64());

	FGenesisWorldClockState ClockState;
	ClockState.Now = FGenesisTimestamp::FromCalendar(2150, 3);
	ClockState.TimeScale = 12.0;
	TArray<uint8> ClockBytes;
	GenesisPersistence::Write(ClockState, ClockBytes);
	FGenesisWorldClockState RestoredClock;
	TestTrue(TEXT("Clock lesen"), GenesisPersistence::Read(RestoredClock, ClockBytes));
	TestEqual(TEXT("Clock Zeitpunkt"), RestoredClock.Now.Seconds, ClockState.Now.Seconds);
	TestTrue(TEXT("Clock TimeScale"), FMath::IsNearlyEqual(RestoredClock.TimeScale, 12.0));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
