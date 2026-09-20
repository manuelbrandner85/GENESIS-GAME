// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisFrontendLogic.h"
#include "GameFramework/InputSettings.h"

namespace GenesisFrontendTests
{
	static constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Jede Einstellung einmal in beide Richtungen bis an den Anschlag treiben. */
	int32 PushToLimit(FGenesisPlayerSettings& Settings, FName Id, int32 Direction, int32 MaxSteps = 200)
	{
		int32 Steps = 0;
		while (Steps < MaxSteps && GenesisFrontendLogic::Adjust(Settings, Id, Direction))
		{
			++Steps;
		}
		return Steps;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSettingsStayInRangeTest, "Genesis.Frontend.SettingsStayInRange", GenesisFrontendTests::Flags)
bool FGenesisSettingsStayInRangeTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFrontendTests;

	// Eine von Hand verstellte Einstellungsdatei darf das Spiel nicht in einen unmöglichen
	// Zustand bringen: alles weit außerhalb, danach muss alles gültig sein.
	FGenesisPlayerSettings Broken;
	Broken.ResolutionScalePercent = 4000;
	Broken.FrameRateLimit = 7;
	Broken.MasterVolume = 12.0f;
	Broken.MusicVolume = -3.0f;
	Broken.LookSensitivity = 99.0f;
	Broken.TextScalePercent = 3;
	GenesisFrontendLogic::Clamp(Broken);

	TestEqual(TEXT("Auflösungsskala begrenzt"), Broken.ResolutionScalePercent, 100);
	TestEqual(TEXT("Unbekannte Bildrate wird unbegrenzt"), Broken.FrameRateLimit, 0);
	TestEqual(TEXT("Lautstärke begrenzt"), Broken.MasterVolume, 1.0f);
	TestEqual(TEXT("Lautstärke nicht negativ"), Broken.MusicVolume, 0.0f);
	TestEqual(TEXT("Empfindlichkeit begrenzt"), Broken.LookSensitivity, 3.0f);
	TestEqual(TEXT("Schriftgröße begrenzt"), Broken.TextScalePercent, 80);

	// Und kein Regler lässt sich über seinen Anschlag hinaustreiben
	FGenesisPlayerSettings Settings;
	PushToLimit(Settings, TEXT("Aufloesungsskala"), -1);
	TestEqual(TEXT("Auflösungsskala unten bei 50 %"), Settings.ResolutionScalePercent, 50);
	PushToLimit(Settings, TEXT("Empfindlichkeit"), -1);
	TestEqual(TEXT("Empfindlichkeit unten bei 0,2"), Settings.LookSensitivity, 0.2f, 0.001f);
	PushToLimit(Settings, TEXT("Gesamt"), -1);
	TestEqual(TEXT("Gesamtlautstärke unten bei 0"), Settings.MasterVolume, 0.0f, 0.001f);
	PushToLimit(Settings, TEXT("Schriftgroesse"), 1);
	TestEqual(TEXT("Schriftgröße oben bei 200 %"), Settings.TextScalePercent, 200);
	PushToLimit(Settings, TEXT("Qualitaet"), 1);
	TestEqual(TEXT("Grafikstufe oben bei Ultra"), static_cast<int32>(Settings.Quality), 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEverySettingDoesSomethingTest, "Genesis.Frontend.EverySettingDoesSomething", GenesisFrontendTests::Flags)
bool FGenesisEverySettingDoesSomethingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFrontendTests;

	// Eine Einstellung, die nichts verändert, ist Dekoration. Jeder Eintrag muss sich verstellen
	// lassen **und** der angezeigte Wert muss danach ein anderer sein – sonst merkt der Spieler nicht,
	// dass er etwas getan hat.
	FGenesisPlayerSettings Settings;
	const TArray<FGenesisSettingEntry> Entries = GenesisFrontendLogic::BuildEntries(Settings);
	TestTrue(TEXT("Es gibt Einstellungen"), Entries.Num() >= 15);

	TSet<FName> SeenIds;
	for (const FGenesisSettingEntry& Entry : Entries)
	{
		TestFalse(FString::Printf(TEXT("Kennung doppelt: %s"), *Entry.Id.ToString()), SeenIds.Contains(Entry.Id));
		SeenIds.Add(Entry.Id);
		TestFalse(FString::Printf(TEXT("Beschriftung fehlt: %s"), *Entry.Id.ToString()), Entry.Label.IsEmpty());
		TestFalse(FString::Printf(TEXT("Abschnitt fehlt: %s"), *Entry.Id.ToString()), Entry.Section.IsEmpty());

		FGenesisPlayerSettings Copy = Settings;
		const bool bChanged = GenesisFrontendLogic::Adjust(Copy, Entry.Id, 1) || GenesisFrontendLogic::Adjust(Copy, Entry.Id, -1);
		TestTrue(FString::Printf(TEXT("%s lässt sich verstellen"), *Entry.Id.ToString()), bChanged);

		const TArray<FGenesisSettingEntry> After = GenesisFrontendLogic::BuildEntries(Copy);
		const FGenesisSettingEntry* Same = After.FindByPredicate([&Entry](const FGenesisSettingEntry& Candidate) { return Candidate.Id == Entry.Id; });
		if (TestNotNull(TEXT("Eintrag danach noch da"), Same))
		{
			TestNotEqual(FString::Printf(TEXT("Angezeigter Wert ändert sich: %s"), *Entry.Id.ToString()), Same->Value, Entry.Value);
		}
	}

