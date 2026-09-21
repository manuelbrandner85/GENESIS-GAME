// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisSceneSpeech.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSceneSpeechTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Lässt die Zeit laufen und sammelt, was gesagt wird (jede Zeile belegt die Szene 5 s). */
	TArray<FName> Run(TFunctionRef<void(GenesisSceneSpeechLogic::FInputs&, float)> Script, float Seconds, bool bGirl = true)
	{
		GenesisSceneSpeechLogic::FMemory Memory;
		TArray<FName> Said;
		float Busy = 0.0f;
		float BusyVocal = 0.0f;
		for (float T = 0.0f; T < Seconds; T += 0.1f)
		{
			Memory.Now = T;
			GenesisSceneSpeechLogic::FInputs In;
			In.bInBirthScene = true;
			In.bGirl = bGirl;
			Script(In, T);
			In.bCanSpeak = Busy <= 0.0f;
			In.bCanVocalize = BusyVocal <= 0.0f;
			const FName Line = GenesisSceneSpeechLogic::Choose(In, Memory);
			if (!Line.IsNone())
			{
				Said.Add(Line);
				(GenesisSceneSpeechLogic::IsVocalization(Line) ? BusyVocal : Busy) = 5.0f;
			}
			Busy -= 0.1f;
			BusyVocal -= 0.1f;
		}
		return Said;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSceneSpeechBirthTest, "Genesis.Slice.Speech.AfterBirth", GenesisSceneSpeechTests::Flags)
bool FGenesisSceneSpeechBirthTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSceneSpeechTests;
	auto Script = [](GenesisSceneSpeechLogic::FInputs& In, float T)
	{
		In.Stage = EGenesisLaborStage::Delivered;
		In.bNewborn = true;
		In.SecondsSinceBirth = T;
		In.bSkinToSkin = T > 20.0f;
		In.bEyeContact = T > 45.0f && T < 60.0f;
	};
	const TArray<FName> Girl = Run(Script, 90.0f, true);
	const TArray<FName> Boy = Run(Script, 90.0f, false);
	AddInfo(FString::Printf(TEXT("Mädchen: %s"), *FString::JoinBy(Girl, TEXT(", "), [](FName N) { return N.ToString(); })));

	TestTrue(TEXT("Zuerst ruft die Hebamme das Kind aus"), Girl.Num() > 0 && Girl[0] == FName(TEXT("D_H_Da_F")));
	TestTrue(TEXT("Beim Jungen die Jungen-Variante"), Boy.Num() > 0 && Boy[0] == FName(TEXT("D_H_Da_M")));
	TestTrue(TEXT("Dann die Mutter"), Girl.Num() > 1 && Girl[1] == FName(TEXT("D_M_Hallo")));
	TestTrue(TEXT("Haut an Haut spricht sie leise"), Girl.Contains(FName(TEXT("S_M_Warm"))));
	TestTrue(TEXT("Blickkontakt wird beantwortet"), Girl.Contains(FName(TEXT("E_M_Blick_01"))));
	TestFalse(TEXT("Nie Jungen-Zeilen bei einem Mädchen"), Girl.ContainsByPredicate([](FName N) { return N.ToString().EndsWith(TEXT("_M")); }));
	TestFalse(TEXT("Keine Wehen-Zeilen nach der Geburt"), Girl.ContainsByPredicate([](FName N) { return N.ToString().StartsWith(TEXT("P_")) || N.ToString().StartsWith(TEXT("L_")); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSceneSpeechLaborTest, "Genesis.Slice.Speech.Labor", GenesisSceneSpeechTests::Flags)
bool FGenesisSceneSpeechLaborTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSceneSpeechTests;
	// Presswehen alle 20 s, Kopf tritt nach 60 s durch, die Schulter hängt
	auto Script = [](GenesisSceneSpeechLogic::FInputs& In, float T)
	{
		In.Stage = EGenesisLaborStage::Pushing;
		In.Complication = EGenesisBirthComplication::ShoulderDystocia;
		In.ContractionIntensity = FMath::Fmod(T, 20.0f) < 9.0f ? 1.0f : 0.0f;
		In.Descent = FMath::Clamp(T / 80.0f, 0.0f, 1.0f);
	};
	const TArray<FName> Said = Run(Script, 100.0f);
	AddInfo(FString::Printf(TEXT("Austreibung: %s"), *FString::JoinBy(Said, TEXT(", "), [](FName N) { return N.ToString(); })));
	TestTrue(TEXT("Hebamme leitet das Schieben an"), Said.Contains(FName(TEXT("P_H_Schieben_01"))));
	TestTrue(TEXT("Beim Kopf: hecheln statt pressen"), Said.Contains(FName(TEXT("P_H_Kopf"))));
	TestTrue(TEXT("Schulterdystokie wird angesagt"), Said.Contains(FName(TEXT("C_H_Schulter"))));
	TestTrue(TEXT("Die Mutter presst, während die Hebamme spricht"), Said.Contains(FName(TEXT("P_M_Pressen"))));
	TestTrue(TEXT("Nach der Wehe: ausruhen"), Said.Contains(FName(TEXT("P_H_Pause"))));
	const int32 Head = Said.IndexOfByKey(FName(TEXT("P_H_Kopf")));
	const int32 Shoulder = Said.IndexOfByKey(FName(TEXT("C_H_Schulter")));
	TestTrue(TEXT("Erst der Kopf, dann die Schulter"), Head != INDEX_NONE && Shoulder > Head);

	// Vor der Geburt: keine Zeilen der ersten Stunde
	TestFalse(TEXT("Nichts aus der Zeit nach der Geburt"), Said.ContainsByPredicate([](FName N) { return N.ToString().StartsWith(TEXT("D_")) || N.ToString().StartsWith(TEXT("S_")); }));
	TestTrue(TEXT("Rolle aus der ID"), GenesisSceneSpeechLogic::IsMother(TEXT("D_M_Hallo")) && !GenesisSceneSpeechLogic::IsMother(TEXT("D_H_Da_M")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSceneSpeechChildTest, "Genesis.Slice.Speech.ChildSounds", GenesisSceneSpeechTests::Flags)
bool FGenesisSceneSpeechChildTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSceneSpeechLogic;
	float LastQuiet = -100.0f;
	bool bFirstCry = false;
	FInputs In;
	TestEqual(TEXT("Vor der Geburt schreit niemand"), ChooseChild(In, 0.0f, LastQuiet, bFirstCry, true), FName());

	In.bNewborn = true;
	In.SecondsSinceBirth = 0.6f;
	TestEqual(TEXT("Der erste Schrei nach einer halben Sekunde"), ChooseChild(In, 0.6f, LastQuiet, bFirstCry, true), FName(TEXT("SFX_Schrei_Erster")));
	TestEqual(TEXT("Nur einmal"), ChooseChild(In, 0.7f, LastQuiet, bFirstCry, true), FName());

	In.bCrying = true;
	In.CryLoudness = 0.9f;
	TestEqual(TEXT("Kalt und allein: kräftiges Schreien"), ChooseChild(In, 5.0f, LastQuiet, bFirstCry, true), FName(TEXT("SFX_Schrei_Stark")));
	In.CryLoudness = 0.4f;
	TestEqual(TEXT("Leichte Unruhe: Wimmern"), ChooseChild(In, 6.0f, LastQuiet, bFirstCry, true), FName(TEXT("SFX_Schrei_Wimmern")));
	TestEqual(TEXT("Solange ein Laut läuft, kein zweiter"), ChooseChild(In, 6.1f, LastQuiet, bFirstCry, false), FName());

	In.bCrying = false;
	In.CryLoudness = 0.0f;
	In.bSkinToSkin = true;
	TestEqual(TEXT("Ruhig auf der Haut: leise Laute"), ChooseChild(In, 30.0f, LastQuiet, bFirstCry, true), FName(TEXT("SFX_Baby_Laute")));
	TestEqual(TEXT("…aber selten"), ChooseChild(In, 35.0f, LastQuiet, bFirstCry, true), FName());
	In.bAsleep = true;
	TestEqual(TEXT("Schlafend: still"), ChooseChild(In, 80.0f, LastQuiet, bFirstCry, true), FName());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSceneSpeechHearingTest, "Genesis.Slice.Speech.NewbornHearing", GenesisSceneSpeechTests::Flags)
bool FGenesisSceneSpeechHearingTest::RunTest(const FString& Parameters)
{
	// Das Ohr des Neugeborenen wird in der ersten Stunde klarer – stetig, nie dumpfer
	const float AtBirth = AGenesisSceneSpeech::NewbornCutoff(0.0f, 3500.0f, 12000.0f);
	const float HalfHour = AGenesisSceneSpeech::NewbornCutoff(30.0f, 3500.0f, 12000.0f);
	const float Hour = AGenesisSceneSpeech::NewbornCutoff(60.0f, 3500.0f, 12000.0f);
	const float Later = AGenesisSceneSpeech::NewbornCutoff(180.0f, 3500.0f, 12000.0f);
	TestTrue(TEXT("Bei der Geburt gedämpft"), FMath::IsNearlyEqual(AtBirth, 3500.0f, 1.0f));
	TestTrue(TEXT("Nach einer halben Stunde dazwischen (logarithmisch: ~6500 Hz)"), HalfHour > 6000.0f && HalfHour < 7000.0f);
	TestTrue(TEXT("Nach einer Stunde klar"), FMath::IsNearlyEqual(Hour, 12000.0f, 1.0f));
	TestTrue(TEXT("Danach bleibt es so"), FMath::IsNearlyEqual(Later, Hour, 1.0f));
	return true;
}

#endif
