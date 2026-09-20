// GENESIS: Der Kreislauf des Lebens

#include "GenesisGameplayTags.h"
#include "GenesisMusicExport.h"
#include "GenesisMusicSynth.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisMusicTests
{
	constexpr EAutomationTestFlags MusicTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float TestSampleRate = 48000.0f;

	FGenesisMusicPhrase MakeSingleNote(int32 MidiPitch, EGenesisInstrument Instrument, float DurationBeats = 2.0f)
	{
		FGenesisMusicPhrase Phrase;
		Phrase.TempoBpm = 60.0f;
		Phrase.LengthBeats = DurationBeats + 1.0f;
		Phrase.Presence = 0.6f;
		Phrase.Space = 0.0f; // ohne Nachhall, damit die Messung den reinen Ton sieht

		FGenesisMusicNote Note;
		Note.MidiPitch = MidiPitch;
		Note.StartBeat = 0.0f;
		Note.DurationBeats = DurationBeats;
		Note.Velocity = 0.8f;
		Note.Instrument = Instrument;
		Phrase.Notes.Add(Note);
		return Phrase;
	}

	TArray<float> Render(const FGenesisMusicPhrase& Phrase, float Seconds, bool bLoop = false)
	{
		TArray<float> Samples;
		GenesisMusicExport::RenderPhraseAudio(Phrase, TestSampleRate, Seconds, Samples, bLoop);
		return Samples;
	}

	/** Grundfrequenz über Autokorrelation – die Obertöne stören die Nulldurchgänge zu sehr. */
	float EstimateFrequency(const TArray<float>& Samples, int32 Start, int32 Count)
	{
		const int32 MinLag = FMath::RoundToInt(TestSampleRate / 1200.0f);
		const int32 MaxLag = FMath::RoundToInt(TestSampleRate / 60.0f);
		const int32 End = FMath::Min(Samples.Num(), Start + Count);
		if (End - Start <= MaxLag * 2)
		{
			return 0.0f;
		}

		double BestScore = -1.0;
		int32 BestLag = 0;
		for (int32 Lag = MinLag; Lag <= MaxLag; ++Lag)
		{
			double Sum = 0.0;
			for (int32 Index = Start; Index + Lag < End; ++Index)
			{
				Sum += static_cast<double>(Samples[Index]) * Samples[Index + Lag];
			}
			Sum /= static_cast<double>(End - Start - Lag);
			if (Sum > BestScore)
			{
				BestScore = Sum;
				BestLag = Lag;
			}
		}
		return BestLag > 0 ? TestSampleRate / BestLag : 0.0f;
	}

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

	/** Anteil der Energie oberhalb einer Grenzfrequenz – das Maß für "hell" gegen "dunkel". */
	float HighBandRatio(const TArray<float>& Samples, float CutoffHz)
	{
		const float Tau = 1.0f / (2.0f * PI * CutoffHz);
		const float Alpha = 1.0f - FMath::Exp(-1.0f / (Tau * TestSampleRate));
		float LowPass = 0.0f;
		double High = 0.0;
		double Total = 0.0;
		for (float Sample : Samples)
		{
			LowPass += (Sample - LowPass) * Alpha;
			const double HighPart = static_cast<double>(Sample) - LowPass;
			High += HighPart * HighPart;
			Total += static_cast<double>(Sample) * Sample;
		}
		return Total > 0.0 ? static_cast<float>(FMath::Sqrt(High / Total)) : 0.0f;
	}
}

/** Ein Kammerton muss ein Kammerton sein: MIDI 69 sind 440 Hz, gemessen und nicht behauptet. */
using namespace GenesisMusicTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMusicPitchTest, "Genesis.Music.PitchIsAccurate", MusicTestFlags)

