// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisBootFlow.h"

namespace GenesisBootFlowTests
{
	static constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Den Ablauf in kleinen Schritten laufen lassen, bis eine Stufe erreicht ist oder die Zeit um ist. */
	float RunUntil(FGenesisBootState& State, EGenesisBootStage Target, float MaxSeconds, TArray<FGenesisBootEvent>& Events)
	{
		float Elapsed = 0.0f;
		while (State.Stage != Target && Elapsed < MaxSeconds)
		{
			GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
			Elapsed += 1.0f / 60.0f;
		}
		return Elapsed;
	}

	int32 Count(const TArray<FGenesisBootEvent>& Events, EGenesisBootEventType Type)
	{
		return Events.FilterByPredicate([Type](const FGenesisBootEvent& Event) { return Event.Type == Type; }).Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBootRunsLikeAGameTest, "Genesis.Frontend.Boot.RunsLikeAGame", GenesisBootFlowTests::Flags)
bool FGenesisBootRunsLikeAGameTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBootFlowTests;

	// Ohne einen einzigen Tastendruck: Logo, Engine, Hinweis, Prolog, Titel – und dann wartet das
	// Spiel. Es darf nicht von selbst ins Menü oder gar ins Spiel springen.
	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Start(State, Events);
	TestEqual(TEXT("Beginnt mit dem Studio"), State.Stage, EGenesisBootStage::Studio);

	const float ToTitleScreen = RunUntil(State, EGenesisBootStage::Taste, 200.0f, Events);
	AddInfo(FString::Printf(TEXT("Vom Start bis „Drücke eine beliebige Taste“: %.1f s"), ToTitleScreen));
	TestEqual(TEXT("Landet auf dem Titelbildschirm"), State.Stage, EGenesisBootStage::Taste);
	// Lang genug für eine Stimmung, kurz genug, dass niemand ungeduldig wird
	TestTrue(TEXT("Vorspann 55–90 s"), ToTitleScreen >= 55.0f && ToTitleScreen <= 90.0f);

	// Jeder Satz des Prologs wurde genau einmal gesprochen
	TestEqual(TEXT("Alle Sätze gesprochen"), Count(Events, EGenesisBootEventType::PlayVoice), GenesisBootFlow::PrologueBeats().Num());

	// Und dann wartet es – auch nach fünf Minuten
	for (int32 Step = 0; Step < 60 * 300; ++Step)
	{
		GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
	}
	TestEqual(TEXT("Wartet auf den Spieler"), State.Stage, EGenesisBootStage::Taste);
	TestEqual(TEXT("Kein Leben ohne Entscheidung"), Count(Events, EGenesisBootEventType::StartLife), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBootSkipTest, "Genesis.Frontend.Boot.Skipping", GenesisBootFlowTests::Flags)
bool FGenesisBootSkipTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBootFlowTests;

	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Start(State, Events);

	// Karten: ein Druck genügt
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Studio übersprungen"), State.Stage, EGenesisBootStage::Engine);
	GenesisBootFlow::Press(State, Events);
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Karten übersprungen, Prolog beginnt"), State.Stage, EGenesisBootStage::Prolog);

	// Prolog: Ein versehentlicher Druck darf die Erzählung nicht beenden
	GenesisBootFlow::Advance(State, 1.0f, Events);
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Erster Druck lässt den Prolog weiterlaufen"), State.Stage, EGenesisBootStage::Prolog);
	TestTrue(TEXT("… zeigt aber, wie man überspringt"), State.bSkipArmed);

	// Der Hinweis verfällt – wer nur einmal gedrückt hat, soll nicht Minuten später doch rausfliegen
	for (int32 Step = 0; Step < 60 * 4; ++Step)
	{
		GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
	}
	TestFalse(TEXT("Hinweis verfällt nach wenigen Sekunden"), State.bSkipArmed);
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Nach dem Verfall zählt der Druck wieder als erster"), State.Stage, EGenesisBootStage::Prolog);

	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Zweiter Druck überspringt"), State.Stage, EGenesisBootStage::Taste);

	// Derselbe Druck, der übersprungen hat, darf nicht gleich das Menü öffnen
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Kein Doppelsprung"), State.Stage, EGenesisBootStage::Taste);
	for (int32 Step = 0; Step < 36; ++Step)
	{
		GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
	}
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Taste öffnet das Menü"), State.Stage, EGenesisBootStage::Menue);

