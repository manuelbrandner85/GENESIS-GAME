// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceHud.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisSliceDirector.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisBootFlow.h"
#include "GenesisSlicePlayerController.h"

AGenesisSliceHud::AGenesisSliceHud()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGenesisSliceHud::DrawLine(const FString& Text, float LineIndex, float Alpha)
{
	if (!Canvas || Alpha <= 0.01f)
	{
		return;
	}

	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Font)
	{
		return;
	}

	float Width = 0.0f;
	float Height = 0.0f;
	Canvas->TextSize(Font, Text, Width, Height);

	const float X = 0.5f * Canvas->SizeX - 0.5f * Width;
	const float Y = Canvas->SizeY - 110.0f + LineIndex * (Height + 6.0f);

	// Erst ein dunkler Schatten, dann der Text: Auf hellem Gewebe wäre weiße Schrift sonst weg
	FCanvasTextItem Shadow(FVector2D(X + 1.0f, Y + 1.0f), FText::FromString(Text), Font, FLinearColor(0.0f, 0.0f, 0.0f, 0.6f * Alpha));
	Canvas->DrawItem(Shadow);
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, FLinearColor(1.0f, 0.96f, 0.92f, Alpha));
	Canvas->DrawItem(Item);
}

void AGenesisSliceHud::DrawMenuTitle(const FString& Text, float Y, float Scale)
{
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (!Canvas || !Font)
	{
		return;
	}
	float Width = 0.0f;
	float Height = 0.0f;
	Canvas->TextSize(Font, Text, Width, Height, Scale, Scale);

	FCanvasTextItem Item(FVector2D(0.5f * Canvas->SizeX - 0.5f * Width, Y), FText::FromString(Text), Font, FLinearColor(1.0f, 0.94f, 0.88f, 0.95f));
	Item.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Item);
}

void AGenesisSliceHud::DrawMenuHint(const FString& Text, float Y, float Scale, float Alpha)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Canvas || !Font || Text.IsEmpty())
	{
		return;
	}
	float Width = 0.0f;
	float Height = 0.0f;
	Canvas->TextSize(Font, Text, Width, Height, Scale, Scale);

	FCanvasTextItem Item(FVector2D(0.5f * Canvas->SizeX - 0.5f * Width, Y), FText::FromString(Text), Font, FLinearColor(0.86f, 0.80f, 0.75f, Alpha));
	Item.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Item);
}

void AGenesisSliceHud::DrawMenuRow(const FString& Left, const FString& Right, float Y, bool bSelected, float Scale, bool bSection)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	if (!Canvas || !Font)
	{
		return;
	}

	// Auf der Hauptseite steht nur eine Spalte, und die gehört in die Mitte; auf der
	// Einstellungsseite stehen Name und Wert nebeneinander, sonst kann man sie nicht lesen.
	float ColumnLeft = 0.30f * Canvas->SizeX;
	const float ColumnRight = 0.68f * Canvas->SizeX;
	if (Right.IsEmpty() && !bSection)
	{
		float Width = 0.0f;
		float Height = 0.0f;
		Canvas->TextSize(GEngine->GetMediumFont(), Left, Width, Height, Scale, Scale);
		ColumnLeft = 0.5f * Canvas->SizeX - 0.5f * Width;
	}

	// Abschnittsüberschriften stehen matt und ohne Auswahlbalken – sie sind kein Eintrag
	const FLinearColor Colour = bSection
		? FLinearColor(0.72f, 0.62f, 0.56f, 0.85f)
		: (bSelected ? FLinearColor(1.0f, 0.97f, 0.92f, 1.0f) : FLinearColor(0.80f, 0.76f, 0.72f, 0.80f));

	if (bSelected)
	{
		// Ein schmaler Strich links statt eines Balkens: Er zeigt die Auswahl, ohne das Bild zuzudecken
		Canvas->K2_DrawBox(FVector2D(ColumnLeft - 20.0f * Scale, Y + 2.0f * Scale), FVector2D(4.0f * Scale, 18.0f * Scale), 4.0f * Scale, FLinearColor(0.95f, 0.75f, 0.62f, 0.95f));
	}

	FCanvasTextItem LeftItem(FVector2D(ColumnLeft, Y), FText::FromString(Left), Font, Colour);
	LeftItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(LeftItem);

	if (!Right.IsEmpty())
	{
		FCanvasTextItem RightItem(FVector2D(ColumnRight, Y), FText::FromString(Right), Font, Colour);
		RightItem.Scale = FVector2D(Scale, Scale);
		Canvas->DrawItem(RightItem);
	}
}