	// Ein unbekannter Eintrag darf nichts anrichten
	FGenesisPlayerSettings Untouched = Settings;
	TestFalse(TEXT("Unbekannte Kennung ändert nichts"), GenesisFrontendLogic::Adjust(Untouched, TEXT("GibtEsNicht"), 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSettingsReachTheEngineTest, "Genesis.Frontend.SettingsReachTheEngine", GenesisFrontendTests::Flags)
bool FGenesisSettingsReachTheEngineTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFrontendTests;

	// Die Grafikstufe muss alle Skalierbarkeitsgruppen setzen – sonst bleibt ein Teil der Anzeige
	// auf dem alten Wert stehen und "Niedrig" ist nicht wirklich niedrig.
	FGenesisPlayerSettings Low;
	Low.Quality = EGenesisQuality::Niedrig;
	Low.bHardwareRayTracing = false;
	Low.ResolutionScalePercent = 70;
	Low.FrameRateLimit = 60;
	Low.bMotionBlur = false;
	const TArray<FString> LowCommands = GenesisFrontendLogic::RenderCommands(Low);

	const TCHAR* Groups[] = {
		TEXT("sg.ViewDistanceQuality"), TEXT("sg.AntiAliasingQuality"), TEXT("sg.ShadowQuality"),
		TEXT("sg.GlobalIlluminationQuality"), TEXT("sg.ReflectionQuality"), TEXT("sg.PostProcessQuality"),
		TEXT("sg.TextureQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.FoliageQuality"), TEXT("sg.ShadingQuality")
	};
	for (const TCHAR* Group : Groups)
	{
		const FString Expected = FString::Printf(TEXT("%s 0"), Group);
		TestTrue(FString::Printf(TEXT("%s wird gesetzt"), Group), LowCommands.Contains(Expected));
	}
	TestTrue(TEXT("Auflösungsskala wird gesetzt"), LowCommands.Contains(TEXT("r.ScreenPercentage 70")));
	TestTrue(TEXT("Bildratengrenze wird gesetzt"), LowCommands.Contains(TEXT("t.MaxFPS 60")));
	TestTrue(TEXT("Hardware-Strahlen aus"), LowCommands.Contains(TEXT("r.Lumen.HardwareRayTracing 0")));
	// Ohne Hardware-Strahlen braucht Lumen die Abstandsfelder, sonst fehlt die indirekte Beleuchtung ganz
	TestTrue(TEXT("Ersatz für Lumen ohne Strahlen an"), LowCommands.Contains(TEXT("r.Lumen.TraceMeshSDFs 1")));
	TestTrue(TEXT("Bewegungsunschärfe aus"), LowCommands.Contains(TEXT("r.MotionBlurQuality 0")));

	FGenesisPlayerSettings Ultra;
	Ultra.Quality = EGenesisQuality::Ultra;
	Ultra.bHardwareRayTracing = true;
	const TArray<FString> UltraCommands = GenesisFrontendLogic::RenderCommands(Ultra);
	TestTrue(TEXT("Ultra setzt Stufe 4"), UltraCommands.Contains(TEXT("sg.ShadowQuality 4")));
	TestTrue(TEXT("Hardware-Strahlen an"), UltraCommands.Contains(TEXT("r.Lumen.HardwareRayTracing 1")));
	TestTrue(TEXT("Unbegrenzte Bildrate ist 0"), UltraCommands.Contains(TEXT("t.MaxFPS 0")));

	// Blitze dämpfen muss sich auf das Bild auswirken, nicht nur in der Datei stehen
	FGenesisPlayerSettings Gentle = Ultra;
	Gentle.bReduceFlashing = true;
	TestNotEqual(TEXT("Blitze dämpfen verändert die Befehle"), GenesisFrontendLogic::RenderCommands(Gentle), UltraCommands);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMenuNavigationTest, "Genesis.Frontend.MenuNavigation", GenesisFrontendTests::Flags)
bool FGenesisMenuNavigationTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFrontendTests;