	// Und das Menü nimmt nicht im selben Augenblick Eingaben an
	TestFalse(TEXT("Menü sperrt im ersten Augenblick"), GenesisBootFlow::AcceptsMenuInput(State));
	GenesisBootFlow::Advance(State, 0.1f, Events);
	GenesisBootFlow::Advance(State, 0.1f, Events);
	GenesisBootFlow::Advance(State, 0.1f, Events);
	GenesisBootFlow::Advance(State, 0.1f, Events);
	TestTrue(TEXT("Menü nimmt danach Eingaben an"), GenesisBootFlow::AcceptsMenuInput(State));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBootLifeCycleTest, "Genesis.Frontend.Boot.LifeCycle", GenesisBootFlowTests::Flags)
bool FGenesisBootLifeCycleTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBootFlowTests;

	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Menue, Events);
	Events.Reset();

	// Leben beginnen: Der Level darf erst geladen werden, wenn das Bild ganz schwarz ist –
	// sonst sieht man, wie die Welt verschwindet und neu entsteht.
	GenesisBootFlow::BeginLife(State, Events);
	TestEqual(TEXT("Kapitelkarte"), State.Stage, EGenesisBootStage::Kapitel);
	bool bBlackAtStart = false;
	for (int32 Step = 0; Step < 60 * 10 && State.Stage == EGenesisBootStage::Kapitel; ++Step)
	{
		const int32 Before = Count(Events, EGenesisBootEventType::StartLife);
		GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
		if (Count(Events, EGenesisBootEventType::StartLife) > Before)
		{
			bBlackAtStart = GenesisBootFlow::FadeAlpha(State) >= 0.999f;
			TestTrue(TEXT("Kapitelkarte ist zu lesen, während geladen wird"), GenesisBootFlow::CardAlpha(State) > 0.5f);
		}
	}
	TestEqual(TEXT("Genau ein Lebensbeginn"), Count(Events, EGenesisBootEventType::StartLife), 1);
	TestTrue(TEXT("Geladen wird hinter Schwarz"), bBlackAtStart);
	TestEqual(TEXT("Danach läuft das Spiel"), State.Stage, EGenesisBootStage::Spiel);
	TestEqual(TEXT("Und das Bild ist wieder klar"), GenesisBootFlow::FadeAlpha(State), 0.0f);

	// Ende des spielbaren Abschnitts: Abspann, dann zurück ins Menü – nicht einfach Stillstand
	GenesisBootFlow::EndLife(State, Events);
	TestEqual(TEXT("Abspann"), State.Stage, EGenesisBootStage::Ende);
	RunUntil(State, EGenesisBootStage::Menue, 30.0f, Events);
	TestEqual(TEXT("Zurück im Menü"), State.Stage, EGenesisBootStage::Menue);
	TestEqual(TEXT("Der Menühintergrund wird frisch geladen"), Count(Events, EGenesisBootEventType::ReturnToMenu), 1);

	// Ein zweites Leben ist möglich
	Events.Reset();
	GenesisBootFlow::BeginLife(State, Events);
	RunUntil(State, EGenesisBootStage::Spiel, 30.0f, Events);
	TestEqual(TEXT("Zweites Leben beginnt ebenso"), Count(Events, EGenesisBootEventType::StartLife), 1);

	// Ein langer Ladevorgang darf die Karte nicht überspringen
	GenesisBootFlow::EndLife(State, Events);
	GenesisBootFlow::Advance(State, 8.0f, Events);
	TestEqual(TEXT("Ein hängendes Bild zählt nicht als acht Sekunden"), State.Stage, EGenesisBootStage::Ende);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisPrologueTest, "Genesis.Frontend.Boot.Prologue", GenesisBootFlowTests::Flags)
