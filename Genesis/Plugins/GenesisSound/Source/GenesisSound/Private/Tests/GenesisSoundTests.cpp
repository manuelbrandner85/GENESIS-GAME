// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodySynth.h"
#include "GenesisSoundExport.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Eigener Namensraum: Im Unity-Build landen mehrere Testdateien in derselben Übersetzungseinheit,
// und gleichnamige Hilfsfunktionen kollidieren dort.
namespace GenesisBodySoundTests
{
	constexpr EAutomationTestFlags SoundTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float TestSampleRate = 48000.0f;

	/** Effektivwert – das Maß für "wie laut". */
	float ComputeRms(const TArray<float>& Samples, int32 Start = 0, int32 Count = -1)
	{
		const int32 End = Count < 0 ? Samples.Num() : FMath::Min(Samples.Num(), Start + Count);
		double Sum = 0.0;
		int32 Counted = 0;
		for (int32 Index = FMath::Max(0, Start); Index < End; ++Index)
		{
			Sum += static_cast<double>(Samples[Index]) * Samples[Index];
			++Counted;
		}
		return Counted > 0 ? static_cast<float>(FMath::Sqrt(Sum / Counted)) : 0.0f;
	}

	/**
	 * Energie oberhalb einer Grenzfrequenz, gemessen mit einem einpoligen Hochpass.
	 * Das genügt, um "dumpf" von "hell" zu unterscheiden – dafür braucht es keine Spektralanalyse.
	 */
	float ComputeHighBandRms(const TArray<float>& Samples, float CutoffHz, int32 Start = 0, int32 Count = -1)
	{
		const int32 End = Count < 0 ? Samples.Num() : FMath::Min(Samples.Num(), Start + Count);
		const float Tau = 1.0f / (2.0f * PI * CutoffHz);
		const float Alpha = 1.0f - FMath::Exp(-1.0f / (Tau * TestSampleRate));

		float LowPass = 0.0f;
		double Sum = 0.0;
		int32 Counted = 0;
		for (int32 Index = FMath::Max(0, Start); Index < End; ++Index)
		{
			LowPass += (Samples[Index] - LowPass) * Alpha;
			const double High = static_cast<double>(Samples[Index]) - LowPass;
			Sum += High * High;
			++Counted;
		}
		return Counted > 0 ? static_cast<float>(FMath::Sqrt(Sum / Counted)) : 0.0f;
	}

	TArray<float> RenderSynth(const FGenesisBodySoundParams& Params, float Seconds, uint64 Seed = 0x50554C53ull)
	{
		FGenesisBodySynth Synth(Seed);
		Synth.Initialize(TestSampleRate);
		Synth.SetParams(Params);

		TArray<float> Samples;
		Samples.SetNumUninitialized(FMath::RoundToInt(Seconds * TestSampleRate));
		Synth.Render(Samples.GetData(), Samples.Num());
		return Samples;
	}
}

/** Der Herzschlag muss zählbar sein: 120 Schläge je Minute heißt 120 Schläge in einer Minute. */
using namespace GenesisBodySoundTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoundHeartbeatTest, "Genesis.Sound.HeartbeatMatchesPulse", SoundTestFlags)

bool FGenesisSoundHeartbeatTest::RunTest(const FString& Parameters)
{
	FGenesisBodySoundParams Params;
	Params.HeartRateBpm = 120.0f;
	Params.MotherHeartRateBpm = 0.0f;
	Params.bInWomb = true;

	FGenesisBodySynth Synth;
	Synth.Initialize(TestSampleRate);
	Synth.SetParams(Params);

	TArray<float> Samples;
	Samples.SetNumUninitialized(FMath::RoundToInt(60.0f * TestSampleRate));
	Synth.Render(Samples.GetData(), Samples.Num());

	AddInfo(FString::Printf(TEXT("%d Schläge in 60 s bei 120/min, Effektivwert %.3f"), Synth.GetHeartbeatCount(), ComputeRms(Samples)));
	TestTrue(TEXT("120 Schläge je Minute"), FMath::Abs(Synth.GetHeartbeatCount() - 120) <= 1);

	// Nichts darf übersteuern, und still darf es auch nicht sein
	float Peak = 0.0f;
	for (float Sample : Samples)
	{
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}
	AddInfo(FString::Printf(TEXT("Spitzenwert %.3f"), Peak));
	TestTrue(TEXT("Kein Übersteuern"), Peak <= 1.0f);
	TestTrue(TEXT("Es ist etwas zu hören"), ComputeRms(Samples) > 0.01f);

	return true;
}