void AGenesisSliceHud::DrawBlack(float Alpha)
{
	if (!Canvas || Alpha <= 0.001f)
	{
		return;
	}
	FCanvasTileItem Black(FVector2D::ZeroVector, FVector2D(Canvas->SizeX, Canvas->SizeY), FLinearColor(0.0f, 0.0f, 0.0f, FMath::Clamp(Alpha, 0.0f, 1.0f)));
	Black.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Black);
}

void AGenesisSliceHud::DrawCentered(const FString& Text, float Y, float Scale, float Alpha, bool bLarge)
{
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	if (!Canvas || !Font || Text.IsEmpty() || Alpha <= 0.001f)
	{
		return;
	}
	float Width = 0.0f;
	float Height = 0.0f;
	Canvas->TextSize(Font, Text, Width, Height, Scale, Scale);
	const FVector2D Position(0.5f * Canvas->SizeX - 0.5f * Width, Y);

	// Ein weicher Schatten trägt die Schrift auch über hellem Gewebe
	FCanvasTextItem Shadow(Position + FVector2D(1.5f, 1.5f), FText::FromString(Text), Font, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f * Alpha));
	Shadow.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Shadow);
	FCanvasTextItem Item(Position, FText::FromString(Text), Font, FLinearColor(1.0f, 0.95f, 0.90f, Alpha));
	Item.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(Item);
}