bool FGenesisPrologueTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBootFlowTests;

	// Kein Satz darf in den nächsten hineinreden, und jeder muss zu Ende sein, bevor das Bild
	// abblendet. Die Dauer ist die gemessene Länge der Aufnahme.
	const TArray<FGenesisPrologueBeat>& Beats = GenesisBootFlow::PrologueBeats();
	TestTrue(TEXT("Der Prolog hat Sätze"), Beats.Num() >= 6);
	const float PrologueLength = GenesisBootFlow::StageDuration(EGenesisBootStage::Prolog);
	for (int32 Index = 0; Index < Beats.Num(); ++Index)
	{
		const FGenesisPrologueBeat& Beat = Beats[Index];
		TestFalse(FString::Printf(TEXT("Satz %d hat einen Untertitel"), Index), Beat.Subtitle.IsEmpty());
		TestFalse(FString::Printf(TEXT("Satz %d hat eine Aufnahme"), Index), Beat.VoiceAsset.IsNone());
		TestTrue(FString::Printf(TEXT("Satz %d hat eine Länge"), Index), Beat.DurationSeconds > 0.5f);
		if (Beats.IsValidIndex(Index + 1))
		{
			TestTrue(FString::Printf(TEXT("Satz %d endet, bevor %d beginnt"), Index, Index + 1),
				Beat.StartSeconds + Beat.DurationSeconds + 0.3f <= Beats[Index + 1].StartSeconds);
		}
		TestTrue(FString::Printf(TEXT("Satz %d endet vor dem Abblenden"), Index),
			Beat.StartSeconds + Beat.DurationSeconds <= PrologueLength - 2.5f);
	}

	// Die Kamera fährt auf die Eizelle zu – nie zurück, nie durch sie hindurch
	float Previous = GenesisBootFlow::PrologueCameraDistance(0.0f);
	for (float Seconds = 0.5f; Seconds <= PrologueLength; Seconds += 0.5f)
	{
		const float Distance = GenesisBootFlow::PrologueCameraDistance(Seconds);
		TestTrue(TEXT("Die Fahrt geht nur nach vorn"), Distance <= Previous + 0.01f);
		Previous = Distance;
	}
	TestTrue(TEXT("Sie endet vor der Corona, nicht in ihr"), Previous >= 300.0f);

	// Untertitel erscheinen genau, wenn gesprochen wird
	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Prolog, Events);
	State.StageSeconds = Beats[0].StartSeconds + 0.5f;
	TestEqual(TEXT("Untertitel zum ersten Satz"), GenesisBootFlow::CurrentSubtitle(State), Beats[0].Subtitle);
	State.StageSeconds = 0.5f;
	TestTrue(TEXT("Vor dem ersten Satz kein Untertitel"), GenesisBootFlow::CurrentSubtitle(State).IsEmpty());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBootFilmTest, "Genesis.Frontend.Boot.Film", GenesisBootFlowTests::Flags)
bool FGenesisBootFilmTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBootFlowTests;

	// Mit Film: Logo, Engine, Hinweis – dann der Vorfilm statt des Prologs. Der Hinweis auf
	// Lichtwechsel steht vor dem Film, nicht dahinter: Der Film hat helle Blitze.
	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::Start(State, Events, true);
	const float ToFilm = RunUntil(State, EGenesisBootStage::Vorfilm, 60.0f, Events);
	AddInfo(FString::Printf(TEXT("Vom Start bis zum Vorfilm: %.1f s"), ToFilm));
	TestEqual(TEXT("Nach dem Hinweis kommt der Vorfilm"), State.Stage, EGenesisBootStage::Vorfilm);
	TestTrue(TEXT("Hinter dem Film ist es schwarz"), GenesisBootFlow::FadeAlpha(State) >= 0.999f);

	// Der Film spricht selbst: keine Erzählerstimme obendrauf
	const int32 VoicesBefore = Count(Events, EGenesisBootEventType::PlayVoice);
	GenesisBootFlow::Advance(State, 0.1f, Events);
	for (int32 Step = 0; Step < 60 * 60; ++Step)
	{
		GenesisBootFlow::Advance(State, 1.0f / 60.0f, Events);
	}
	TestEqual(TEXT("Kein Prologsatz unter dem Film"), Count(Events, EGenesisBootEventType::PlayVoice), VoicesBefore);

	// Meldet der Abspieler das Ende, geht es direkt zum Startbildschirm – nicht zur Titelkarte,
	// denn der Film endet schon auf dem Titel
	GenesisBootFlow::FilmFinished(State, Events);
	TestEqual(TEXT("Filmende führt zum Startbildschirm"), State.Stage, EGenesisBootStage::Taste);
	GenesisBootFlow::FilmFinished(State, Events);
	TestEqual(TEXT("Ein spätes Filmende ändert nichts mehr"), State.Stage, EGenesisBootStage::Taste);

	// Meldet er es nie (Datei kaputt, Treiber hängt), geht es nach Filmlänge trotzdem weiter
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Vorfilm, Events);
	const float Waited = RunUntil(State, EGenesisBootStage::Taste, 300.0f, Events);
	TestEqual(TEXT("Ohne Meldung trotzdem zum Startbildschirm"), State.Stage, EGenesisBootStage::Taste);
	TestTrue(TEXT("… aber erst nach dem Film"), Waited >= GenesisBootFlow::FilmLength() && Waited <= GenesisBootFlow::FilmLength() + 3.0f);

	// Überspringen wie beim Prolog: zwei Drücke, damit ein versehentlicher den Film nicht beendet
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Vorfilm, Events);
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Erster Druck lässt den Film laufen"), State.Stage, EGenesisBootStage::Vorfilm);
	TestTrue(TEXT("… und zeigt, wie man überspringt"), State.bSkipArmed);
	GenesisBootFlow::Press(State, Events);
	TestEqual(TEXT("Zweiter Druck überspringt zum Startbildschirm"), State.Stage, EGenesisBootStage::Taste);

	// Ohne Film bleibt alles wie vorher: Der Prolog erzählt
	FGenesisBootState Without;
	GenesisBootFlow::Start(Without, Events, false);
	RunUntil(Without, EGenesisBootStage::Prolog, 60.0f, Events);
	TestEqual(TEXT("Fehlt der Film, kommt der Prolog"), Without.Stage, EGenesisBootStage::Prolog);

	TestEqual(TEXT("Filmlänge = 2916 Bilder bei 24/s"), GenesisBootFlow::FilmLength(), 121.5f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBootFilmSubtitlesTest, "Genesis.Frontend.Boot.FilmSubtitles", GenesisBootFlowTests::Flags)
