// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisLipSync.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisLipSyncPlaybackTest, "Genesis.People.LipSync.Playback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenesisLipSyncPlaybackTest::RunTest(const FString& Parameters)
{
	using namespace GenesisLipSyncLogic;
	// Weich ein und aus, dazwischen voll
	TestEqual(TEXT("Vor der Zeile still"), Envelope(-0.1f, 2.0f), 0.0f);
	TestEqual(TEXT("Am Anfang noch geschlossen"), Envelope(0.0f, 2.0f), 0.0f);
	TestEqual(TEXT("Mitten in der Zeile voll"), Envelope(1.0f, 2.0f), 1.0f);
	TestTrue(TEXT("Nach der Zeile still"), Envelope(2.1f, 2.0f) == 0.0f);

	// Lidschlag und Blick bleiben bei der Figur
	TestTrue(TEXT("Lidschlag gehört der Figur"), IsOwnedByCharacter(TEXT("CTRL_expressions_eyeBlinkL")));
	TestTrue(TEXT("Blick gehört der Figur"), IsOwnedByCharacter(TEXT("CTRL_expressions_eyeLookUpR")));
	TestFalse(TEXT("Der Kiefer gehört der Sprache"), IsOwnedByCharacter(TEXT("CTRL_expressions_jawOpen")));

	// Spur: 3 Bilder bei 50 fps, Kiefer auf – zu – auf; zwischen zwei Bildern linear
	UGenesisLipSyncTrack* Track = NewObject<UGenesisLipSyncTrack>();
	Track->NumFrames = 3;
	Track->CurveNames = { TEXT("CTRL_expressions_jawOpen"), TEXT("CTRL_expressions_mouthLipsPurseUL") };
	Track->Values = { 0.6f, 0.0f,   0.0f, 0.4f,   0.6f, 0.0f };
	TMap<FName, float> Curves;
	Track->Sample(0.01f, 1.0f, Curves);
	TestTrue(TEXT("Halb zwischen Bild 0 und 1"), FMath::IsNearlyEqual(Curves.FindRef(TEXT("CTRL_expressions_jawOpen")), 0.3f, 0.001f));
	TestTrue(TEXT("…auch die Lippen"), FMath::IsNearlyEqual(Curves.FindRef(TEXT("CTRL_expressions_mouthLipsPurseUL")), 0.2f, 0.001f));
	TestTrue(TEXT("Dauer = Bilder / 50"), FMath::IsNearlyEqual(Track->GetDuration(), 0.06f, 0.0001f));
	TestTrue(TEXT("Spitze des Kiefers"), FMath::IsNearlyEqual(Track->PeakOf(TEXT("CTRL_expressions_jawOpen")), 0.6f, 0.0001f));

	// Der Spieler: spricht, solange die Spur läuft, und verstummt danach von selbst
	FGenesisLipSyncPlayer Player;
	Player.Start(Track, 10.0);
	TestTrue(TEXT("Spricht"), Player.IsSpeaking(10.03));
	Player.Evaluate(10.03, Curves);
	TestTrue(TEXT("Kurven kommen an"), Curves.Num() == 2);
	Player.Evaluate(10.5, Curves);
	TestTrue(TEXT("Nach der Zeile keine Kurven mehr"), Curves.Num() == 0 && !Player.IsSpeaking(10.5));
	return true;
}

#endif