	// Ein Menü darf nicht in einer Ecke hängen bleiben: Vom letzten Eintrag geht es nach unten
	// zum ersten zurück. Wer nur einen Stick hat, kommt sonst nicht überall hin.
	TestEqual(TEXT("Unten läuft um"), GenesisFrontendLogic::Wrap(3, 1, 4), 0);
	TestEqual(TEXT("Oben läuft um"), GenesisFrontendLogic::Wrap(0, -1, 4), 3);
	TestEqual(TEXT("Leere Liste bleibt bei 0"), GenesisFrontendLogic::Wrap(0, 1, 0), 0);

	// Vor dem ersten Leben steht "Leben beginnen", danach "Weiterspielen" – nie beides
	const TArray<FString> Fresh = GenesisFrontendLogic::MainMenuEntries(false);
	TestTrue(TEXT("Startbildschirm bietet den Anfang an"), Fresh.Contains(TEXT("Leben beginnen")));
	TestFalse(TEXT("Startbildschirm bietet kein Weiterspielen"), Fresh.Contains(TEXT("Weiterspielen")));
	TestTrue(TEXT("Einstellungen erreichbar"), Fresh.Contains(TEXT("Einstellungen")));
	TestTrue(TEXT("Beenden erreichbar"), Fresh.Contains(TEXT("Beenden")));

	const TArray<FString> Running = GenesisFrontendLogic::MainMenuEntries(true);
	TestTrue(TEXT("Pausenmenü bietet Weiterspielen"), Running.Contains(TEXT("Weiterspielen")));
	TestTrue(TEXT("Pausenmenü bietet einen Neuanfang"), Running.Contains(TEXT("Von vorn beginnen")));
	TestFalse(TEXT("Pausenmenü bietet keinen zweiten Anfang"), Running.Contains(TEXT("Leben beginnen")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisEveryActionHasAGamepadKeyTest, "Genesis.Frontend.EveryActionHasAGamepadKey", GenesisFrontendTests::Flags)
bool FGenesisEveryActionHasAGamepadKeyTest::RunTest(const FString& Parameters)
{
	using namespace GenesisFrontendTests;

	// Der Game Director hat gefordert, dass sich das Spiel vollständig mit dem Controller
	// bedienen lässt. Eine Handlung, die nur auf der Tastatur liegt, macht genau das kaputt –
	// und sie fällt beim Spielen erst auf, wenn man an ihr hängen bleibt. Also prüfen wir es hier.
	const UInputSettings* Settings = GetDefault<UInputSettings>();
	if (!TestNotNull(TEXT("Eingabe-Einstellungen vorhanden"), Settings))
	{
		return false;
	}

	const TArray<FName> Actions = {
		TEXT("GenesisCry"), TEXT("GenesisRoot"),
		TEXT("GenesisMenu"), TEXT("GenesisAccept"), TEXT("GenesisBack"),
		TEXT("GenesisMenuUp"), TEXT("GenesisMenuDown"), TEXT("GenesisMenuLeft"), TEXT("GenesisMenuRight")
	};

	for (const FName& Action : Actions)
	{
		bool bKeyboard = false;
		bool bGamepad = false;
		for (const FInputActionKeyMapping& Mapping : Settings->GetActionMappings())
		{
			if (Mapping.ActionName != Action)
			{
				continue;
			}
			bGamepad |= Mapping.Key.IsGamepadKey();
			bKeyboard |= !Mapping.Key.IsGamepadKey();
		}
		TestTrue(FString::Printf(TEXT("%s liegt auf einer Taste"), *Action.ToString()), bKeyboard);
		TestTrue(FString::Printf(TEXT("%s liegt auf dem Controller"), *Action.ToString()), bGamepad);
	}

	// Und der Blick ebenso: ohne rechten Stick keine Kopfdrehung mit dem Controller
	const TArray<FName> Axes = { TEXT("GenesisLookRight"), TEXT("GenesisLookUp") };
	for (const FName& Axis : Axes)
	{
		bool bGamepad = false;
		bool bMouse = false;
		for (const FInputAxisKeyMapping& Mapping : Settings->GetAxisMappings())
		{
			if (Mapping.AxisName != Axis)
			{
				continue;
			}
			bGamepad |= Mapping.Key.IsGamepadKey();
			bMouse |= Mapping.Key.IsMouseButton() || Mapping.Key == EKeys::MouseX || Mapping.Key == EKeys::MouseY;
		}
		TestTrue(FString::Printf(TEXT("%s liegt auf der Maus"), *Axis.ToString()), bMouse);
		TestTrue(FString::Printf(TEXT("%s liegt auf dem Stick"), *Axis.ToString()), bGamepad);
	}

	return true;
}