/**
 * Der Mutterleib ist ein Tiefpass. Das ist der ganze Unterschied zwischen Hören im Wasser und in Luft –
 * und er muss messbar sein, nicht nur behauptet.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoundWombFilterTest, "Genesis.Sound.WombIsMuffled", SoundTestFlags)

bool FGenesisSoundWombFilterTest::RunTest(const FString& Parameters)
{
	FGenesisBodySoundParams Womb;
	Womb.bInWomb = true;
	Womb.LowPassCutoffHz = 400.0f;
	Womb.ExternalAudibility = 0.1f;

	FGenesisBodySoundParams Air = Womb;
	Air.bInWomb = false;
	Air.LowPassCutoffHz = 16000.0f;
	Air.ExternalAudibility = 1.0f;

	const TArray<float> WombSamples = RenderSynth(Womb, 4.0f);
	const TArray<float> AirSamples = RenderSynth(Air, 4.0f);

	const float WombHigh = ComputeHighBandRms(WombSamples, 2000.0f);
	const float AirHigh = ComputeHighBandRms(AirSamples, 2000.0f);
	AddInfo(FString::Printf(TEXT("Energie über 2 kHz: Mutterleib %.5f, Luft %.5f (Faktor %.1f)"),
		WombHigh, AirHigh, AirHigh / FMath::Max(0.00001f, WombHigh)));

	TestTrue(TEXT("Im Mutterleib fehlen die Höhen"), AirHigh > WombHigh * 5.0f);
	TestTrue(TEXT("Im Mutterleib ist trotzdem etwas zu hören"), ComputeRms(WombSamples) > 0.01f);

	return true;
}

/** Die Geburt ist ein Sprung: Der Ton wird innerhalb eines Augenblicks hell. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoundBirthJumpTest, "Genesis.Sound.BirthOpensTheEars", SoundTestFlags)

bool FGenesisSoundBirthJumpTest::RunTest(const FString& Parameters)
{
	TArray<float> Samples;
	GenesisSoundExport::RenderPreset(GenesisSoundExport::EPreset::Birth, TestSampleRate, 10.0f, Samples);

	const int32 Moment = FMath::RoundToInt(10.0f * 0.55f * TestSampleRate);
	const int32 Window = FMath::RoundToInt(2.0f * TestSampleRate);
	const float Before = ComputeHighBandRms(Samples, 2000.0f, Moment - Window, Window);
	const float After = ComputeHighBandRms(Samples, 2000.0f, Moment + FMath::RoundToInt(0.5f * TestSampleRate), Window);

	AddInfo(FString::Printf(TEXT("Höhen vor der Geburt %.5f, danach %.5f (Faktor %.1f)"),
		Before, After, After / FMath::Max(0.00001f, Before)));
	TestTrue(TEXT("Nach der Geburt ist es hörbar heller"), After > Before * 4.0f);

	return true;
}

/** Gleiche Werte, gleicher Klang – sonst wäre kein Ton reproduzierbar und keine Messung etwas wert. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoundDeterminismTest, "Genesis.Sound.Determinism", SoundTestFlags)

bool FGenesisSoundDeterminismTest::RunTest(const FString& Parameters)
{
	FGenesisBodySoundParams Params;
	Params.HeartRateBpm = 138.0f;

	const TArray<float> First = RenderSynth(Params, 2.0f, 12345);
	const TArray<float> Second = RenderSynth(Params, 2.0f, 12345);
	const TArray<float> Other = RenderSynth(Params, 2.0f, 999);

	TestEqual(TEXT("Gleiche Länge"), Second.Num(), First.Num());
	float MaxDifference = 0.0f;
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		MaxDifference = FMath::Max(MaxDifference, FMath::Abs(First[Index] - Second[Index]));
	}
	AddInfo(FString::Printf(TEXT("Größter Unterschied bei gleichem Seed: %.8f"), MaxDifference));
	TestTrue(TEXT("Gleicher Seed, gleicher Klang"), MaxDifference < 1e-6f);

	float OtherDifference = 0.0f;
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		OtherDifference = FMath::Max(OtherDifference, FMath::Abs(First[Index] - Other[Index]));
	}
	TestTrue(TEXT("Anderer Seed, anderes Rauschen"), OtherDifference > 0.001f);

	return true;
}

/** Eine Wehe presst und dämpft: Unter Druck wird es lauter und dumpfer. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoundPressureTest, "Genesis.Sound.ContractionPresses", SoundTestFlags)

bool FGenesisSoundPressureTest::RunTest(const FString& Parameters)
{
	FGenesisBodySoundParams Calm;
	Calm.bInWomb = true;
	Calm.LowPassCutoffHz = 400.0f;
	Calm.Pressure = 0.0f;

	FGenesisBodySoundParams Contraction = Calm;
	Contraction.Pressure = 1.0f;
	Contraction.Oxygen = 0.55f;

	const TArray<float> CalmSamples = RenderSynth(Calm, 4.0f);
	const TArray<float> PressedSamples = RenderSynth(Contraction, 4.0f);

	const float CalmRms = ComputeRms(CalmSamples);
	const float PressedRms = ComputeRms(PressedSamples);
	const float CalmHigh = ComputeHighBandRms(CalmSamples, 1000.0f);
	const float PressedHigh = ComputeHighBandRms(PressedSamples, 1000.0f);

	AddInfo(FString::Printf(TEXT("ruhig: laut %.4f, hell %.5f | unter der Wehe: laut %.4f, hell %.5f"),
		CalmRms, CalmHigh, PressedRms, PressedHigh));

	TestTrue(TEXT("Unter der Wehe wird es lauter"), PressedRms > CalmRms);
	TestTrue(TEXT("Unter der Wehe wird es dumpfer"), PressedHigh / PressedRms < CalmHigh / CalmRms);

	return true;
}

#endif
