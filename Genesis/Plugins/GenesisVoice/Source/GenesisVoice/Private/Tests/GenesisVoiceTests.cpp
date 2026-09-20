// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceExport.h"
#include "GenesisVoiceLogic.h"
#include "GenesisVoiceSynth.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Eigener Namensraum: Im Unity-Build landen mehrere Testdateien in derselben Übersetzungseinheit.
namespace GenesisVoiceTests
{
	constexpr EAutomationTestFlags VoiceTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float TestSampleRate = 48000.0f;

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

	/** Energie oberhalb einer Grenzfrequenz – das Maß für "hell" gegen "dumpf". */
	float ComputeHighBandRms(const TArray<float>& Samples, float CutoffHz)
	{
		const float Alpha = 1.0f - FMath::Exp(-2.0f * PI * CutoffHz / TestSampleRate);
		float LowPass = 0.0f;
		double Sum = 0.0;
		for (float Sample : Samples)
		{
			LowPass += (Sample - LowPass) * Alpha;
			const double High = static_cast<double>(Sample) - LowPass;
			Sum += High * High;
		}
		return Samples.Num() > 0 ? static_cast<float>(FMath::Sqrt(Sum / Samples.Num())) : 0.0f;
	}

	/**
	 * Grundfrequenz über Autokorrelation. Liefert zusätzlich die Periodizität:
	 * eine glatte Stimme erreicht Werte nahe 1, eine heisere deutlich weniger.
	 */
	float DetectPitch(const TArray<float>& Samples, int32 Start, int32 Count, float& OutPeriodicity)
	{
		OutPeriodicity = 0.0f;
		const int32 End = FMath::Min(Samples.Num(), Start + Count);
		const int32 MinLag = FMath::RoundToInt(TestSampleRate / 900.0f);
		const int32 MaxLag = FMath::RoundToInt(TestSampleRate / 60.0f);
		if (End - Start <= MaxLag * 2)
		{
			return 0.0f;
		}

		double Energy = 0.0;
		for (int32 Index = Start; Index < End - MaxLag; ++Index)
		{
			Energy += static_cast<double>(Samples[Index]) * Samples[Index];
		}
		if (Energy <= 0.0)
		{
			return 0.0f;
		}

		int32 BestLag = 0;
		double BestValue = 0.0;
		for (int32 Lag = MinLag; Lag <= MaxLag; ++Lag)
		{
			double Sum = 0.0;
			for (int32 Index = Start; Index < End - MaxLag; ++Index)
			{
				Sum += static_cast<double>(Samples[Index]) * Samples[Index + Lag];
			}
			if (Sum > BestValue)
			{
				BestValue = Sum;
				BestLag = Lag;
			}
		}

		if (BestLag <= 0)
		{
			return 0.0f;
		}
		OutPeriodicity = static_cast<float>(BestValue / Energy);
		return TestSampleRate / static_cast<float>(BestLag);
	}

	FGenesisVoiceProfile MakeProfile(EGenesisBiologicalSex Sex, float AgeYears, float HeightCm, float Illness = 0.0f, float Arousal = 0.25f)
	{
		FGenesisVoiceInputs Inputs;
		Inputs.Sex = Sex;
		Inputs.AgeYears = AgeYears;
		Inputs.HeightCm = HeightCm;
		Inputs.Illness = Illness;
		Inputs.Arousal = Arousal;
		Inputs.IndividualSeed = 0x47454E45ull;
		return GenesisVoiceLogic::BuildProfile(Inputs, FGenesisVoiceTuning());
	}

	TArray<float> Render(const FGenesisVoiceProfile& Profile, EGenesisUtterance Type, float Seconds, float Intensity = 0.6f, uint64 Seed = 0x53454544ull)
	{
		FGenesisUtterance Utterance;
		Utterance.Type = Type;
		Utterance.Intensity = Intensity;
		Utterance.DurationSeconds = Seconds;
		Utterance.Seed = Seed;

		TArray<float> Samples;
		GenesisVoiceExport::RenderUtterance(Profile, Utterance, TestSampleRate, Seconds, Samples);
		return Samples;
	}
}

// Kein using auf Dateiebene: Im Unity-Build würden gleichnamige Hilfsfunktionen anderer Testdateien
// dadurch mehrdeutig. Die Tests holen sich den Namensraum jeweils in ihrem Rumpf.

