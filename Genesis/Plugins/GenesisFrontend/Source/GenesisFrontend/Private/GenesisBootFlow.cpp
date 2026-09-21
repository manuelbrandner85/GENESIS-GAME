// GENESIS: Der Kreislauf des Lebens

#include "GenesisBootFlow.h"

namespace
{
	/** Sanftes Ein-/Ausblenden über einen Zeitraum. */
	float Ramp(float Seconds, float From, float To)
	{
		if (To <= From)
		{
			return Seconds >= To ? 1.0f : 0.0f;
		}
		return FMath::SmoothStep(From, To, Seconds);
	}

	/** Wie lange der Hinweis „Nochmal drücken" stehen bleibt (s). */
	constexpr float SkipArmWindow = 3.0f;

	/** Ab wann in der Kapitelkarte der Level frisch geladen wird: Das Bild ist schwarz, der Text steht. */
	constexpr float StartLifeAt = 2.4f;
}

float GenesisBootFlow::StageDuration(EGenesisBootStage Stage)
{
	switch (Stage)
	{
	case EGenesisBootStage::Studio:  return 4.0f;
	case EGenesisBootStage::Engine:  return 3.2f;
	case EGenesisBootStage::Hinweis: return 6.5f;
	case EGenesisBootStage::Prolog:  return 52.0f;
	case EGenesisBootStage::Titel:   return 7.0f;
	case EGenesisBootStage::Kapitel: return 6.2f;
	case EGenesisBootStage::Ende:    return 10.0f;
	default:                         return -1.0f;
	}
}

const TArray<FGenesisPrologueBeat>& GenesisBootFlow::PrologueBeats()
{
	// Die Sätze stammen aus dem Trailer (Docs/Trailer/VO_Takes.json, jeweils Take A). Die Dauer ist
	// die gemessene Länge der Datei, damit sich zwei Sätze nie überlagern. Die Pausen dazwischen sind
	// gewollt: Der Prolog soll atmen, nicht erklären.
	static const TArray<FGenesisPrologueBeat> Beats = []()
	{
		auto Beat = [](float Start, float Duration, const TCHAR* Voice, const TCHAR* Text)
		{
			FGenesisPrologueBeat Result;
			Result.StartSeconds = Start;
			Result.DurationSeconds = Duration;
			Result.VoiceAsset = Voice;
			Result.Subtitle = Text;
			return Result;
		};
		return TArray<FGenesisPrologueBeat>{
			Beat(3.0f, 4.88f, TEXT("VO_Prolog_01a"), TEXT("Bevor du deinen ersten Atemzug nahmst …")),
			Beat(8.4f, 3.40f, TEXT("VO_Prolog_01b"), TEXT("… hatte deine Geschichte bereits begonnen.")),
			Beat(14.0f, 2.20f, TEXT("VO_Prolog_02"), TEXT("Du wirst lieben.")),
			Beat(17.2f, 2.12f, TEXT("VO_Prolog_03"), TEXT("Du wirst verlieren.")),
			Beat(21.4f, 3.28f, TEXT("VO_Prolog_04"), TEXT("Du wirst Entscheidungen treffen …")),
			Beat(25.0f, 5.16f, TEXT("VO_Prolog_05"), TEXT("… deren Folgen du vielleicht erst Jahrzehnte später verstehst.")),
			Beat(32.0f, 3.44f, TEXT("VO_Prolog_06"), TEXT("Doch kein Weg ist falsch.")),
			Beat(36.2f, 3.72f, TEXT("VO_Prolog_07"), TEXT("Jeder Weg hinterlässt Spuren.")),
			Beat(42.0f, 3.24f, TEXT("VO_Prolog_08"), TEXT("Am Ende bleiben nicht die Jahre.")),
			Beat(45.8f, 3.32f, TEXT("VO_Prolog_09"), TEXT("Es bleiben die Augenblicke."))
		};
	}();
	return Beats;
}

float GenesisBootFlow::PrologueCameraDistance(float Seconds)
{
	// Eine einzige, langsame Fahrt auf die Eizelle zu – aus dem offenen Eileiter bis vor die
	// Corona. Kein Schnitt: Der Prolog ist ein Atemzug, keine Montage.
	struct FKey { float Seconds; float DistanceUm; };
	// Sie endet dort, wo der ganze Komplex im Bild steht – Eizelle, Corona, Cumulus, das Gewimmel
	// davor. Näher heran füllt die Corona das Bild, und hinter dem Titel stünde nur noch Zellbrei.
	static const FKey Keys[] = { { 0.0f, 1500.0f }, { 20.0f, 1050.0f }, { 52.0f, 620.0f } };
	const int32 Count = UE_ARRAY_COUNT(Keys);
	if (Seconds <= Keys[0].Seconds)
	{
		return Keys[0].DistanceUm;
	}
	for (int32 Index = 1; Index < Count; ++Index)
	{
		if (Seconds <= Keys[Index].Seconds)
		{
			const float Alpha = FMath::SmoothStep(Keys[Index - 1].Seconds, Keys[Index].Seconds, Seconds);
			return FMath::Lerp(Keys[Index - 1].DistanceUm, Keys[Index].DistanceUm, Alpha);
		}
	}
	return Keys[Count - 1].DistanceUm;
}