bool FGenesisBootFilmSubtitlesTest::RunTest(const FString& Parameters)
{
	// Die Untertitel sind an der Sprachspur gemessen. Keiner darf in den nächsten hineinreichen, und
	// alle liegen im Film. Die halbe Sekunde Nachlauf endet spätestens, wenn der nächste Satz beginnt.
	const TArray<FGenesisPrologueBeat>& Lines = GenesisBootFlow::FilmSubtitles();
	TestTrue(TEXT("Der Film hat Untertitel"), Lines.Num() >= 15);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		const FGenesisPrologueBeat& Line = Lines[Index];
		TestFalse(FString::Printf(TEXT("Zeile %d hat Text"), Index), Line.Subtitle.IsEmpty());
		TestTrue(FString::Printf(TEXT("Zeile %d ist lesbar lang (≥ 1 s)"), Index), Line.DurationSeconds >= 1.0f);
		TestTrue(FString::Printf(TEXT("Zeile %d liegt im Film"), Index), Line.StartSeconds >= 0.0f
			&& Line.StartSeconds + Line.DurationSeconds + 0.5f <= GenesisBootFlow::FilmLength());
		if (Lines.IsValidIndex(Index + 1))
		{
			TestTrue(FString::Printf(TEXT("Zeile %d endet, bevor %d beginnt"), Index, Index + 1),
				Line.StartSeconds + Line.DurationSeconds <= Lines[Index + 1].StartSeconds);
		}
	}

	FGenesisBootState State;
	TArray<FGenesisBootEvent> Events;
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Vorfilm, Events);

	// Zu Beginn jeder Zeile steht genau diese Zeile – kein Nachlauf der vorigen verdeckt sie
	for (const FGenesisPrologueBeat& Line : Lines)
	{
		GenesisBootFlow::SetFilmTime(State, Line.StartSeconds + 0.01f);
		TestEqual(TEXT("Zeile erscheint pünktlich"), GenesisBootFlow::CurrentSubtitle(State), Line.Subtitle);
	}

	// Die Stimme spricht schon über dem Schwarz des Anfangs
	GenesisBootFlow::SetFilmTime(State, 2.5f);
	TestEqual(TEXT("Erster Satz aus dem Dunkel"), GenesisBootFlow::CurrentSubtitle(State), FString(TEXT("Bevor du deinen ersten Atemzug nahmst …")));
	GenesisBootFlow::SetFilmTime(State, 11.0f);
	TestTrue(TEXT("In der Pause kein Untertitel"), GenesisBootFlow::CurrentSubtitle(State).IsEmpty());

	// Die Untertitel folgen dem Bild, nicht der Uhr des Ablaufs: Die Uhr sagt 5 s, der Film 30,5 s
	State.StageSeconds = 5.0f;
	GenesisBootFlow::SetFilmTime(State, 30.5f);
	TestEqual(TEXT("Folgt dem Film"), GenesisBootFlow::CurrentSubtitle(State), FString(TEXT("Hebamme: Da ist er!")));

	// Der letzte Satz des Erzählers steht als Schrift auf der Titelkarte – kein doppelter Text
	GenesisBootFlow::SetFilmTime(State, 115.0f);
	TestTrue(TEXT("Auf der Titelkarte kein Untertitel"), GenesisBootFlow::CurrentSubtitle(State).IsEmpty());
	GenesisBootFlow::SetFilmTime(State, 119.0f);
	TestEqual(TEXT("Das Kind am Schluss"), GenesisBootFlow::CurrentSubtitle(State), FString(TEXT("Kind: Kennen wir uns?")));

	// Außerhalb des Films wird keine Filmzeit angenommen
	GenesisBootFlow::EnterStage(State, EGenesisBootStage::Taste, Events);
	GenesisBootFlow::SetFilmTime(State, 30.5f);
	TestTrue(TEXT("Nach dem Film keine Filmuntertitel"), GenesisBootFlow::CurrentSubtitle(State).IsEmpty());
	return true;
}