bool FGenesisMusicPitchTest::RunTest(const FString& Parameters)
{
	const int32 Pitches[3] = { 57, 69, 76 };   // A3, A4 (Kammerton), E5
	const float Expected[3] = { 220.0f, 440.0f, 659.26f };

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const TArray<float> Samples = Render(MakeSingleNote(Pitches[Index], EGenesisInstrument::Piano), 2.0f);
		const float Measured = EstimateFrequency(Samples, FMath::RoundToInt(0.2f * TestSampleRate), FMath::RoundToInt(0.8f * TestSampleRate));
		const float Deviation = FMath::Abs(Measured - Expected[Index]) / Expected[Index];
		AddInfo(FString::Printf(TEXT("MIDI %d: erwartet %.1f Hz, gemessen %.1f Hz (%.2f %% Abweichung)"),
			Pitches[Index], Expected[Index], Measured, 100.0f * Deviation));
		TestTrue(TEXT("Tonhöhe stimmt auf 2 % genau"), Deviation < 0.02f);
	}

	return true;
}

/** Acht Instrumente müssen acht Klänge sein – sonst wäre die Instrumentierung eine leere Angabe. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMusicTimbreTest, "Genesis.Music.InstrumentsDiffer", MusicTestFlags)

bool FGenesisMusicTimbreTest::RunTest(const FString& Parameters)
{
	const TArray<float> MusicBox = Render(MakeSingleNote(72, EGenesisInstrument::MusicBox), 2.5f);
	const TArray<float> Strings = Render(MakeSingleNote(72, EGenesisInstrument::Strings), 2.5f);
	const TArray<float> Pad = Render(MakeSingleNote(72, EGenesisInstrument::CosmicPad), 2.5f);

	const float BoxBright = HighBandRatio(MusicBox, 1500.0f);
	const float StringsBright = HighBandRatio(Strings, 1500.0f);
	const float PadBright = HighBandRatio(Pad, 1500.0f);
	AddInfo(FString::Printf(TEXT("Höhenanteil: Spieldose %.3f, Streicher %.3f, Klangfläche %.3f"),
		BoxBright, StringsBright, PadBright));
	TestTrue(TEXT("Die Spieldose klingt heller als die Klangfläche"), BoxBright > PadBright * 1.2f);

	// Der Anschlag unterscheidet die Instrumente ebenso wie die Klangfarbe: Die Spieldose ist sofort da,
	// die Klangfläche braucht über eine Sekunde.
	const int32 Window = FMath::RoundToInt(0.12f * TestSampleRate);
	const float BoxAttack = ComputeRms(MusicBox, 0, Window);
	const float PadAttack = ComputeRms(Pad, 0, Window);
	AddInfo(FString::Printf(TEXT("Erste 120 ms: Spieldose %.4f, Klangfläche %.4f"), BoxAttack, PadAttack));
	TestTrue(TEXT("Die Klangfläche schwillt langsam an"), PadAttack < BoxAttack * 0.5f);

	return true;
}

/**
 * Der Kern von Soul Music: dasselbe Motiv, andere Lebensphase, anderer Klang.
 * Wenn das nicht messbar ist, ist die ganze Idee nur eine Behauptung.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMusicPhaseTest, "Genesis.Music.PhasesSoundDifferent", MusicTestFlags)

bool FGenesisMusicPhaseTest::RunTest(const FString& Parameters)
{
	const uint64 Seed = 0x50BA5E11ull;
	const FGenesisMusicPhrase Childhood = GenesisMusicExport::BuildPhrase(Seed, GenesisTags::LifePhase_Childhood);
	const FGenesisMusicPhrase Elder = GenesisMusicExport::BuildPhrase(Seed, GenesisTags::LifePhase_Elder);

	AddInfo(FString::Printf(TEXT("Kindheit: %d Noten, %.0f BPM, Raum %.2f | Alter: %d Noten, %.0f BPM, Raum %.2f"),
		Childhood.Notes.Num(), Childhood.TempoBpm, Childhood.Space,
		Elder.Notes.Num(), Elder.TempoBpm, Elder.Space));

	TestTrue(TEXT("Beide Phasen haben Noten"), Childhood.Notes.Num() > 0 && Elder.Notes.Num() > 0);

	const TArray<float> ChildhoodAudio = Render(Childhood, 8.0f);
	const TArray<float> ElderAudio = Render(Elder, 8.0f);

	const float ChildhoodBright = HighBandRatio(ChildhoodAudio, 1200.0f);
	const float ElderBright = HighBandRatio(ElderAudio, 1200.0f);
	AddInfo(FString::Printf(TEXT("Höhenanteil: Kindheit %.3f, Alter %.3f"), ChildhoodBright, ElderBright));

	TestTrue(TEXT("Beide Phasen sind hörbar"), ComputeRms(ChildhoodAudio) > 0.01f && ComputeRms(ElderAudio) > 0.01f);
	TestTrue(TEXT("Die Phasen klingen unterschiedlich"), FMath::Abs(ChildhoodBright - ElderBright) > 0.02f);

	return true;
}

/** Nichts darf übersteuern, und nach der letzten Note darf der Raum noch nachklingen. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMusicLevelTest, "Genesis.Music.LevelAndTail", MusicTestFlags)

bool FGenesisMusicLevelTest::RunTest(const FString& Parameters)
{
	FGenesisMusicPhrase Phrase = GenesisMusicExport::BuildPhrase(0x9911ull, GenesisTags::LifePhase_Adulthood);
	Phrase.Space = 0.8f;
	const float Length = Phrase.GetLengthSeconds();
	const TArray<float> Samples = Render(Phrase, Length + 2.5f);

	float Peak = 0.0f;
	for (float Sample : Samples)
	{
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}
	const float Body = ComputeRms(Samples, 0, FMath::RoundToInt(Length * TestSampleRate));
	// Zwei Fenster nach der letzten Note: Der Raum muss leiser werden, nicht stehen bleiben.
	// Ein Vergleich mit der Musik selbst wäre falsch – dort klingen die Noten noch aus.
	const float EarlyTail = ComputeRms(Samples, FMath::RoundToInt((Length + 0.3f) * TestSampleRate), FMath::RoundToInt(0.5f * TestSampleRate));
	const float LateTail = ComputeRms(Samples, FMath::RoundToInt((Length + 1.6f) * TestSampleRate), FMath::RoundToInt(0.5f * TestSampleRate));

	AddInfo(FString::Printf(TEXT("Phrase %.1f s, Spitzenwert %.3f, Musik %.4f, Ausklang nach 0,3 s %.4f, nach 1,6 s %.4f"),
		Length, Peak, Body, EarlyTail, LateTail));
	TestTrue(TEXT("Kein Übersteuern"), Peak <= 1.0f);
	TestTrue(TEXT("Es ist Musik zu hören"), Body > 0.02f);
	TestTrue(TEXT("Der Raum klingt nach"), EarlyTail > 0.0005f);
	TestTrue(TEXT("Der Ausklang wird leiser"), LateTail < EarlyTail * 0.6f);

	return true;
}

/** Gleiches Motiv, gleicher Klang – zweimal gerendert muss dasselbe herauskommen. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMusicDeterminismTest, "Genesis.Music.Determinism", MusicTestFlags)

bool FGenesisMusicDeterminismTest::RunTest(const FString& Parameters)
{
	const FGenesisMusicPhrase Phrase = GenesisMusicExport::BuildPhrase(4242, GenesisTags::LifePhase_Youth);
	const TArray<float> First = Render(Phrase, 6.0f);
	const TArray<float> Second = Render(Phrase, 6.0f);

	float MaxDifference = 0.0f;
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		MaxDifference = FMath::Max(MaxDifference, FMath::Abs(First[Index] - Second[Index]));
	}
	AddInfo(FString::Printf(TEXT("Größter Unterschied zweier Wiedergaben: %.8f"), MaxDifference));
	TestTrue(TEXT("Zweimal gerendert klingt gleich"), MaxDifference < 1e-6f);

	// Eine andere Seele hat ein anderes Motiv
	const FGenesisMusicPhrase Other = GenesisMusicExport::BuildPhrase(777, GenesisTags::LifePhase_Youth);
	const TArray<float> OtherAudio = Render(Other, 6.0f);
	float OtherDifference = 0.0f;
	for (int32 Index = 0; Index < FMath::Min(First.Num(), OtherAudio.Num()); ++Index)
	{
		OtherDifference = FMath::Max(OtherDifference, FMath::Abs(First[Index] - OtherAudio[Index]));
	}
	TestTrue(TEXT("Eine andere Seele klingt anders"), OtherDifference > 0.01f);

	return true;
}

#endif
