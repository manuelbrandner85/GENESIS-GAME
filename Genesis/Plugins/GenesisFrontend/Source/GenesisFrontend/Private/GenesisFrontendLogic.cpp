// GENESIS: Der Kreislauf des Lebens

#include "GenesisFrontendLogic.h"

namespace
{
	/** Die Stufen der Bildrate. 0 steht am Ende für "unbegrenzt". */
	const TArray<int32>& FrameLimits()
	{
		static const TArray<int32> Limits = { 30, 60, 90, 120, 144, 165, 240, 0 };
		return Limits;
	}

	FString PercentText(int32 Value)
	{
		return FString::Printf(TEXT("%d %%"), Value);
	}

	FString VolumeText(float Value)
	{
		return FString::Printf(TEXT("%d %%"), FMath::RoundToInt(Value * 100.0f));
	}

	/** Einen Regler in Schritten verstellen und dabei in den Grenzen bleiben. */
	bool StepFloat(float& Value, float Step, int32 Direction, float Min, float Max)
	{
		const float Before = Value;
		Value = FMath::Clamp(FMath::RoundToFloat((Value + Step * Direction) / Step) * Step, Min, Max);
		return !FMath::IsNearlyEqual(Before, Value, 1e-4f);
	}

	bool StepInt(int32& Value, int32 Step, int32 Direction, int32 Min, int32 Max)
	{
		const int32 Before = Value;
		Value = FMath::Clamp(Value + Step * Direction, Min, Max);
		return Before != Value;
	}
}

void GenesisFrontendLogic::Clamp(FGenesisPlayerSettings& Settings)
{
	// Unter 50 % Auflösungsskala wird aus einem Bild ein Brei; über 100 % kostet es mehr, als es bringt.
	Settings.ResolutionScalePercent = FMath::Clamp(Settings.ResolutionScalePercent, 50, 100);
	if (!FrameLimits().Contains(Settings.FrameRateLimit))
	{
		Settings.FrameRateLimit = 0;
	}
	Settings.MasterVolume = FMath::Clamp(Settings.MasterVolume, 0.0f, 1.0f);
	Settings.VoiceVolume = FMath::Clamp(Settings.VoiceVolume, 0.0f, 1.0f);
	Settings.MusicVolume = FMath::Clamp(Settings.MusicVolume, 0.0f, 1.0f);
	Settings.WorldVolume = FMath::Clamp(Settings.WorldVolume, 0.0f, 1.0f);
	// Unter 0,2 bewegt sich der Kopf gar nicht mehr, über 3,0 ist er nicht mehr zu halten.
	Settings.LookSensitivity = FMath::Clamp(Settings.LookSensitivity, 0.2f, 3.0f);
	Settings.TextScalePercent = FMath::Clamp(Settings.TextScalePercent, 80, 200);
}

FString GenesisFrontendLogic::QualityName(EGenesisQuality Quality)
{
	switch (Quality)
	{
	case EGenesisQuality::Niedrig: return TEXT("Niedrig");
	case EGenesisQuality::Mittel:  return TEXT("Mittel");
	case EGenesisQuality::Hoch:    return TEXT("Hoch");
	case EGenesisQuality::Episch:  return TEXT("Episch");
	default:                       return TEXT("Ultra");
	}
}

FString GenesisFrontendLogic::WindowModeName(EGenesisWindowMode Mode)
{
	switch (Mode)
	{
	case EGenesisWindowMode::Vollbild:        return TEXT("Vollbild");
	case EGenesisWindowMode::FensterVollbild: return TEXT("Fenster (randlos)");
	default:                                  return TEXT("Fenster");
	}
}