float GenesisBootFlow::FadeAlpha(const FGenesisBootState& State)
{
	const float T = State.StageSeconds;
	switch (State.Stage)
	{
	case EGenesisBootStage::Studio:
	case EGenesisBootStage::Engine:
	case EGenesisBootStage::Hinweis:
	case EGenesisBootStage::Titel:
		return 1.0f;
	case EGenesisBootStage::Prolog:
	{
		const float Duration = StageDuration(EGenesisBootStage::Prolog);
		const float In = 1.0f - Ramp(T, 0.0f, 3.0f);
		const float Out = Ramp(T, Duration - 2.5f, Duration);
		return FMath::Max(In, Out);
	}
	case EGenesisBootStage::Taste:
		return 1.0f - Ramp(T, 0.0f, 2.0f);
	case EGenesisBootStage::Kapitel:
	{
		const float Duration = StageDuration(EGenesisBootStage::Kapitel);
		const float Out = Ramp(T, 0.0f, 1.2f);
		const float Back = Ramp(T, Duration - 1.2f, Duration);
		return FMath::Clamp(Out - Back, 0.0f, 1.0f);
	}
	case EGenesisBootStage::Ende:
		return Ramp(T, 0.0f, 2.0f);
	default:
		return 0.0f;
	}
}

FString GenesisBootFlow::CurrentSubtitle(const FGenesisBootState& State)
{
	if (State.Stage != EGenesisBootStage::Prolog)
	{
		return FString();
	}
	for (const FGenesisPrologueBeat& Beat : PrologueBeats())
	{
		// Etwas länger stehen lassen als gesprochen: Wer langsamer liest, soll den Satz zu Ende lesen können
		if (State.StageSeconds >= Beat.StartSeconds && State.StageSeconds < Beat.StartSeconds + Beat.DurationSeconds + 0.5f)
		{
			return Beat.Subtitle;
		}
	}
	return FString();
}

TArray<FString> GenesisBootFlow::CardLines(EGenesisBootStage Stage)
{
	switch (Stage)
	{
	case EGenesisBootStage::Studio:
		return { TEXT("GENESIS TEAM"), TEXT("präsentiert") };
	case EGenesisBootStage::Engine:
		return { TEXT("Entwickelt mit Unreal Engine 5") };
	case EGenesisBootStage::Hinweis:
		return {
			TEXT("Kopfhörer empfohlen."),
			TEXT("Dieses Spiel enthält Lichtwechsel und helle Blendungen."),
			TEXT("Unter Einstellungen → Barrierefreiheit lassen sie sich dämpfen.")
		};
	case EGenesisBootStage::Titel:
		return { TEXT("GENESIS"), TEXT("Der Kreislauf des Lebens") };
	case EGenesisBootStage::Kapitel:
		return { TEXT("Erstes Kapitel"), TEXT("Der Anfang"), TEXT("Eileiter, am Tag des Eisprungs") };
	case EGenesisBootStage::Ende:
		return {
			TEXT("Hier endet, was es von diesem Leben bisher zu erleben gibt."),
			TEXT("Danke, dass du dabei warst.")
		};
	default:
		return {};
	}
}

float GenesisBootFlow::CardAlpha(const FGenesisBootState& State)
{
	const float T = State.StageSeconds;
	const float Duration = StageDuration(State.Stage);
	switch (State.Stage)
	{
	case EGenesisBootStage::Studio:
	case EGenesisBootStage::Engine:
	case EGenesisBootStage::Hinweis:
	case EGenesisBootStage::Titel:
		return Ramp(T, 0.0f, 0.9f) * (1.0f - Ramp(T, Duration - 0.9f, Duration));
	case EGenesisBootStage::Kapitel:
		return Ramp(T, 1.2f, 2.0f) * (1.0f - Ramp(T, 4.2f, Duration - 1.2f));
	case EGenesisBootStage::Ende:
		return Ramp(T, 2.5f, 3.5f);
	default:
		return 0.0f;
	}
}

void GenesisBootFlow::EnterStage(FGenesisBootState& State, EGenesisBootStage Stage, TArray<FGenesisBootEvent>& OutEvents)
{
	State.Stage = Stage;
	State.StageSeconds = 0.0f;
	State.bSkipArmed = false;
	State.SkipArmedSeconds = 0.0f;
	if (Stage == EGenesisBootStage::Prolog)
	{
		State.NextBeat = 0;
	}
	if (Stage == EGenesisBootStage::Kapitel)
	{
		State.bLifeStarted = false;
	}

	FGenesisBootEvent Event;
	Event.Type = EGenesisBootEventType::StageEntered;
	Event.Stage = Stage;
	OutEvents.Add(Event);
}

void GenesisBootFlow::Start(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents)
{
	State = FGenesisBootState();
	EnterStage(State, EGenesisBootStage::Studio, OutEvents);
}