bool AGenesisSliceHud::DrawBoot()
{
	UGameInstance* GameInstance = GetGameInstance();
	const UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	if (!Canvas || !Frontend)
	{
		return false;
	}

	const FGenesisBootState& Boot = Frontend->GetBootState();
	const float Scale = (Canvas->SizeY / 900.0f) * (Frontend->GetSettings().TextScalePercent / 100.0f);
	const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0f;
	const float CardAlpha = GenesisBootFlow::CardAlpha(Boot);
	const TArray<FString> Lines = GenesisBootFlow::CardLines(Boot.Stage);

	auto SkipPrompt = [&]()
	{
		if (Boot.bSkipArmed)
		{
			const FString Text = FString::Printf(TEXT("Nochmal drücken zum Überspringen"));
			UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
			if (Font)
			{
				float Width = 0.0f;
				float Height = 0.0f;
				Canvas->TextSize(Font, Text, Width, Height, Scale * 0.8f, Scale * 0.8f);
				FCanvasTextItem Item(FVector2D(Canvas->SizeX - Width - 48.0f * Scale, Canvas->SizeY - 60.0f * Scale), FText::FromString(Text), Font, FLinearColor(0.9f, 0.85f, 0.8f, 0.75f));
				Item.Scale = FVector2D(Scale * 0.8f, Scale * 0.8f);
				Canvas->DrawItem(Item);
			}
		}
	};

	switch (Boot.Stage)
	{
	case EGenesisBootStage::Studio:
	case EGenesisBootStage::Engine:
	{
		DrawBlack(1.0f);
		float Y = 0.44f * Canvas->SizeY;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			// Die erste Zeile trägt, die zweite begleitet
			const bool bMain = Index == 0;
			DrawCentered(Lines[Index], Y, Scale * (bMain ? 2.4f : 1.3f), CardAlpha * (bMain ? 1.0f : 0.75f), bMain);
			Y += (bMain ? 84.0f : 48.0f) * Scale;
		}
		return true;
	}

	case EGenesisBootStage::Hinweis:
	{
		DrawBlack(1.0f);
		float Y = 0.40f * Canvas->SizeY;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			DrawCentered(Lines[Index], Y, Scale * (Index == 0 ? 1.55f : 1.2f), CardAlpha * (Index == 0 ? 1.0f : 0.8f), false);
			Y += (Index == 0 ? 66.0f : 42.0f) * Scale;
		}
		return true;
	}

	case EGenesisBootStage::Prolog:
	{
		DrawBlack(GenesisBootFlow::FadeAlpha(Boot));
		// Untertitel wie im Kino: unten, ruhig, und nur, wenn der Spieler sie will
		if (Frontend->GetSettings().bSubtitles)
		{
			const FString Subtitle = GenesisBootFlow::CurrentSubtitle(Boot);
			DrawCentered(Subtitle, Canvas->SizeY - 150.0f * Scale, Scale * 1.5f, 0.95f, false);
		}
		SkipPrompt();
		return true;
	}

	case EGenesisBootStage::Titel:
	{
		DrawBlack(1.0f);
		if (Lines.Num() >= 2)
		{
			DrawCentered(Lines[0], 0.38f * Canvas->SizeY, Scale * 4.2f, CardAlpha, true);
			DrawCentered(Lines[1], 0.38f * Canvas->SizeY + 124.0f * Scale, Scale * 1.6f, CardAlpha * 0.85f, false);
		}
		SkipPrompt();
		return true;
	}

	case EGenesisBootStage::Taste:
	{
		// Der Eileiter hinter dem Titel ist das Bild des Startbildschirms – nur leicht abgedunkelt
		FCanvasTileItem Veil(FVector2D::ZeroVector, FVector2D(Canvas->SizeX, Canvas->SizeY), FLinearColor(0.02f, 0.01f, 0.01f, 0.45f));
		Veil.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Veil);

		const TArray<FString> Title = GenesisBootFlow::CardLines(EGenesisBootStage::Titel);
		const float TitleAlpha = FMath::Clamp(Boot.StageSeconds / 1.5f, 0.0f, 1.0f);
		if (Title.Num() >= 2)
		{
			DrawCentered(Title[0], 0.22f * Canvas->SizeY, Scale * 3.4f, TitleAlpha, true);
			DrawCentered(Title[1], 0.22f * Canvas->SizeY + 104.0f * Scale, Scale * 1.45f, TitleAlpha * 0.85f, false);
		}
		// Ein ruhiges Atmen statt eines Blinkens: 4 Sekunden je Zyklus, nie ganz weg
		const float Breath = 0.55f + 0.45f * FMath::Sin(Now * 2.0f * PI / 4.0f);
		const float PromptAlpha = FMath::Clamp((Boot.StageSeconds - 1.5f) / 1.0f, 0.0f, 1.0f) * Breath;
		DrawCentered(TEXT("Drücke eine beliebige Taste"), 0.74f * Canvas->SizeY, Scale * 1.45f, PromptAlpha, false);
		DrawBlack(GenesisBootFlow::FadeAlpha(Boot));
		return true;
	}

	case EGenesisBootStage::Kapitel:
	case EGenesisBootStage::Ende:
	{
		DrawBlack(GenesisBootFlow::FadeAlpha(Boot));
		float Y = (Boot.Stage == EGenesisBootStage::Kapitel ? 0.40f : 0.44f) * Canvas->SizeY;
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			// Kapitel: kleine Überschrift, großer Titel, leiser Ort
			float LineScale = 1.4f;
			bool bLarge = false;
			float Step = 54.0f;
			if (Boot.Stage == EGenesisBootStage::Kapitel)
			{
				LineScale = Index == 1 ? 3.0f : 1.25f;
				bLarge = Index == 1;
				Step = Index == 0 ? 56.0f : (Index == 1 ? 108.0f : 48.0f);
			}
			DrawCentered(Lines[Index], Y, Scale * LineScale, CardAlpha * (Index == 1 || Boot.Stage == EGenesisBootStage::Ende ? 1.0f : 0.75f), bLarge);
			Y += Step * Scale;
		}
		return true;
	}

	default:
		return false;
	}
}

