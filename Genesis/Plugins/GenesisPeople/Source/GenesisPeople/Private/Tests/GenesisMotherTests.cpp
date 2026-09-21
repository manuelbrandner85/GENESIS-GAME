// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherLogic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags MotherFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Läuft die Mutter Sekunden lang mit 60 Bildern je Sekunde. */
	void Run(FGenesisMotherState& State, const FGenesisMotherTuning& Tuning, const FGenesisMotherInputs& Inputs, float Seconds)
	{
		const int32 Frames = FMath::RoundToInt(Seconds * 60.0f);
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			GenesisMotherLogic::Advance(State, Tuning, Inputs, 1.0f / 60.0f);
		}
	}
}

/**
 * Der Atem, auf dem das Kind liegt: 14 Züge je Minute, Einatmen kürzer als Ausatmen.
 * Die Kamera des Kindes rechnet mit demselben Takt – liefe er auseinander, schwebte das Kind über ihr.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMotherBreathTest, "Genesis.People.Mother.Breath", MotherFlags)

bool FGenesisMotherBreathTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMotherLogic;
	const FGenesisMotherTuning Tuning;
	FGenesisMotherState State = Begin(7);
	State.BreathPhase = 0.0f;

	int32 Peaks = 0;
	float Previous = GetBreathLift(State, Tuning);
	bool bRising = true;
	float InhaleSeconds = 0.0f;
	for (int32 Frame = 0; Frame < 60 * 60; ++Frame)
	{
		Advance(State, Tuning, FGenesisMotherInputs(), 1.0f / 60.0f);
		const float Lift = GetBreathLift(State, Tuning);
		if (Lift > Previous)
		{
			InhaleSeconds += 1.0f / 60.0f;
		}
		if (bRising && Lift < Previous)
		{
			++Peaks;
		}
		bRising = Lift >= Previous;
		Previous = Lift;
	}
	AddInfo(FString::Printf(TEXT("Atemzüge in einer Minute: %d, davon eingeatmet %.1f s"), Peaks, InhaleSeconds));
	TestEqual(TEXT("14 Atemzüge je Minute"), Peaks, 14);
	TestTrue(TEXT("Einatmen ist kürzer als Ausatmen"), InhaleSeconds < 30.0f && InhaleSeconds > 18.0f);
	TestTrue(TEXT("Hub bleibt zwischen 0 und 1"), Previous >= 0.0f && Previous <= 1.0f);
	return true;
}

/**
 * Lidschlag: im Mittel 12 je Minute, aber nicht im Takt – echte Abstände sind rechtsschief verteilt.
 * Das Lid schließt schneller, als es sich öffnet. Beim Blickkontakt wird seltener geblinzelt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMotherBlinkTest, "Genesis.People.Mother.Blink", MotherFlags)

bool FGenesisMotherBlinkTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMotherLogic;
	const FGenesisMotherTuning Tuning;

	FGenesisMotherState Resting = Begin(11);
	Run(Resting, Tuning, FGenesisMotherInputs(), 600.0f);
	const float RestingRate = Resting.Blinks / 10.0f;
	AddInfo(FString::Printf(TEXT("Lidschläge in Ruhe: %.1f je Minute"), RestingRate));
	TestTrue(TEXT("Rund 12 je Minute"), RestingRate > 9.0f && RestingRate < 15.0f);

	// Blickkontakt: Kind vor dem Gesicht, es sieht ihr in die Augen
	FGenesisMotherInputs Contact;
	Contact.bChildOnChest = true;
	Contact.bChildSeeksFace = true;
	Contact.bChildLooksAtEyes = true;
	FGenesisMotherState Gazing = Begin(11);
	Run(Gazing, Tuning, Contact, 600.0f);
	const float GazingRate = Gazing.Blinks / 10.0f;
	AddInfo(FString::Printf(TEXT("Lidschläge beim Blickkontakt: %.1f je Minute"), GazingRate));
	TestTrue(TEXT("Beim Blickkontakt seltener"), GazingRate < RestingRate * 0.85f);

	// Form eines Lidschlags
	FGenesisMotherState Blink = Begin(3);
	Blink.BlinkElapsed = 0.0f;
	float ClosedAt = -1.0f;
	float OpenAt = -1.0f;
	for (int32 Step = 1; Step <= 40; ++Step)
	{
		Blink.BlinkElapsed = Step * Tuning.BlinkSeconds / 40.0f;
		const float Closure = GetBlinkClosure(Blink, Tuning);
		if (ClosedAt < 0.0f && Closure > 0.99f)
		{
			ClosedAt = Blink.BlinkElapsed;
		}
		if (ClosedAt >= 0.0f && OpenAt < 0.0f && Closure < 0.01f)
		{
			OpenAt = Blink.BlinkElapsed;
		}
	}
	AddInfo(FString::Printf(TEXT("Lid zu nach %.0f ms, wieder offen nach %.0f ms"), ClosedAt * 1000.0f, OpenAt * 1000.0f));
	TestTrue(TEXT("Das Lid schließt schneller, als es öffnet"), ClosedAt > 0.0f && ClosedAt < (OpenAt - ClosedAt));
	return true;
}

/**
 * En face: Sie holt das Kind nur vor ihr Gesicht, wenn es auf ihr liegt und sie sucht; der Abstand
 * ist der, auf den ein Neugeborenes scharf sieht. Das Lächeln kommt mit dem Blickkontakt, nicht mit der Zeit.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMotherEnFaceTest, "Genesis.People.Mother.EnFace", MotherFlags)

bool FGenesisMotherEnFaceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMotherLogic;
	const FGenesisMotherTuning Tuning;
	TestTrue(TEXT("Abstand Gesicht zu Gesicht 20–30 cm"), Tuning.EnFaceDistanceMm >= 200.0f && Tuning.EnFaceDistanceMm <= 300.0f);

	// Ohne Kind auf der Brust kein Heben, auch wenn es sucht
	FGenesisMotherInputs NotYet;
	NotYet.bChildSeeksFace = true;
	FGenesisMotherState State = Begin(5);
	Run(State, Tuning, NotYet, 5.0f);
	TestEqual(TEXT("In den Händen der Hebamme hebt sie nichts"), State.EnFace, 0.0f);

	// Auf der Brust, das Kind sucht ihr Gesicht: nach halber Zeit noch unterwegs, danach gegenüber
	FGenesisMotherInputs Seeking;
	Seeking.bChildOnChest = true;
	Seeking.bChildSeeksFace = true;
	Run(State, Tuning, Seeking, Tuning.EnFaceSeconds * 0.5f);
	TestFalse(TEXT("Nach halber Zeit noch nicht gegenüber"), IsFaceToFace(State));
	TestTrue(TEXT("Das Heben beginnt sanft"), GetEnFaceBlend(State) > 0.3f && GetEnFaceBlend(State) < 0.7f);
	Run(State, Tuning, Seeking, Tuning.EnFaceSeconds * 0.6f);
	TestTrue(TEXT("Dann Gesicht zu Gesicht"), IsFaceToFace(State));

	// Ohne Blickkontakt bleibt das Gesicht ruhig
	Run(State, Tuning, Seeking, 3.0f);
	TestTrue(TEXT("Kein Dauerlächeln"), State.Smile <= Tuning.RestingSmile + 0.01f);

	FGenesisMotherInputs Contact = Seeking;
	Contact.bChildLooksAtEyes = true;
	Run(State, Tuning, Contact, 1.5f);
	AddInfo(FString::Printf(TEXT("Lächeln nach 1,5 s Blickkontakt: %.2f"), State.Smile));
	TestTrue(TEXT("Beim Blickkontakt lächelt sie"), State.Smile > 0.4f);
	TestTrue(TEXT("Blickkontakt wird gezählt"), State.EyeContactSeconds > 1.4f);

	// Das Lächeln geht langsamer, als es kam
	Run(State, Tuning, Seeking, 1.0f);
	TestTrue(TEXT("Das Lächeln bleibt einen Moment"), State.Smile > 0.2f);

	// Zurück auf die Brust
	FGenesisMotherInputs Resting;
	Resting.bChildOnChest = true;
	Run(State, Tuning, Resting, Tuning.EnFaceSeconds + 0.1f);
	TestEqual(TEXT("Wieder auf der Brust"), State.EnFace, 0.0f);
	return true;
}

#endif