void GenesisBootFlow::Advance(FGenesisBootState& State, float DeltaSeconds, TArray<FGenesisBootEvent>& OutEvents)
{
	if (State.Stage == EGenesisBootStage::Aus)
	{
		return;
	}
	// Ein Ladevorgang darf die Karten nicht überspringen: Ein Bild, das drei Sekunden hängt, wird
	// hier wie eine Zehntelsekunde gezählt.
	State.StageSeconds += FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);

	if (State.bSkipArmed)
	{
		State.SkipArmedSeconds += FMath::Clamp(DeltaSeconds, 0.0f, 0.1f);
		if (State.SkipArmedSeconds > SkipArmWindow)
		{
			State.bSkipArmed = false;
		}
	}

	if (State.Stage == EGenesisBootStage::Prolog)
	{
		const TArray<FGenesisPrologueBeat>& Beats = PrologueBeats();
		while (Beats.IsValidIndex(State.NextBeat) && State.StageSeconds >= Beats[State.NextBeat].StartSeconds)
		{
			FGenesisBootEvent Event;
			Event.Type = EGenesisBootEventType::PlayVoice;
			Event.Stage = State.Stage;
			Event.VoiceAsset = Beats[State.NextBeat].VoiceAsset;
			OutEvents.Add(Event);
			++State.NextBeat;
		}
	}

	if (State.Stage == EGenesisBootStage::Kapitel && !State.bLifeStarted && State.StageSeconds >= StartLifeAt)
	{
		State.bLifeStarted = true;
		FGenesisBootEvent Event;
		Event.Type = EGenesisBootEventType::StartLife;
		Event.Stage = State.Stage;
		OutEvents.Add(Event);
	}

	const float Duration = StageDuration(State.Stage);
	if (Duration < 0.0f || State.StageSeconds < Duration)
	{
		return;
	}

	switch (State.Stage)
	{
	case EGenesisBootStage::Studio:  EnterStage(State, EGenesisBootStage::Engine, OutEvents); break;
	case EGenesisBootStage::Engine:  EnterStage(State, EGenesisBootStage::Hinweis, OutEvents); break;
	case EGenesisBootStage::Hinweis: EnterStage(State, EGenesisBootStage::Prolog, OutEvents); break;
	case EGenesisBootStage::Prolog:  EnterStage(State, EGenesisBootStage::Titel, OutEvents); break;
	case EGenesisBootStage::Titel:   EnterStage(State, EGenesisBootStage::Taste, OutEvents); break;
	case EGenesisBootStage::Kapitel: EnterStage(State, EGenesisBootStage::Spiel, OutEvents); break;
	case EGenesisBootStage::Ende:
	{
		FGenesisBootEvent Event;
		Event.Type = EGenesisBootEventType::ReturnToMenu;
		Event.Stage = State.Stage;
		OutEvents.Add(Event);
		EnterStage(State, EGenesisBootStage::Menue, OutEvents);
		break;
	}
	default:
		break;
	}
}

void GenesisBootFlow::Press(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents)
{
	switch (State.Stage)
	{
	case EGenesisBootStage::Studio:  EnterStage(State, EGenesisBootStage::Engine, OutEvents); break;
	case EGenesisBootStage::Engine:  EnterStage(State, EGenesisBootStage::Hinweis, OutEvents); break;
	case EGenesisBootStage::Hinweis: EnterStage(State, EGenesisBootStage::Prolog, OutEvents); break;
	case EGenesisBootStage::Prolog:
	case EGenesisBootStage::Titel:
		if (State.bSkipArmed)
		{
			EnterStage(State, EGenesisBootStage::Taste, OutEvents);
		}
		else
		{
			State.bSkipArmed = true;
			State.SkipArmedSeconds = 0.0f;
		}
		break;
	case EGenesisBootStage::Taste:
		// Nicht im allerersten Augenblick: Der Druck, der den Titel übersprungen hat, soll nicht
		// gleich auch noch das Menü öffnen.
		if (State.StageSeconds > 0.4f)
		{
			EnterStage(State, EGenesisBootStage::Menue, OutEvents);
		}
		break;
	default:
		break;
	}
}

void GenesisBootFlow::BeginLife(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents)
{
	EnterStage(State, EGenesisBootStage::Kapitel, OutEvents);
}

void GenesisBootFlow::EndLife(FGenesisBootState& State, TArray<FGenesisBootEvent>& OutEvents)
{
	if (State.Stage == EGenesisBootStage::Spiel)
	{
		EnterStage(State, EGenesisBootStage::Ende, OutEvents);
	}
}

bool GenesisBootFlow::AcceptsMenuInput(const FGenesisBootState& State)
{
	if (State.Stage == EGenesisBootStage::Menue)
	{
		return State.StageSeconds > 0.35f;
	}
	// Im Spiel (Pausenmenü) und ohne Ablauf (Editor) gilt das Menü sofort
	return State.Stage == EGenesisBootStage::Spiel || State.Stage == EGenesisBootStage::Aus;
}