/**
 * Die Grundfrequenz über ein Leben. Das ist die Kernbehauptung dieses Blocks:
 * Eine Stimme ist nicht eingestellt, sie ist gewachsen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoicePitchAcrossLifeTest, "Genesis.Voice.PitchAcrossLife", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoicePitchAcrossLifeTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceTuning Tuning;

	const float Newborn = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Female, 0.0f, Tuning);
	const float Child = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Male, 6.0f, Tuning);
	const float ChildFemale = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Female, 6.0f, Tuning);
	const float BeforeChange = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Male, 11.0f, Tuning);
	const float AdultMale = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Male, 25.0f, Tuning);
	const float AdultFemale = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Female, 25.0f, Tuning);
	const float OldMale = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Male, 80.0f, Tuning);
	const float OldFemale = GenesisVoiceLogic::GetBaseF0Hz(EGenesisBiologicalSex::Female, 80.0f, Tuning);

	AddInfo(FString::Printf(TEXT("Grundfrequenz: Neugeborenes %.0f Hz, Kind %.0f Hz, vor dem Stimmbruch %.0f Hz"),
		Newborn, Child, BeforeChange));
	AddInfo(FString::Printf(TEXT("Erwachsen: Mann %.0f Hz, Frau %.0f Hz | mit 80: Mann %.0f Hz, Frau %.0f Hz"),
		AdultMale, AdultFemale, OldMale, OldFemale));

	TestTrue(TEXT("Ein Neugeborenes ist am höchsten"), Newborn > Child + 100.0f);
	TestTrue(TEXT("Vor dem Stimmbruch klingen Jungen und Mädchen gleich"), FMath::IsNearlyEqual(Child, ChildFemale, 0.5f));
	TestTrue(TEXT("Der Stimmbruch senkt die Männerstimme um etwa eine Oktave"), BeforeChange / AdultMale > 1.8f);
	TestTrue(TEXT("Die Frauenstimme sinkt viel weniger"), BeforeChange / AdultFemale < 1.4f);
	TestTrue(TEXT("Im Alter steigt die Männerstimme wieder"), OldMale > AdultMale + 10.0f);
	TestTrue(TEXT("Im Alter sinkt die Frauenstimme"), OldFemale < AdultFemale - 10.0f);
	TestTrue(TEXT("Die Stimmen nähern sich im Alter an"),
		(OldFemale - OldMale) < (AdultFemale - AdultMale) - 25.0f);

	return true;
}

/** Was das Profil verspricht, muss die Synthese auch erzeugen – sonst ist die Zahl nur Behauptung. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceSynthesisMatchesProfileTest, "Genesis.Voice.SynthesisMatchesProfile", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceSynthesisMatchesProfileTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceProfile Man = MakeProfile(EGenesisBiologicalSex::Male, 35.0f, 179.0f);
	const FGenesisVoiceProfile Woman = MakeProfile(EGenesisBiologicalSex::Female, 32.0f, 167.0f);

	const TArray<float> ManSamples = Render(Man, EGenesisUtterance::Hum, 2.0f);
	const TArray<float> WomanSamples = Render(Woman, EGenesisUtterance::Hum, 2.0f);

	float ManPeriodicity = 0.0f;
	float WomanPeriodicity = 0.0f;
	const int32 Window = FMath::RoundToInt(0.4f * TestSampleRate);
	const float ManPitch = DetectPitch(ManSamples, FMath::RoundToInt(0.1f * TestSampleRate), Window, ManPeriodicity);
	const float WomanPitch = DetectPitch(WomanSamples, FMath::RoundToInt(0.1f * TestSampleRate), Window, WomanPeriodicity);

	AddInfo(FString::Printf(TEXT("Mann: Profil %.0f Hz, gemessen %.0f Hz (%.1f %%) | Frau: Profil %.0f Hz, gemessen %.0f Hz (%.1f %%)"),
		Man.F0Hz, ManPitch, 100.0f * FMath::Abs(ManPitch - Man.F0Hz) / Man.F0Hz,
		Woman.F0Hz, WomanPitch, 100.0f * FMath::Abs(WomanPitch - Woman.F0Hz) / Woman.F0Hz));

	TestTrue(TEXT("Die Männerstimme klingt so hoch wie angegeben"), FMath::Abs(ManPitch - Man.F0Hz) < Man.F0Hz * 0.06f);
	TestTrue(TEXT("Die Frauenstimme klingt so hoch wie angegeben"), FMath::Abs(WomanPitch - Woman.F0Hz) < Woman.F0Hz * 0.06f);
	TestTrue(TEXT("Es ist etwas zu hören"), ComputeRms(ManSamples) > 0.02f && ComputeRms(WomanSamples) > 0.02f);

	float Peak = 0.0f;
	for (float Sample : ManSamples)
	{
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}
	AddInfo(FString::Printf(TEXT("Spitzenwert %.3f, Effektivwert %.3f"), Peak, ComputeRms(ManSamples)));
	TestTrue(TEXT("Kein Übersteuern"), Peak <= 1.0f);

	return true;
}

/**
 * Die Klangfarbe kommt vom Körper: Ein Kind klingt nicht nur höher, sondern heller –
 * sein Ansatzrohr ist kürzer, also liegen alle Formanten oben.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceTimbreFollowsBodyTest, "Genesis.Voice.TimbreFollowsBody", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceTimbreFollowsBodyTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceProfile Newborn = MakeProfile(EGenesisBiologicalSex::Female, 0.0f, 50.0f);
	const FGenesisVoiceProfile Child = MakeProfile(EGenesisBiologicalSex::Female, 6.0f, 118.0f);
	const FGenesisVoiceProfile Man = MakeProfile(EGenesisBiologicalSex::Male, 35.0f, 179.0f);
	const FGenesisVoiceProfile TallMan = MakeProfile(EGenesisBiologicalSex::Male, 35.0f, 200.0f);

	AddInfo(FString::Printf(TEXT("Formantskalierung: Neugeborenes %.2f, Kind %.2f, Mann %.2f, großer Mann %.2f"),
		Newborn.FormantScale, Child.FormantScale, Man.FormantScale, TallMan.FormantScale));

	TestTrue(TEXT("Das Ansatzrohr eines Säuglings ist am kürzesten"), Newborn.FormantScale > Child.FormantScale);
	TestTrue(TEXT("Ein Kind klingt heller als ein Mann"), Child.FormantScale > Man.FormantScale * 1.2f);
	TestTrue(TEXT("Ein großer Mensch klingt dunkler"), TallMan.FormantScale < Man.FormantScale);

	// Und das muss man hören: gleicher Laut, gemessen oberhalb von 1,5 kHz
	const TArray<float> ChildSamples = Render(Child, EGenesisUtterance::Speak, 1.6f);
	const TArray<float> ManSamples = Render(Man, EGenesisUtterance::Speak, 1.6f);
	const float ChildHigh = ComputeHighBandRms(ChildSamples, 1500.0f) / FMath::Max(0.0001f, ComputeRms(ChildSamples));
	const float ManHigh = ComputeHighBandRms(ManSamples, 1500.0f) / FMath::Max(0.0001f, ComputeRms(ManSamples));

	AddInfo(FString::Printf(TEXT("Höhenanteil über 1,5 kHz: Kind %.3f, Mann %.3f"), ChildHigh, ManHigh));
	TestTrue(TEXT("Die Kinderstimme ist hörbar heller"), ChildHigh > ManHigh * 1.3f);

	return true;
}

/** Ein Neugeborenes kann genau eines: schreien. Und ein Schrei hat Einatempausen. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceNewbornCryTest, "Genesis.Voice.NewbornCanOnlyCry", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceNewbornCryTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	TestTrue(TEXT("Schreien kann es sofort"), GenesisVoiceLogic::CanMake(EGenesisUtterance::Cry, 0.0f));
	TestFalse(TEXT("Lachen noch nicht"), GenesisVoiceLogic::CanMake(EGenesisUtterance::Laugh, 0.0f));
	TestFalse(TEXT("Lallen noch nicht"), GenesisVoiceLogic::CanMake(EGenesisUtterance::Babble, 0.0f));
	TestTrue(TEXT("Mit einem halben Jahr lallt es"), GenesisVoiceLogic::CanMake(EGenesisUtterance::Babble, 0.5f));
	TestTrue(TEXT("Mit vier Monaten lacht es"), GenesisVoiceLogic::CanMake(EGenesisUtterance::Laugh, 0.34f));

	const FGenesisVoiceProfile Newborn = MakeProfile(EGenesisBiologicalSex::Female, 0.0f, 50.0f, 0.0f, 0.95f);
	const TArray<float> Cry = Render(Newborn, EGenesisUtterance::Cry, 4.0f, 0.95f);

	// Einatmen: In einem Schrei gibt es regelmäßig Stellen, die deutlich leiser sind als der Schrei selbst
	const int32 WindowFrames = FMath::RoundToInt(0.05f * TestSampleRate);
	float Loudest = 0.0f;
	int32 QuietWindows = 0;
	int32 TotalWindows = 0;
	for (int32 Start = 0; Start + WindowFrames < Cry.Num(); Start += WindowFrames)
	{
		const float WindowRms = ComputeRms(Cry, Start, WindowFrames);
		Loudest = FMath::Max(Loudest, WindowRms);
	}
	for (int32 Start = 0; Start + WindowFrames < Cry.Num(); Start += WindowFrames)
	{
		const float WindowRms = ComputeRms(Cry, Start, WindowFrames);
		++TotalWindows;
		if (WindowRms < Loudest * 0.2f)
		{
			++QuietWindows;
		}
	}

	const FGenesisVoiceProfile Woman = MakeProfile(EGenesisBiologicalSex::Female, 32.0f, 167.0f);
	const TArray<float> Speech = Render(Woman, EGenesisUtterance::Speak, 4.0f, 0.6f);

	AddInfo(FString::Printf(TEXT("Schrei: %.0f Hz, Effektivwert %.3f, %d von %d Fenstern leise (Atempausen) | Sprechen: %.3f"),
		Newborn.F0Hz, ComputeRms(Cry), QuietWindows, TotalWindows, ComputeRms(Speech)));

	TestTrue(TEXT("Der Schrei hat Atempausen"), QuietWindows >= 3);
	TestTrue(TEXT("Der Schrei ist lauter als ruhiges Sprechen"), ComputeRms(Cry) > ComputeRms(Speech));
	TestTrue(TEXT("Der Schrei liegt hoch"), Newborn.F0Hz > 380.0f);

	return true;
}

/** Krankheit hört man: Eine heisere Stimme schwingt unregelmäßiger und klingt leiser. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceIllnessIsAudibleTest, "Genesis.Voice.IllnessIsAudible", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceIllnessIsAudibleTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceProfile Healthy = MakeProfile(EGenesisBiologicalSex::Female, 29.0f, 165.0f, 0.0f);
	const FGenesisVoiceProfile Sick = MakeProfile(EGenesisBiologicalSex::Female, 29.0f, 165.0f, 0.85f);

	const TArray<float> HealthySamples = Render(Healthy, EGenesisUtterance::Hum, 2.0f);
	const TArray<float> SickSamples = Render(Sick, EGenesisUtterance::Hum, 2.0f);

	float HealthyPeriodicity = 0.0f;
	float SickPeriodicity = 0.0f;
	const int32 Window = FMath::RoundToInt(0.4f * TestSampleRate);
	DetectPitch(HealthySamples, FMath::RoundToInt(0.1f * TestSampleRate), Window, HealthyPeriodicity);
	DetectPitch(SickSamples, FMath::RoundToInt(0.1f * TestSampleRate), Window, SickPeriodicity);

	AddInfo(FString::Printf(TEXT("Gesund: Rauigkeit %.2f, Periodizität %.3f, laut %.3f | krank: Rauigkeit %.2f, Periodizität %.3f, laut %.3f"),
		Healthy.Roughness, HealthyPeriodicity, ComputeRms(HealthySamples),
		Sick.Roughness, SickPeriodicity, ComputeRms(SickSamples)));

	TestTrue(TEXT("Krankheit macht die Stimme rau"), Sick.Roughness > Healthy.Roughness + 0.3f);
	TestTrue(TEXT("Die Schwingung wird messbar unregelmäßiger"), SickPeriodicity < HealthyPeriodicity - 0.01f);
	TestTrue(TEXT("Die Stimme wird leiser"), ComputeRms(SickSamples) < ComputeRms(HealthySamples));
	TestTrue(TEXT("Und tiefer, weil die Schleimhaut anschwillt"), Sick.F0Hz < Healthy.F0Hz);

	return true;
}

/**
 * Die Stimme der Mutter, gehört aus dem Mutterleib. Das ist die Brücke zu den ersten Minuten:
 * Das Kind kennt diese Stimme, weil es sie neun Monate lang gehört hat – gedämpft, aber mit ihrer Melodie.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceWombTest, "Genesis.Voice.WombMufflesTheMother", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceWombTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceProfile Mother = MakeProfile(EGenesisBiologicalSex::Female, 30.0f, 168.0f);

	TArray<float> InAir = Render(Mother, EGenesisUtterance::Speak, 3.0f);
	TArray<float> InWomb = InAir;

	FGenesisVoiceHearing Womb;
	Womb.CutoffHz = 400.0f;
	Womb.Gain = 0.8f;
	Womb.Process(InWomb.GetData(), InWomb.Num(), TestSampleRate);

	const float AirHigh = ComputeHighBandRms(InAir, 2000.0f);
	const float WombHigh = ComputeHighBandRms(InWomb, 2000.0f);

	float AirPeriodicity = 0.0f;
	float WombPeriodicity = 0.0f;
	const int32 Window = FMath::RoundToInt(0.3f * TestSampleRate);
	const float AirPitch = DetectPitch(InAir, FMath::RoundToInt(0.3f * TestSampleRate), Window, AirPeriodicity);
	const float WombPitch = DetectPitch(InWomb, FMath::RoundToInt(0.3f * TestSampleRate), Window, WombPeriodicity);

	AddInfo(FString::Printf(TEXT("Höhen über 2 kHz: an Luft %.4f, im Mutterleib %.4f (Faktor %.1f)"),
		AirHigh, WombHigh, AirHigh / FMath::Max(0.00001f, WombHigh)));
	AddInfo(FString::Printf(TEXT("Tonhöhe: an Luft %.0f Hz, im Mutterleib %.0f Hz"), AirPitch, WombPitch));

	TestTrue(TEXT("Im Mutterleib fehlen die Höhen"), AirHigh > WombHigh * 5.0f);
	TestTrue(TEXT("Die Stimme bleibt trotzdem hörbar"), ComputeRms(InWomb) > 0.01f);
	TestTrue(TEXT("Die Melodie bleibt erhalten – daran erkennt das Kind die Mutter"),
		FMath::Abs(AirPitch - WombPitch) < AirPitch * 0.08f);

	return true;
}

/** Gleiche Person, gleiche Stimme. Ohne das wäre keine Messung etwas wert. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisVoiceDeterminismTest, "Genesis.Voice.Determinism", GenesisVoiceTests::VoiceTestFlags)

bool FGenesisVoiceDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace GenesisVoiceTests;
	const FGenesisVoiceProfile Profile = MakeProfile(EGenesisBiologicalSex::Male, 35.0f, 179.0f);

	const TArray<float> First = Render(Profile, EGenesisUtterance::Speak, 1.5f, 0.6f, 4711);
	const TArray<float> Second = Render(Profile, EGenesisUtterance::Speak, 1.5f, 0.6f, 4711);
	const TArray<float> Other = Render(Profile, EGenesisUtterance::Speak, 1.5f, 0.6f, 1234);

	TestEqual(TEXT("Gleiche Länge"), Second.Num(), First.Num());
	float MaxDifference = 0.0f;
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		MaxDifference = FMath::Max(MaxDifference, FMath::Abs(First[Index] - Second[Index]));
	}
	AddInfo(FString::Printf(TEXT("Größter Unterschied bei gleichem Seed: %.8f"), MaxDifference));
	TestTrue(TEXT("Gleicher Seed, gleiche Stimme"), MaxDifference < 1e-6f);

	float OtherDifference = 0.0f;
	for (int32 Index = 0; Index < FMath::Min(First.Num(), Other.Num()); ++Index)
	{
		OtherDifference = FMath::Max(OtherDifference, FMath::Abs(First[Index] - Other[Index]));
	}
	TestTrue(TEXT("Anderer Seed, anderer Laut"), OtherDifference > 0.01f);

	// Zwei Menschen gleichen Alters klingen nicht gleich
	FGenesisVoiceInputs A;
	A.Sex = EGenesisBiologicalSex::Male;
	A.AgeYears = 35.0f;
	A.HeightCm = 179.0f;
	A.IndividualSeed = 111;
	FGenesisVoiceInputs B = A;
	B.IndividualSeed = 222;

	const FGenesisVoiceProfile ProfileA = GenesisVoiceLogic::BuildProfile(A, FGenesisVoiceTuning());
	const FGenesisVoiceProfile ProfileB = GenesisVoiceLogic::BuildProfile(B, FGenesisVoiceTuning());
	AddInfo(FString::Printf(TEXT("Zwei Männer, 35 Jahre, 179 cm: %.0f Hz gegen %.0f Hz"), ProfileA.F0Hz, ProfileB.F0Hz));
	TestTrue(TEXT("Zwei Menschen klingen verschieden"), FMath::Abs(ProfileA.F0Hz - ProfileB.F0Hz) > 1.0f);

	return true;
}

#endif