TArray<FGenesisSettingEntry> GenesisFrontendLogic::BuildEntries(const FGenesisPlayerSettings& Settings)
{
	TArray<FGenesisSettingEntry> Entries;

	auto Add = [&Entries](const TCHAR* Section, const TCHAR* Id, const TCHAR* Label, const FString& Value, EGenesisSettingKind Kind, const TCHAR* Hint)
	{
		FGenesisSettingEntry Entry;
		Entry.Section = Section;
		Entry.Id = Id;
		Entry.Label = Label;
		Entry.Value = Value;
		Entry.Kind = Kind;
		Entry.Hint = Hint;
		Entries.Add(MoveTemp(Entry));
	};

	const FString FrameLimitText = Settings.FrameRateLimit == 0
		? FString(TEXT("unbegrenzt"))
		: FString::Printf(TEXT("%d fps"), Settings.FrameRateLimit);

	Add(TEXT("Bild"), TEXT("Fenstermodus"), TEXT("Fenstermodus"), WindowModeName(Settings.WindowMode), EGenesisSettingKind::Auswahl,
		TEXT("Randloses Fenster wechselt schneller, echtes Vollbild ist minimal schneller."));
	Add(TEXT("Bild"), TEXT("Aufloesungsskala"), TEXT("Auflösungsskala"), PercentText(Settings.ResolutionScalePercent), EGenesisSettingKind::Regler,
		TEXT("Der wirksamste Regler gegen Ruckeln: 80 % kosten kaum Schärfe, sparen aber ein Drittel Rechenzeit."));
	Add(TEXT("Bild"), TEXT("Bildrate"), TEXT("Bildrate begrenzen"), FrameLimitText, EGenesisSettingKind::Auswahl,
		TEXT("Eine feste Grenze läuft ruhiger als eine schwankende hohe Bildrate."));
	Add(TEXT("Bild"), TEXT("VSync"), TEXT("Bildsynchronisation"), Settings.bVSync ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter,
		TEXT("Verhindert zerrissene Bilder, fügt aber Verzögerung hinzu."));

	Add(TEXT("Grafik"), TEXT("Qualitaet"), TEXT("Grafikstufe"), QualityName(Settings.Quality), EGenesisSettingKind::Auswahl,
		TEXT("Stellt Schatten, Beleuchtung, Texturen und Effekte gemeinsam."));
	Add(TEXT("Grafik"), TEXT("Strahlen"), TEXT("Hardware-Strahlen (Lumen)"), Settings.bHardwareRayTracing ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter,
		TEXT("Echte Spiegelungen und Schatten. Ohne Strahlen läuft es deutlich schneller."));
	Add(TEXT("Grafik"), TEXT("Bewegungsunschaerfe"), TEXT("Bewegungsunschärfe"), Settings.bMotionBlur ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter,
		TEXT("Wirkt filmisch; wer davon Unwohlsein bekommt, schaltet sie ab."));

	Add(TEXT("Ton"), TEXT("Gesamt"), TEXT("Gesamtlautstärke"), VolumeText(Settings.MasterVolume), EGenesisSettingKind::Regler, TEXT(""));
	Add(TEXT("Ton"), TEXT("Stimmen"), TEXT("Stimmen"), VolumeText(Settings.VoiceVolume), EGenesisSettingKind::Regler,
		TEXT("Stimmen und Schreien – das Wichtigste in diesem Spiel."));
	Add(TEXT("Ton"), TEXT("Musik"), TEXT("Musik"), VolumeText(Settings.MusicVolume), EGenesisSettingKind::Regler, TEXT(""));
	Add(TEXT("Ton"), TEXT("Welt"), TEXT("Welt und Körper"), VolumeText(Settings.WorldVolume), EGenesisSettingKind::Regler,
		TEXT("Herzschlag, Atem, Raum."));

	Add(TEXT("Steuerung"), TEXT("Empfindlichkeit"), TEXT("Blickempfindlichkeit"), FString::Printf(TEXT("%.1f"), Settings.LookSensitivity), EGenesisSettingKind::Regler,
		TEXT("Gilt für Maus und für den rechten Stick."));
	Add(TEXT("Steuerung"), TEXT("InvertY"), TEXT("Y-Achse umkehren"), Settings.bInvertLookY ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter, TEXT(""));
	Add(TEXT("Steuerung"), TEXT("Vibration"), TEXT("Vibration"), Settings.bVibration ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter,
		TEXT("Herzschlag und Wehen werden im Controller spürbar."));

	Add(TEXT("Barrierefreiheit"), TEXT("Untertitel"), TEXT("Untertitel"), Settings.bSubtitles ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter, TEXT(""));
	Add(TEXT("Barrierefreiheit"), TEXT("Schriftgroesse"), TEXT("Schriftgröße"), PercentText(Settings.TextScalePercent), EGenesisSettingKind::Regler,
		TEXT("Vergrößert alle Texte und Hinweise im Bild."));
	Add(TEXT("Barrierefreiheit"), TEXT("Blitze"), TEXT("Blitze dämpfen"), Settings.bReduceFlashing ? TEXT("an") : TEXT("aus"), EGenesisSettingKind::Schalter,
		TEXT("Dämpft harte Helligkeitswechsel – etwa den ersten Moment nach der Geburt."));

	return Entries;
}

