// GENESIS: Der Kreislauf des Lebens

#include "GenesisAudioCoreLogic.h"
#include "GenesisAudioTypes.h"
#include "GenesisWorldSoundExport.h"
#include "GenesisWorldSoundSynth.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Eigener Namensraum: Im Unity-Build landen mehrere Testdateien in derselben Übersetzungseinheit.
namespace GenesisWorldSoundTests
{
	constexpr EAutomationTestFlags WorldSoundFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float TestSampleRate = 48000.0f;

	float ComputeRms(const TArray<float>& Samples)
	{
		double Sum = 0.0;
		for (float Sample : Samples)
		{
			Sum += static_cast<double>(Sample) * Sample;
		}
		return Samples.Num() > 0 ? static_cast<float>(FMath::Sqrt(Sum / Samples.Num())) : 0.0f;
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

	TArray<float> RenderPlace(EGenesisPlace Place, float Seconds)
	{
		TArray<float> Samples;
		GenesisWorldSoundExport::RenderPlace(Place, TestSampleRate, Seconds, Samples);
		return Samples;
	}
}

/**
 * Orte klingen verschieden – und zwar messbar. Der Mutterleib ist dumpf, der Kreißsaal hell,
 * der Eileiter liegt dazwischen und schimmert oben.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisWorldSoundPlacesTest, "Genesis.WorldSound.PlacesSoundDifferent", GenesisWorldSoundTests::WorldSoundFlags)

bool FGenesisWorldSoundPlacesTest::RunTest(const FString& Parameters)
{
	using namespace GenesisWorldSoundTests;

	const TArray<float> Womb = RenderPlace(EGenesisPlace::Womb, 6.0f);
	const TArray<float> Room = RenderPlace(EGenesisPlace::DeliveryRoom, 6.0f);
	const TArray<float> Oviduct = RenderPlace(EGenesisPlace::OviductAmpulla, 6.0f);
	const TArray<float> Silence = RenderPlace(EGenesisPlace::None, 2.0f);

	const float WombHigh = ComputeHighBandRms(Womb, 2000.0f) / FMath::Max(0.0001f, ComputeRms(Womb));
	const float RoomHigh = ComputeHighBandRms(Room, 2000.0f) / FMath::Max(0.0001f, ComputeRms(Room));
	const float OviductHigh = ComputeHighBandRms(Oviduct, 2000.0f) / FMath::Max(0.0001f, ComputeRms(Oviduct));

	AddInfo(FString::Printf(TEXT("Höhenanteil über 2 kHz: Mutterleib %.4f, Eileiter %.4f, Kreißsaal %.4f"),
		WombHigh, OviductHigh, RoomHigh));
	AddInfo(FString::Printf(TEXT("Lautheit: Mutterleib %.3f, Eileiter %.3f, Kreißsaal %.3f"),
		ComputeRms(Womb), ComputeRms(Oviduct), ComputeRms(Room)));

	TestTrue(TEXT("Der Kreißsaal ist heller als der Mutterleib"), RoomHigh > WombHigh * 3.0f);
	TestTrue(TEXT("Der Eileiter schimmert oben mehr als der Mutterleib"), OviductHigh > WombHigh * 1.5f);
	TestTrue(TEXT("Jeder Ort ist hörbar"), ComputeRms(Womb) > 0.01f && ComputeRms(Room) > 0.01f && ComputeRms(Oviduct) > 0.01f);
	TestTrue(TEXT("Kein Ort ist Stille"), ComputeRms(Silence) < 0.0001f);

	float Peak = 0.0f;
	for (float Sample : Room)
	{
		Peak = FMath::Max(Peak, FMath::Abs(Sample));
	}
	AddInfo(FString::Printf(TEXT("Spitzenwert Kreißsaal %.3f"), Peak));
	TestTrue(TEXT("Kein Übersteuern"), Peak <= 1.0f);

	return true;
}

/**
 * Darmgeräusche der Mutter – in GENESIS-012 als fehlend notiert, hier nachgeholt.
 * Beim Menschen treten sie 5- bis 30-mal je Minute auf; genau das muss zählbar sein.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisWorldSoundWombTest, "Genesis.WorldSound.WombHasBowelSounds", GenesisWorldSoundTests::WorldSoundFlags)

bool FGenesisWorldSoundWombTest::RunTest(const FString& Parameters)
{
	using namespace GenesisWorldSoundTests;

	FGenesisWorldSoundParams Params = GenesisWorldSoundExport::GetPresetParams(EGenesisPlace::Womb);
	Params.Digestion = 0.6f;
	Params.MaternalHeartRateBpm = 78.0f;

	FGenesisWorldSoundSynth Synth(0x4F52544Full);
	Synth.Initialize(TestSampleRate);
	Synth.SetParams(Params);

	TArray<float> Samples;
	Samples.SetNumUninitialized(FMath::RoundToInt(120.0f * TestSampleRate));
	Synth.Render(Samples.GetData(), Samples.Num());

	const float GurglesPerMinute = Synth.GetEventCount(EGenesisWorldEvent::Gurgle) / 2.0f;
	const float WhooshPerMinute = Synth.GetEventCount(EGenesisWorldEvent::PlacentalWhoosh) / 2.0f;

	AddInfo(FString::Printf(TEXT("In zwei Minuten: %d Darmgeräusche (%.1f/min, vorgesehen %.1f), %d Mutterkuchen-Rauschen (%.1f/min bei Puls %.0f)"),
		Synth.GetEventCount(EGenesisWorldEvent::Gurgle), GurglesPerMinute,
		Synth.GetEventRatePerMinute(EGenesisWorldEvent::Gurgle),
		Synth.GetEventCount(EGenesisWorldEvent::PlacentalWhoosh), WhooshPerMinute, Params.MaternalHeartRateBpm));

	TestTrue(TEXT("Darmgeräusche liegen im menschlichen Bereich"), GurglesPerMinute >= 5.0f && GurglesPerMinute <= 30.0f);
	TestTrue(TEXT("Der Mutterkuchen rauscht im Takt des mütterlichen Herzens"),
		FMath::Abs(WhooshPerMinute - Params.MaternalHeartRateBpm) < 4.0f);

	// Eine verdauende Mutter gluckert öfter als eine ruhende
	FGenesisWorldSoundParams Quiet = Params;
	Quiet.Digestion = 0.0f;
	FGenesisWorldSoundSynth QuietSynth(0x4F52544Full);
	QuietSynth.Initialize(TestSampleRate);
	QuietSynth.SetParams(Quiet);
	TArray<float> QuietSamples;
	QuietSamples.SetNumUninitialized(FMath::RoundToInt(120.0f * TestSampleRate));
	QuietSynth.Render(QuietSamples.GetData(), QuietSamples.Num());

	AddInfo(FString::Printf(TEXT("Ruhende Verdauung: %d Darmgeräusche in zwei Minuten"),
		QuietSynth.GetEventCount(EGenesisWorldEvent::Gurgle)));
	TestTrue(TEXT("Wer verdaut, gluckert öfter"),
		Synth.GetEventCount(EGenesisWorldEvent::Gurgle) > QuietSynth.GetEventCount(EGenesisWorldEvent::Gurgle) + 10);

	return true;
}

/** Im Kreißsaal hängt die Häufigkeit daran, wie viel gerade geschieht. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisWorldSoundRoomTest, "Genesis.WorldSound.RoomFollowsActivity", GenesisWorldSoundTests::WorldSoundFlags)

bool FGenesisWorldSoundRoomTest::RunTest(const FString& Parameters)
{
	using namespace GenesisWorldSoundTests;

	auto CountEvents = [](float Activity, float HeartRate, int32& OutClinks, int32& OutSteps, int32& OutBeeps)
	{
		FGenesisWorldSoundParams Params = GenesisWorldSoundExport::GetPresetParams(EGenesisPlace::DeliveryRoom);
		Params.Activity = Activity;
		Params.MaternalHeartRateBpm = HeartRate;

		FGenesisWorldSoundSynth Synth(0x52554D45ull);
		Synth.Initialize(TestSampleRate);
		Synth.SetParams(Params);

		TArray<float> Samples;
		Samples.SetNumUninitialized(FMath::RoundToInt(120.0f * TestSampleRate));
		Synth.Render(Samples.GetData(), Samples.Num());

		OutClinks = Synth.GetEventCount(EGenesisWorldEvent::Clink);
		OutSteps = Synth.GetEventCount(EGenesisWorldEvent::Footstep);
		OutBeeps = Synth.GetEventCount(EGenesisWorldEvent::Monitor);
	};

	int32 CalmClinks = 0, CalmSteps = 0, CalmBeeps = 0;
	int32 BusyClinks = 0, BusySteps = 0, BusyBeeps = 0;
	CountEvents(0.05f, 82.0f, CalmClinks, CalmSteps, CalmBeeps);
	CountEvents(0.95f, 124.0f, BusyClinks, BusySteps, BusyBeeps);

	AddInfo(FString::Printf(TEXT("Ruhig (2 min): %d Instrumente, %d Schritte, %d Monitortöne bei Puls 82"),
		CalmClinks, CalmSteps, CalmBeeps));
	AddInfo(FString::Printf(TEXT("Austreibung (2 min): %d Instrumente, %d Schritte, %d Monitortöne bei Puls 124"),
		BusyClinks, BusySteps, BusyBeeps));

	TestTrue(TEXT("Unter der Geburt klappert es öfter"), BusyClinks > CalmClinks * 2);
	TestTrue(TEXT("Unter der Geburt läuft mehr Personal"), BusySteps > CalmSteps * 2);
	TestTrue(TEXT("Der Monitor folgt dem Puls der Mutter"),
		FMath::Abs(CalmBeeps / 2.0f - 82.0f) < 4.0f && FMath::Abs(BusyBeeps / 2.0f - 124.0f) < 5.0f);

	return true;
}

/** Derselbe Seed, derselbe Raum. Sonst wäre keine Messung etwas wert. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisWorldSoundDeterminismTest, "Genesis.WorldSound.Determinism", GenesisWorldSoundTests::WorldSoundFlags)

bool FGenesisWorldSoundDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace GenesisWorldSoundTests;

	const TArray<float> First = RenderPlace(EGenesisPlace::DeliveryRoom, 4.0f);
	const TArray<float> Second = RenderPlace(EGenesisPlace::DeliveryRoom, 4.0f);

	float MaxDifference = 0.0f;
	for (int32 Index = 0; Index < FMath::Min(First.Num(), Second.Num()); ++Index)
	{
		MaxDifference = FMath::Max(MaxDifference, FMath::Abs(First[Index] - Second[Index]));
	}
	AddInfo(FString::Printf(TEXT("Größter Unterschied bei gleichem Seed: %.8f"), MaxDifference));
	TestTrue(TEXT("Gleicher Seed, gleicher Raum"), MaxDifference < 1e-6f);

	FGenesisWorldSoundSynth Other(999);
	Other.Initialize(TestSampleRate);
	Other.SetParams(GenesisWorldSoundExport::GetPresetParams(EGenesisPlace::DeliveryRoom));
	TArray<float> OtherSamples;
	OtherSamples.SetNumUninitialized(First.Num());
	Other.Render(OtherSamples.GetData(), OtherSamples.Num());

	float OtherDifference = 0.0f;
	for (int32 Index = 0; Index < First.Num(); ++Index)
	{
		OtherDifference = FMath::Max(OtherDifference, FMath::Abs(First[Index] - OtherSamples[Index]));
	}
	TestTrue(TEXT("Anderer Seed, anderer Verlauf"), OtherDifference > 0.01f);

	return true;
}

/**
 * Die Mischung muss wirken: Wenn jemand spricht, tritt der Raum zurück – und kommt langsamer
 * zurück, als er gegangen ist. Das ist der Unterschied zwischen einem Mix und einem Haufen Klänge.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisWorldSoundDuckingTest, "Genesis.WorldSound.VoiceDucksTheRoom", GenesisWorldSoundTests::WorldSoundFlags)

bool FGenesisWorldSoundDuckingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisWorldSoundTests;

	const FGenesisAudioMixTuning Tuning;
	FGenesisMixState State;

	// Zwei Sekunden Stille im Mix: Alles steht auf 0 dB
	for (int32 Step = 0; Step < 120; ++Step)
	{
		GenesisAudioCoreLogic::UpdateMix(State, {}, Tuning, 1.0f / 60.0f);
	}
	const float RestingAmbient = State.GetGainDb(EGenesisAudioBus::Ambient);
	const float RestingMusic = State.GetGainDb(EGenesisAudioBus::Music);

	// Jemand spricht
	FGenesisMixRequest Speech;
	Speech.Bus = EGenesisAudioBus::Dialogue;
	Speech.Importance = 1.0f;
	for (int32 Step = 0; Step < 120; ++Step)
	{
		GenesisAudioCoreLogic::UpdateMix(State, { Speech }, Tuning, 1.0f / 60.0f);
	}
	const float DuckedAmbient = State.GetGainDb(EGenesisAudioBus::Ambient);
	const float DuckedMusic = State.GetGainDb(EGenesisAudioBus::Music);

	// Eine halbe Sekunde nach dem Satz
	for (int32 Step = 0; Step < 30; ++Step)
	{
		GenesisAudioCoreLogic::UpdateMix(State, {}, Tuning, 1.0f / 60.0f);
	}
	const float ReturningAmbient = State.GetGainDb(EGenesisAudioBus::Ambient);

	AddInfo(FString::Printf(TEXT("Umgebung: Ruhe %.1f dB → beim Sprechen %.1f dB → 0,5 s danach %.1f dB"),
		RestingAmbient, DuckedAmbient, ReturningAmbient));
	AddInfo(FString::Printf(TEXT("Musik: Ruhe %.1f dB → beim Sprechen %.1f dB"), RestingMusic, DuckedMusic));

	TestTrue(TEXT("Der Raum tritt beim Sprechen zurück"), DuckedAmbient < RestingAmbient - 2.0f);
	TestTrue(TEXT("Die Musik tritt beim Sprechen zurück"), DuckedMusic < RestingMusic - 2.0f);
	TestTrue(TEXT("Und kommt langsamer zurück, als sie gegangen ist"),
		ReturningAmbient > DuckedAmbient && ReturningAmbient < RestingAmbient - 0.5f);

	return true;
}

#endif