bool AGenesisSliceHud::DrawMenu()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	if (!Canvas || !Frontend || Frontend->GetPage() == EGenesisMenuPage::Keine)
	{
		return false;
	}

	// Die Szene bleibt sichtbar, sie wird nur gedämpft: Das Bild hinter dem Menü ist das Spiel,
	// nicht ein Hintergrundbild. Deshalb ein Schleier statt einer Fläche.
	FCanvasTileItem Veil(FVector2D::ZeroVector, FVector2D(Canvas->SizeX, Canvas->SizeY), FLinearColor(0.015f, 0.008f, 0.008f, 0.86f));
	Veil.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Veil);

	// Die Schriftgröße folgt der Einstellung für Barrierefreiheit **und** der Bildhöhe
	const float Scale = (Canvas->SizeY / 900.0f) * (Frontend->GetSettings().TextScalePercent / 100.0f);
	const float RowHeight = 30.0f * Scale;
	float Y = 0.12f * Canvas->SizeY;

	if (Frontend->GetPage() == EGenesisMenuPage::Haupt)
	{
		DrawMenuTitle(TEXT("GENESIS"), Y, Scale * 3.4f);
		Y += 96.0f * Scale;
		DrawMenuTitle(TEXT("Der Kreislauf des Lebens"), Y, Scale * 1.1f);
		Y += 110.0f * Scale;

		const TArray<FString> Entries = Frontend->GetMainEntries();
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			DrawMenuRow(Entries[Index], FString(), Y, Index == Frontend->GetSelection(), Scale);
			Y += RowHeight;
		}
	}
	else
	{
		Y = 0.06f * Canvas->SizeY;
		DrawMenuTitle(TEXT("Einstellungen"), Y, Scale * 2.0f);
		Y += 64.0f * Scale;

		const TArray<FGenesisSettingEntry> Entries = Frontend->GetSettingEntries();

		// Die Liste muss in das Bild passen, auch wenn der Spieler die Schrift auf 200 % stellt.
		// Deshalb wird die Zeilenhöhe aus dem vorhandenen Platz berechnet, nicht fest gesetzt:
		// Eine Einstellung, die unter dem Bildrand verschwindet, gibt es nicht.
		int32 Sections = 0;
		FString CountedSection;
		for (const FGenesisSettingEntry& Entry : Entries)
		{
			if (Entry.Section != CountedSection) { CountedSection = Entry.Section; ++Sections; }
		}
		const float Available = (Canvas->SizeY - 140.0f * Scale) - Y;
		const float Rows = static_cast<float>(Entries.Num()) + 1.6f * Sections;
		const float Step = FMath::Min(RowHeight, Available / FMath::Max(Rows, 1.0f));

		FString LastSection;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			if (Entries[Index].Section != LastSection)
			{
				LastSection = Entries[Index].Section;
				Y += 0.6f * Step;
				DrawMenuRow(LastSection.ToUpper(), FString(), Y, false, Scale * 0.8f, true);
				Y += Step;
			}
			DrawMenuRow(Entries[Index].Label, Entries[Index].Value, Y, Index == Frontend->GetSelection(), Scale);
			Y += Step;
		}

		// Der Satz zur ausgewählten Einstellung steht unten – er erklärt, was sie kostet oder bringt
		if (Entries.IsValidIndex(Frontend->GetSelection()) && !Entries[Frontend->GetSelection()].Hint.IsEmpty())
		{
			DrawMenuHint(Entries[Frontend->GetSelection()].Hint, Canvas->SizeY - 118.0f * Scale, Scale * 0.85f, 0.8f);
		}
	}

	// Die Belegung steht unten und nennt beides: Taste und Controller-Taste. Ein Spiel, das nur
	// die Tastatur nennt, wirkt mit dem Controller in der Hand wie nicht dafür gemacht.
	const bool bSettings = Frontend->GetPage() == EGenesisMenuPage::Einstellungen;
	const FString Help = FString::Printf(TEXT("Hoch/Runter oder Steuerkreuz: wählen     %s     %s: zurück"),
		bSettings
			? *FString::Printf(TEXT("Links/Rechts oder %s: ändern"), *AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisAccept")))
			: *FString::Printf(TEXT("%s: bestätigen"), *AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisAccept"))),
		*AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisBack")));
	DrawMenuHint(Help, Canvas->SizeY - 70.0f * Scale, Scale * 0.9f, 0.7f);

	return true;
}

void AGenesisSliceHud::DrawHUD()
{
	Super::DrawHUD();

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance || !Canvas)
	{
		return;
	}

	// Der Startablauf hat das Bild für sich: Karten, Prolog, Titel, Kapitelkarte, Abspann.
	if (DrawBoot())
	{
		return;
	}

	// Das Menü liegt über allem und ersetzt die Hinweise: Wer im Menü ist, spielt gerade nicht.
	if (DrawMenu())
	{
		return;
	}

	const UGenesisSliceDirector* Director = GameInstance->GetSubsystem<UGenesisSliceDirector>();
	const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>();
	const AGenesisSlicePlayerController* Controller = Cast<AGenesisSlicePlayerController>(GetOwningPlayerController());
	if (!EarlyLife || !EarlyLife->HasNewborn() || !Controller)
	{
		return;
	}

	// Nur in der ersten Stunde: Davor gibt es für das Kind nichts zu entscheiden
	if (Director && Director->GetState().Phase != EGenesisSlicePhase::FirstHour
		&& Director->GetState().Phase != EGenesisSlicePhase::Idle)
	{
		return;
	}

	const FGenesisNewbornState& State = EarlyLife->GetState();
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (Controller->GetCryInput() > 0.2f && CryUsedSeconds < 0.0f)
	{
		CryUsedSeconds = Now;
	}
	if (Controller->GetRootInput() > 0.2f && RootUsedSeconds < 0.0f)
	{
		RootUsedSeconds = Now;
	}
	if (Controller->IsSeekingFace() && SeekFaceUsedSeconds < 0.0f)
	{
		SeekFaceUsedSeconds = Now;
	}

	auto FadeOf = [this, Now](float UsedSeconds)
	{
		if (UsedSeconds < 0.0f)
		{
			return 1.0f;
		}
		return FMath::Clamp(1.0f - (Now - UsedSeconds) / FMath::Max(0.1f, FadeAfterUseSeconds), 0.0f, 1.0f);
	};

	// Schreien geht immer. Suchen erst, wenn das Kind auf der Haut liegt – vorher wäre der Hinweis eine Lüge.
	DrawLine(AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisCry")) + TEXT(": rufen"), 0.0f, FadeOf(CryUsedSeconds));
	float Line = 1.0f;
	if (State.bSkinToSkin && !State.bHasFed)
	{
		DrawLine(AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisRoot")) + TEXT(": suchen"), Line, FadeOf(RootUsedSeconds));
		Line += 1.0f;
	}
	// Hinsehen: Auf der Haut der Mutter kann das Kind ihr Gesicht suchen – sie antwortet darauf
	if (State.bSkinToSkin)
	{
		DrawLine(TEXT("Blick nach oben halten: ihr Gesicht suchen"), Line, FadeOf(SeekFaceUsedSeconds));
	}
}