bool GenesisFrontendLogic::Adjust(FGenesisPlayerSettings& Settings, FName Id, int32 Direction)
{
	if (Direction == 0)
	{
		return false;
	}
	bool bChanged = false;

	if (Id == TEXT("Fenstermodus"))
	{
		const int32 Before = static_cast<int32>(Settings.WindowMode);
		Settings.WindowMode = static_cast<EGenesisWindowMode>(Wrap(Before, Direction, 3));
		bChanged = Before != static_cast<int32>(Settings.WindowMode);
	}
	else if (Id == TEXT("Aufloesungsskala"))
	{
		bChanged = StepInt(Settings.ResolutionScalePercent, 5, Direction, 50, 100);
	}
	else if (Id == TEXT("Bildrate"))
	{
		const int32 Before = FrameLimits().IndexOfByKey(Settings.FrameRateLimit);
		const int32 Index = Wrap(FMath::Max(Before, 0), Direction, FrameLimits().Num());
		bChanged = FrameLimits()[Index] != Settings.FrameRateLimit;
		Settings.FrameRateLimit = FrameLimits()[Index];
	}
	else if (Id == TEXT("VSync"))
	{
		Settings.bVSync = !Settings.bVSync;
		bChanged = true;
	}
	else if (Id == TEXT("Qualitaet"))
	{
		const int32 Before = static_cast<int32>(Settings.Quality);
		Settings.Quality = static_cast<EGenesisQuality>(FMath::Clamp(Before + Direction, 0, 4));
		bChanged = Before != static_cast<int32>(Settings.Quality);
	}
	else if (Id == TEXT("Strahlen"))
	{
		Settings.bHardwareRayTracing = !Settings.bHardwareRayTracing;
		bChanged = true;
	}
	else if (Id == TEXT("Bewegungsunschaerfe"))
	{
		Settings.bMotionBlur = !Settings.bMotionBlur;
		bChanged = true;
	}
	else if (Id == TEXT("Gesamt"))          { bChanged = StepFloat(Settings.MasterVolume, 0.05f, Direction, 0.0f, 1.0f); }
	else if (Id == TEXT("Stimmen"))         { bChanged = StepFloat(Settings.VoiceVolume, 0.05f, Direction, 0.0f, 1.0f); }
	else if (Id == TEXT("Musik"))           { bChanged = StepFloat(Settings.MusicVolume, 0.05f, Direction, 0.0f, 1.0f); }
	else if (Id == TEXT("Welt"))            { bChanged = StepFloat(Settings.WorldVolume, 0.05f, Direction, 0.0f, 1.0f); }
	else if (Id == TEXT("Empfindlichkeit")) { bChanged = StepFloat(Settings.LookSensitivity, 0.1f, Direction, 0.2f, 3.0f); }
	else if (Id == TEXT("InvertY"))         { Settings.bInvertLookY = !Settings.bInvertLookY; bChanged = true; }
	else if (Id == TEXT("Vibration"))       { Settings.bVibration = !Settings.bVibration; bChanged = true; }
	else if (Id == TEXT("Untertitel"))      { Settings.bSubtitles = !Settings.bSubtitles; bChanged = true; }
	else if (Id == TEXT("Schriftgroesse"))  { bChanged = StepInt(Settings.TextScalePercent, 10, Direction, 80, 200); }
	else if (Id == TEXT("Blitze"))          { Settings.bReduceFlashing = !Settings.bReduceFlashing; bChanged = true; }

	Clamp(Settings);
	return bChanged;
}

TArray<FString> GenesisFrontendLogic::RenderCommands(const FGenesisPlayerSettings& Settings)
{
	TArray<FString> Commands;
	const int32 Level = static_cast<int32>(Settings.Quality);

	// Die Skalierbarkeitsgruppen einzeln statt pauschal: Die Auflösung bekommt einen eigenen
	// Regler, sie darf nicht an der Grafikstufe hängen.
	Commands.Add(FString::Printf(TEXT("sg.ViewDistanceQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.AntiAliasingQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.ShadowQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.GlobalIlluminationQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.ReflectionQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.PostProcessQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.TextureQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.EffectsQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.FoliageQuality %d"), Level));
	Commands.Add(FString::Printf(TEXT("sg.ShadingQuality %d"), Level));

	Commands.Add(FString::Printf(TEXT("r.ScreenPercentage %d"), Settings.ResolutionScalePercent));
	Commands.Add(FString::Printf(TEXT("t.MaxFPS %d"), Settings.FrameRateLimit));
	Commands.Add(FString::Printf(TEXT("r.VSync %d"), Settings.bVSync ? 1 : 0));

	// Lumen ohne Hardware-Strahlen läuft weiter – dann über Abstandsfelder statt echter Strahlen.
	Commands.Add(FString::Printf(TEXT("r.Lumen.HardwareRayTracing %d"), Settings.bHardwareRayTracing ? 1 : 0));
	Commands.Add(FString::Printf(TEXT("r.Lumen.TraceMeshSDFs %d"), Settings.bHardwareRayTracing ? 0 : 1));
	Commands.Add(FString::Printf(TEXT("r.MotionBlurQuality %d"), Settings.bMotionBlur ? 4 : 0));

	// Blitze dämpfen: weicherer Bloom, damit ein harter Helligkeitswechsel nicht schlägt
	Commands.Add(FString::Printf(TEXT("r.BloomQuality %d"), Settings.bReduceFlashing ? 2 : 5));

	return Commands;
}

TArray<FString> GenesisFrontendLogic::MainMenuEntries(bool bRunActive)
{
	TArray<FString> Entries;
	if (bRunActive)
	{
		Entries.Add(TEXT("Weiterspielen"));
		Entries.Add(TEXT("Von vorn beginnen"));
	}
	else
	{
		Entries.Add(TEXT("Leben beginnen"));
	}
	Entries.Add(TEXT("Einstellungen"));
	Entries.Add(TEXT("Beenden"));
	return Entries;
}

int32 GenesisFrontendLogic::Wrap(int32 Index, int32 Delta, int32 Count)
{
	if (Count <= 0)
	{
		return 0;
	}
	return ((Index + Delta) % Count + Count) % Count;
}
