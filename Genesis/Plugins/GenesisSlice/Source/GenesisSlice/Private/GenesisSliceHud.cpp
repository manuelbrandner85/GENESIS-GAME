// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceHud.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisSliceDirector.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisBootFlow.h"
#include "GenesisSlicePlayerController.h"
#include "GenesisSpermSwarm.h"
#include "GenesisOocyte.h"
#include "GenesisMicroscopeCameraRig.h"
#include "GenesisEmbryoSubsystem.h"
#include "GenesisEmbryoLogic.h"
#include "GenesisEmbryogenesisLogic.h"
#include "GenesisFetalLogic.h"
#include "GenesisWombScene.h"
#include "GenesisSliceLogic.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

AGenesisSliceHud::AGenesisSliceHud()
{
	PrimaryActorTick.bCanEverTick = true;

	// Die Schrift des Covers: Cinzel, eine Kapitälchen-Antiqua nach römischen Inschriften (SIL Open Font License)
	static ConstructorHelpers::FObjectFinder<UFontFace> Cinzel(TEXT("/Game/Genesis/UI/Fonts/FF_GEN_Cinzel.FF_GEN_Cinzel"));
	TitleFontFace = Cinzel.Succeeded() ? Cinzel.Object : nullptr;
}

UFont* AGenesisSliceHud::GetTitleFont()
{
	// Ein Font-Asset lässt sich im Editor-Skript nicht anlegen (die Daten sind dort nicht zugänglich),
	// deshalb entsteht die Laufzeitschrift hier aus dem Schriftschnitt. Slate rastert sie in jeder Größe frisch.
	if (!TitleFont && TitleFontFace)
	{
		TitleFont = NewObject<UFont>(this);
		TitleFont->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry& Entry = TitleFont->CompositeFont.DefaultTypeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = TEXT("Default");
		Entry.Font = FFontData(TitleFontFace);
	}
	return TitleFont;
}

UTexture2D* AGenesisSliceHud::GetVeilGradient()
{
	if (VeilGradient)
	{
		return VeilGradient;
	}
	const int32 Height = 256;
	VeilGradient = UTexture2D::CreateTransient(1, Height, PF_B8G8R8A8);
	if (!VeilGradient)
	{
		return nullptr;
	}
	VeilGradient->SRGB = false;
	VeilGradient->AddressY = TA_Clamp;
	VeilGradient->Filter = TF_Bilinear;
	FTexture2DMipMap& Mip = VeilGradient->GetPlatformData()->Mips[0];
	uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Row = 0; Row < Height; ++Row)
	{
		// Oben voll, nach unten quadratisch auslaufend – so bleibt keine Kante stehen
		const float T = static_cast<float>(Row) / (Height - 1);
		const uint8 Alpha = static_cast<uint8>(FMath::Clamp(FMath::Square(1.0f - T) * 255.0f, 0.0f, 255.0f));
		Pixels[Row * 4 + 0] = 255;
		Pixels[Row * 4 + 1] = 255;
		Pixels[Row * 4 + 2] = 255;
		Pixels[Row * 4 + 3] = Alpha;
	}
	Mip.BulkData.Unlock();
	VeilGradient->UpdateResource();
	return VeilGradient;
}

void AGenesisSliceHud::DrawVeil(float FlatAlpha, float GradientAlpha)
{
	if (!Canvas)
	{
		return;
	}
	const FLinearColor Deep(0.006f, 0.008f, 0.013f, 1.0f);   // tiefes Blauschwarz wie der Raum auf dem Cover
	if (FlatAlpha > 0.001f)
	{
		FCanvasTileItem Flat(FVector2D::ZeroVector, FVector2D(Canvas->SizeX, Canvas->SizeY),
			FLinearColor(Deep.R, Deep.G, Deep.B, FlatAlpha));
		Flat.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Flat);
	}
	if (GradientAlpha > 0.001f)
	{
		if (UTexture2D* Gradient = GetVeilGradient())
		{
			if (FTextureResource* Resource = Gradient->GetResource())
			{
				FCanvasTileItem Band(FVector2D::ZeroVector, Resource, FVector2D(Canvas->SizeX, 0.58f * Canvas->SizeY),
					FVector2D(0.0f, 0.0f), FVector2D(1.0f, 1.0f), FLinearColor(0.004f, 0.006f, 0.010f, GradientAlpha));
				Band.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(Band);
			}
		}
	}
}

float AGenesisSliceHud::DrawCoverTitle(const FString& Text, float Y, float Scale, const FLinearColor& Colour, float Tracking, float Glow)
{
	UFont* Font = GetTitleFont();
	if (!Canvas || !Font || Text.IsEmpty())
	{
		return 0.0f;
	}

	// Zeichen für Zeichen, damit zwischen den Buchstaben Luft steht – wie auf dem Cover
	TArray<float> Widths;
	Widths.Reserve(Text.Len());
	float Total = 0.0f;
	float Height = 0.0f;
	for (int32 Index = 0; Index < Text.Len(); ++Index)
	{
		float Width = 0.0f;
		float Line = 0.0f;
		Canvas->TextSize(Font, FString::Chr(Text[Index]), Width, Line, Scale, Scale);
		Widths.Add(Width);
		Total += Width + (Index + 1 < Text.Len() ? Tracking * Scale : 0.0f);
		Height = FMath::Max(Height, Line);
	}

	auto Pass = [&](float OffsetX, float OffsetY, const FLinearColor& PassColour)
	{
		float X = 0.5f * Canvas->SizeX - 0.5f * Total + OffsetX;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			FCanvasTextItem Item(FVector2D(X, Y + OffsetY), FText::FromString(FString::Chr(Text[Index])), Font, PassColour);
			Item.Scale = FVector2D(Scale, Scale);
			Canvas->DrawItem(Item);
			X += Widths[Index] + Tracking * Scale;
		}
	};

	// Erst ein Schein (mehrere versetzte, sehr schwache Durchgänge), dann ein dunkler Grund, dann die Schrift.
	// Auf dem Cover leuchtet der Schriftzug von innen; ein harter Schlagschatten würde ihn aufkleben.
	if (Glow > 0.0f)
	{
		const float Radius = 2.5f * Scale;
		for (int32 Step = 0; Step < 8; ++Step)
		{
			const float Angle = 2.0f * PI * Step / 8.0f;
			Pass(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), FLinearColor(Colour.R, Colour.G * 0.8f, Colour.B * 0.5f, Glow));
		}
	}
	Pass(1.5f * Scale, 1.5f * Scale, FLinearColor(0.02f, 0.015f, 0.01f, 0.55f));
	Pass(0.0f, 0.0f, Colour);
	return Total;
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
		Canvas->K2_DrawBox(FVector2D(ColumnLeft - 20.0f * Scale, Y + 2.0f * Scale), FVector2D(4.0f * Scale, 18.0f * Scale), 4.0f * Scale, FLinearColor(0.79f, 0.57f, 0.26f, 0.95f));
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

	case EGenesisBootStage::Vorfilm:
	{
		DrawBlack(1.0f);

		// Der Film füllt das Bild so weit, wie sein Seitenverhältnis es erlaubt; der Rest bleibt schwarz.
		// Bis das erste Bild da ist, gilt 16:9 – die Untertitel stehen dann schon an ihrem Platz.
		UTexture* Film = Frontend->GetFilmTexture();
		const float FilmAspect = Film ? Film->GetSurfaceWidth() / FMath::Max(1.0f, Film->GetSurfaceHeight()) : 16.0f / 9.0f;
		const float CanvasAspect = Canvas->SizeX / FMath::Max(1.0f, static_cast<float>(Canvas->SizeY));
		const FVector2D Size = FilmAspect >= CanvasAspect
			? FVector2D(Canvas->SizeX, Canvas->SizeX / FilmAspect)
			: FVector2D(Canvas->SizeY * FilmAspect, Canvas->SizeY);
		const FVector2D Origin(0.5f * (Canvas->SizeX - Size.X), 0.5f * (Canvas->SizeY - Size.Y));
		if (Film && Film->GetResource())
		{
			FCanvasTileItem Tile(Origin, Film->GetResource(), Size, FLinearColor::White);
			Tile.BlendMode = SE_BLEND_Opaque;
			Canvas->DrawItem(Tile);
		}

		// Untertitel im unteren Kinobalken, den der Film selbst mitbringt (2,39:1 in 16:9 = 12,8 % der
		// Bildhöhe): So verdecken sie nichts vom Bild.
		if (Frontend->GetSettings().bSubtitles)
		{
			const FString Subtitle = GenesisBootFlow::CurrentSubtitle(Boot);
			UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
			if (Font && !Subtitle.IsEmpty())
			{
				// Rund 37 px bei 1080p: gut lesbar vom Sofa aus und immer noch klein genug für den Balken
				// (138 px bei 1080p). Die erste Fassung (1,35) ergab 24 px – am Bildschirm zu klein.
				const float TextScale = Scale * 2.2f;
				float Width = 0.0f;
				float Height = 0.0f;
				Canvas->TextSize(Font, Subtitle, Width, Height, TextScale, TextScale);
				const float Bar = Size.Y * 0.128f;
				DrawCentered(Subtitle, Origin.Y + Size.Y - 0.5f * Bar - 0.5f * Height, TextScale, 0.95f, false);
			}
		}
		SkipPrompt();
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
			// Wie auf dem Cover: Gold, weiter Buchstabenabstand, eine feine Linie, Untertitel in Kapitälchen
			const float Width = DrawCoverTitle(Lines[0].ToUpper(), 0.36f * Canvas->SizeY, Scale * 4.2f,
				FLinearColor(0.82f, 0.60f, 0.28f, CardAlpha), 22.0f, 0.07f * CardAlpha);
			const float LineY = 0.36f * Canvas->SizeY + 138.0f * Scale;
			Canvas->K2_DrawBox(FVector2D(0.5f * Canvas->SizeX - 0.46f * Width, LineY - 18.0f * Scale),
				FVector2D(0.92f * Width, FMath::Max(1.0f, 1.2f * Scale)), 1.0f, FLinearColor(0.70f, 0.55f, 0.32f, 0.55f * CardAlpha));
			DrawCoverTitle(Lines[1].ToUpper(), LineY, Scale * 1.15f, FLinearColor(0.84f, 0.78f, 0.70f, 0.9f * CardAlpha), 11.0f, 0.0f);
		}
		SkipPrompt();
		return true;
	}

	case EGenesisBootStage::Taste:
	{
		// Der Eileiter hinter dem Titel ist das Bild des Startbildschirms – nur leicht abgedunkelt
		// Derselbe Schleier wie im Menü: Über dem hellen Zellkranz stand der goldene Schriftzug sonst kraftlos da
		DrawVeil(0.62f, 0.6f);

		const TArray<FString> Title = GenesisBootFlow::CardLines(EGenesisBootStage::Titel);
		const float TitleAlpha = FMath::Clamp(Boot.StageSeconds / 1.5f, 0.0f, 1.0f);
		if (Title.Num() >= 2)
		{
			const float Width = DrawCoverTitle(Title[0].ToUpper(), 0.22f * Canvas->SizeY, Scale * 3.4f,
				FLinearColor(0.82f, 0.60f, 0.28f, TitleAlpha), 20.0f, 0.06f * TitleAlpha);
			const float LineY = 0.22f * Canvas->SizeY + 116.0f * Scale;
			Canvas->K2_DrawBox(FVector2D(0.5f * Canvas->SizeX - 0.46f * Width, LineY - 16.0f * Scale),
				FVector2D(0.92f * Width, FMath::Max(1.0f, 1.0f * Scale)), 1.0f, FLinearColor(0.70f, 0.55f, 0.32f, 0.5f * TitleAlpha));
			DrawCoverTitle(Title[1].ToUpper(), LineY, Scale * 1.0f, FLinearColor(0.84f, 0.78f, 0.70f, 0.85f * TitleAlpha), 10.0f, 0.0f);
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
	// Nach oben hin dunkler, damit die Schrift trägt. Als Verlauf, nicht als Fläche: Eine Fläche hinterließ eine
	// sichtbare Kante quer durchs Bild, einzelne Streifen feine Linien (beides gesehen).
	DrawVeil(0.72f, 0.62f);

	// Die Schriftgröße folgt der Einstellung für Barrierefreiheit **und** der Bildhöhe
	const float Scale = (Canvas->SizeY / 900.0f) * (Frontend->GetSettings().TextScalePercent / 100.0f);
	const float RowHeight = 30.0f * Scale;
	float Y = 0.12f * Canvas->SizeY;

	if (Frontend->GetPage() == EGenesisMenuPage::Haupt)
	{
		// Der Satz des Covers steht oben, weit gesetzt und zurückhaltend
		DrawCoverTitle(TEXT("JEDE ENTSCHEIDUNG HINTERLÄSST EIN ECHO"), 0.052f * Canvas->SizeY, Scale * 0.86f,
			FLinearColor(0.80f, 0.72f, 0.60f, 0.95f), 7.0f, 0.0f);

		// GENESIS in Gold (gemessen am Cover), mit Schein
		const float TitleWidth = DrawCoverTitle(TEXT("GENESIS"), Y, Scale * 3.8f, FLinearColor(0.82f, 0.60f, 0.28f, 1.0f), 20.0f, 0.07f);
		Y += 132.0f * Scale;

		// Eine feine Linie darunter, wie auf dem Cover, und der Untertitel in Kapitälchen
		if (TitleWidth > 0.0f)
		{
			const float LineWidth = TitleWidth * 0.92f;
			Canvas->K2_DrawBox(FVector2D(0.5f * Canvas->SizeX - 0.5f * LineWidth, Y - 18.0f * Scale),
				FVector2D(LineWidth, FMath::Max(1.0f, 1.2f * Scale)), 1.0f, FLinearColor(0.70f, 0.55f, 0.32f, 0.55f));
		}
		DrawCoverTitle(TEXT("DER KREISLAUF DES LEBENS"), Y, Scale * 1.05f, FLinearColor(0.84f, 0.78f, 0.70f, 0.92f), 10.0f, 0.0f);
		Y += 130.0f * Scale;

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
		DrawCoverTitle(TEXT("EINSTELLUNGEN"), Y, Scale * 1.4f, FLinearColor(0.79f, 0.57f, 0.26f, 0.95f), 12.0f, 0.0f);
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

bool AGenesisSliceHud::DrawRace(const UGenesisSliceDirector& Director)
{
	const AGenesisSpermSwarm* Swarm = nullptr;
	for (TActorIterator<AGenesisSpermSwarm> It(GetWorld()); It; ++It)
	{
		Swarm = *It;
		break;
	}
	const AGenesisSlicePlayerController* Controller = Cast<AGenesisSlicePlayerController>(GetOwningPlayerController());
	const UGenesisFrontendSubsystem* Frontend = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	if (!Swarm || !Swarm->IsRacing() || !Controller || !Frontend || !Canvas)
	{
		return false;
	}
	const float Scale = (Canvas->SizeY / 900.0f) * (Frontend->GetSettings().TextScalePercent / 100.0f);
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// Niederlage: groß, ruhig, ohne Häme
	if (Swarm->GetRaceOutcome() == EGenesisRaceOutcome::Lost)
	{
		// Viele kommen an, eine verschmilzt – nicht die schnellste (Docs/38)
		DrawCentered(TEXT("Eine andere Zelle ist verschmolzen."), 0.40f * Canvas->SizeY, Scale * 2.0f, 0.95f, true);
		DrawCentered(TEXT("Dieses Leben beginnt nicht. Noch einmal."), 0.40f * Canvas->SizeY + 70.0f * Scale, Scale * 1.3f, 0.8f, false);
		return true;
	}
	if (Swarm->GetRaceOutcome() == EGenesisRaceOutcome::Won)
	{
		DrawCentered(TEXT("Verschmolzen."), 0.40f * Canvas->SizeY, Scale * 2.0f, 0.95f, true);
		return true;
	}

	// Markierung wie in einer CASA-Software: ein feiner Kreis um den Kopf der verfolgten Zelle und ihre
	// Bahn der letzten zwei Sekunden. So zeigen Messgeräte für Spermienbewegung die Zelle, die sie verfolgen.
	// Nicht in der Ich-Perspektive: Dort ist man die Zelle selbst, und die Bahn läge hinter der Linse.
	bool bEgoView = false;
	for (TActorIterator<AGenesisMicroscopeCameraRig> It(GetWorld()); It; ++It)
	{
		bEgoView |= It->IsShowingEgoView();
	}
	if (!bEgoView)
	{
		const FVector Head = Swarm->GetCellHeadWorldPosition(Swarm->GetPlayerCellIndex());
		if (TrackSampleSeconds <= Now)
		{
			TrackSampleSeconds = Now + 0.1f;
			PlayerTrack.Add(Head);
			if (PlayerTrack.Num() > 20)
			{
				PlayerTrack.RemoveAt(0);
			}
		}
		const FLinearColor TrackColor(0.55f, 0.95f, 0.75f, 0.55f);
		for (int32 Index = 1; Index < PlayerTrack.Num(); ++Index)
		{
			// Am Ende des Kanalabschnitts springt die Zelle um (1 µm = 1 Einheit) – dort keine Linie quer durchs Bild
			if (FVector::Dist(PlayerTrack[Index - 1], PlayerTrack[Index]) > 40.0)
			{
				continue;
			}
			const FVector A = Project(PlayerTrack[Index - 1]);
			const FVector B = Project(PlayerTrack[Index]);
			if (A.Z > 0.0 && B.Z > 0.0)
			{
				FCanvasLineItem Line(FVector2D(A.X, A.Y), FVector2D(B.X, B.Y));
				Line.SetColor(TrackColor * FLinearColor(1.0f, 1.0f, 1.0f, static_cast<float>(Index) / PlayerTrack.Num()));
				Line.LineThickness = 1.0f;
				Canvas->DrawItem(Line);
			}
		}
		const FVector Center = Project(Head);
		if (Center.Z > 0.0)
		{
			const float Radius = 14.0f * Scale;
			constexpr int32 Segments = 24;
			for (int32 Segment = 0; Segment < Segments; ++Segment)
			{
				const float A0 = 2.0f * PI * Segment / Segments;
				const float A1 = 2.0f * PI * (Segment + 1) / Segments;
				FCanvasLineItem Arc(FVector2D(Center.X + Radius * FMath::Cos(A0), Center.Y + Radius * FMath::Sin(A0)),
					FVector2D(Center.X + Radius * FMath::Cos(A1), Center.Y + Radius * FMath::Sin(A1)));
				Arc.SetColor(TrackColor);
				Arc.LineThickness = 1.0f;
				Canvas->DrawItem(Arc);
			}
		}
	}

	// Zustand der eigenen Zelle – in der Sprache, in der ein Labor es notieren würde
	FString Status;
	switch (Swarm->GetPlayerPhase())
	{
	case EGenesisSpermPhase::Bound:
		Status = TEXT("An der Zona gebunden – Akrosomreaktion");
		break;
	case EGenesisSpermPhase::Penetrating:
		Status = FString::Printf(TEXT("In der Zona: %.1f von %.0f µm"), Swarm->GetPlayerPenetrationUm(), Swarm->GetZonaThicknessUm());
		break;
	case EGenesisSpermPhase::Perivitelline:
		// Die Membranen müssen sich finden (Izumo1 an Juno) – das dauert Minuten, und es liegt nicht mehr in der Hand
		Status = Swarm->GetOocyte() && Swarm->GetOocyte()->GetState().IsFertilized()
			? TEXT("Im Spalt unter der Zona – eine andere Zelle ist verschmolzen")
			: TEXT("Im Spalt unter der Zona – die Membranen suchen einander");
		break;
	case EGenesisSpermPhase::Blocked:
		Status = TEXT("Abgewiesen – die Zona hat sich verändert");
		break;
	default:
		// Keine Platzierung: Es ist kein Wettlauf um Tempo (Docs/38)
		Status = FString::Printf(TEXT("Abstand zur Eizelle %.0f µm · %s"),
			Swarm->GetPlayerDistanceToZonaUm(), Swarm->IsPlayerHyperactivated() ? TEXT("hyperaktiviert") : TEXT("progressiv"));
		break;
	}
	if (Director.GetRaceAttempts() > 0)
	{
		Status += FString::Printf(TEXT(" · Versuch %d"), Director.GetRaceAttempts() + 1);
	}
	DrawCentered(Status, Canvas->SizeY - 110.0f * Scale, Scale * 1.15f, 0.85f, false);

	// Zeitraffer mit Uhr, wie der Zeitstempel oben auf einer Zeitrafferaufnahme am Mikroskop: Die Zeit an
	// der Eizelle wird sichtbar gerafft statt heimlich (Docs/38) – eine halbe Stunde Biologie in gut vierzig
	// Sekunden. Oben, weil unten Zustand und Tastenhinweise stehen (dort lagen sie zuerst übereinander).
	if (Swarm->IsTimeLapse())
	{
		const FGenesisOocyteState* Egg = Swarm->GetOocyte() ? &Swarm->GetOocyte()->GetState() : nullptr;
		const bool bAfterFusion = Egg && Egg->IsFertilized();
		const float Clock = bAfterFusion ? Egg->SecondsSinceFusion : Swarm->GetPlayerSecondsAtEgg();
		if (Clock >= 0.0f)
		{
			const int32 Whole = FMath::FloorToInt(Clock);
			FString Line = FString::Printf(TEXT("Zeitraffer ×%.0f · %d:%02d min %s"), Swarm->GetEffectiveTimeScale(),
				Whole / 60, Whole % 60, bAfterFusion ? TEXT("nach der Verschmelzung") : TEXT("an der Eizelle"));
			if (Egg && Egg->PerivitellineCells > 0)
			{
				Line += FString::Printf(TEXT(" · im Spalt %d"), Egg->PerivitellineCells);
			}
			DrawCentered(Line, 48.0f * Scale, Scale * 1.0f, 0.75f, false);
		}
	}

	// Kraft in der Zona: ein schlichter Balken, nur solange sie zählt
	if (Swarm->GetPlayerPhase() == EGenesisSpermPhase::Penetrating || Swarm->GetPlayerPhase() == EGenesisSpermPhase::Bound)
	{
		const float Width = 260.0f * Scale;
		const float Height = 6.0f * Scale;
		const float X = 0.5f * (Canvas->SizeX - Width);
		const float Y = Canvas->SizeY - 80.0f * Scale;
		FCanvasTileItem Back(FVector2D(X, Y), FVector2D(Width, Height), FLinearColor(1.0f, 1.0f, 1.0f, 0.15f));
		Back.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Back);
		FCanvasTileItem Fill(FVector2D(X, Y), FVector2D(Width * Swarm->GetPlayerVigor(), Height), FLinearColor(1.0f, 0.93f, 0.85f, 0.8f));
		Fill.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Fill);
	}

	// Hinweise zur Steuerung – sie gehen, sobald man sie benutzt hat
	const float SteerAlpha = Controller->HasSteered() ? FMath::Clamp(1.0f - (Now - SteerHintUsedSeconds) / 4.0f, 0.0f, 1.0f) : 1.0f;
	if (Controller->HasSteered() && SteerHintUsedSeconds < 0.0f)
	{
		SteerHintUsedSeconds = Now;
	}
	// Lenken nur, solange die Zelle schwimmt – an und in der Eizelle gibt es nichts zu lenken
	if (Swarm->GetPlayerPhase() == EGenesisSpermPhase::Swimming)
	{
		DrawLine(TEXT("W A S D / linker Stick: lenken"), 0.0f, SteerAlpha);
	}
	if (Swarm->GetPlayerPhase() == EGenesisSpermPhase::Bound || Swarm->GetPlayerPhase() == EGenesisSpermPhase::Penetrating)
	{
		DrawLine(AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisCry")) + TEXT(" schnell drücken: schlagen – die Kraft bringt dich durch die Zona"), 1.0f, 1.0f);
	}
	else if (Swarm->GetPlayerPhase() == EGenesisSpermPhase::Swimming)
	{
		DrawLine(TEXT("Maus / rechter Stick: umsehen"), 1.0f, Controller->HasMovedMicroscope() ? 0.0f : 0.7f);
		DrawLine(AGenesisSlicePlayerController::DescribeAction(TEXT("GenesisToggleView")) + TEXT(": Ansicht wechseln"), -1.0f, Controller->HasToggledView() ? 0.0f : 0.6f);
	}
	return true;
}

void AGenesisSliceHud::DrawWomb(float Scale, const FGenesisGestationPlanPoint& Plan, const TArray<FGenesisGestationMoment>& Moments, bool bInRun)
{
	// Was das Kind gerade spürt, schmeckt, erlebt – und was der Spieler in dieser Woche tun kann (Docs/37)
	for (TActorIterator<AGenesisWombScene> It(GetWorld()); It; ++It)
	{
		float CaptionAlpha = 0.0f;
		const FString Line = It->GetCaption(CaptionAlpha);
		DrawCentered(Line, 0.70f * Canvas->SizeY, Scale * 1.0f, 0.85f * CaptionAlpha, false);
		if (Moments.IsValidIndex(Plan.MomentIndex) || !bInRun)
		{
			UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
			float Y = 0.30f * Canvas->SizeY;
			for (const FGenesisWombHint& Hint : It->GetHints())
			{
				// Was schon getan wurde, tritt zurück – der Hinweis soll nicht stören, sobald man es kann
				const float Alpha = Hint.bUsed ? 0.18f : 0.6f;
				if (Font)
				{
					FCanvasTextItem Shadow(FVector2D(41.0f * Scale, Y + 1.0f), FText::FromString(Hint.Verb), Font, FLinearColor(0.0f, 0.0f, 0.0f, 0.5f * Alpha));
					Shadow.Scale = FVector2D(Scale * 0.8f, Scale * 0.8f);
					Canvas->DrawItem(Shadow);
					FCanvasTextItem Item(FVector2D(40.0f * Scale, Y), FText::FromString(Hint.Verb), Font, FLinearColor(1.0f, 0.93f, 0.86f, Alpha));
					Item.Scale = FVector2D(Scale * 0.8f, Scale * 0.8f);
					Canvas->DrawItem(Item);
				}
				Y += 20.0f * Scale;
			}
		}
	}
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

	// Das Wettrennen (GENESIS-037): nüchterne Messwerte wie am Mikroskop, keine Spielgrafik
	if (Director && Director->GetState().Phase == EGenesisSlicePhase::Conception && DrawRace(*Director))
	{
		return;
	}

	// Die erste Woche: Beschriftung wie in einem Zeitraffer aus dem Brutschrank (EmbryoScope)
	if (Director && Director->GetState().Phase == EGenesisSlicePhase::Embryo)
	{
		const UGenesisEmbryoSubsystem* Embryo = GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>();
		const UGenesisFrontendSubsystem* Frontend = GameInstance->GetSubsystem<UGenesisFrontendSubsystem>();
		if (Embryo && Embryo->HasEmbryo() && Frontend)
		{
			const FGenesisEmbryoState& Keim = Embryo->GetState();
			const float Scale = (Canvas->SizeY / 900.0f) * (Frontend->GetSettings().TextScalePercent / 100.0f);
			// Die zweite Woche (GENESIS-040): Ab der Einnistung zählen Größe und hCG, nicht mehr die Zellen
			const bool bSecondWeek = Keim.Stage >= EGenesisEmbryoStage::Implanting && Keim.Stage != EGenesisEmbryoStage::Arrested;
			const FGenesisImplantationState& Nid = Keim.Nidation;
			const FGenesisEmbryogenesisState& Plan = Keim.Embryogenesis;
			// Ab der dritten Woche zählen Länge, Somiten und der Herzschlag (GENESIS-041)
			const bool bBodyPlan = Plan.Stage != EGenesisEmbryogenesisStage::None;
			const FString Clock = bBodyPlan
				? FString::Printf(TEXT("Tag %d · %.1f mm%s%s"),
					static_cast<int32>(Keim.HoursSinceFusion / 24.0) + 1, Plan.LengthMm,
					Plan.Somites > 0 ? *FString::Printf(TEXT(" · %d Somitenpaare"), Plan.Somites) : TEXT(""),
					Plan.bHeartBeating ? *FString::Printf(TEXT(" · Herz %.0f/min"), Plan.HeartRateBpm) : TEXT(""))
				: bSecondWeek
				? FString::Printf(TEXT("%.0f h seit Verschmelzung · Tag %d · Keim %.2f mm%s"),
					Keim.HoursSinceFusion, static_cast<int32>(Keim.HoursSinceFusion / 24.0) + 1, Nid.ConceptusDiameterUm / 1000.0f,
					Nid.HcgMilliIU >= 1.0f ? *FString::Printf(TEXT(" · hCG %.0f mIU/ml"), Nid.HcgMilliIU) : TEXT(""))
				: FString::Printf(TEXT("%.1f h seit Verschmelzung · Tag %d · %d %s"),
					Keim.HoursSinceFusion, static_cast<int32>(Keim.HoursSinceFusion / 24.0) + 1,
					Keim.GetCellCount(), Keim.GetCellCount() == 1 ? TEXT("Zelle") : TEXT("Zellen"));
			DrawCentered(Clock, Canvas->SizeY - 118.0f * Scale, Scale * 1.15f, 0.85f, false);
			const FString Line = bBodyPlan ? GenesisEmbryogenesisLogic::DescribeStage(Plan.Stage)
				: (bSecondWeek ? GenesisEmbryoLogic::DescribeImplantation(Nid.Phase) : GenesisSliceLogic::DescribeEmbryoStage(Keim.Stage));
			DrawCentered(Line, Canvas->SizeY - 88.0f * Scale, Scale * 0.95f, 0.7f, false);
		}
		return;
	}

	// Die Schwangerschaft (GENESIS-044): Kapitelzeile beim Moment, darunter nüchtern Woche, Länge, Gewicht.
	// Ohne Durchlauf (Prüfansicht L_GEN_Mutterleib) nur Sinneszeile und Handlungen.
	const bool bInRun = Director && Director->GetState().Phase == EGenesisSlicePhase::Gestation;
	if (!bInRun && TActorIterator<AGenesisWombScene>(GetWorld()))
	{
		const UGenesisFrontendSubsystem* Frontend = GameInstance->GetSubsystem<UGenesisFrontendSubsystem>();
		const float Scale = (Canvas->SizeY / 900.0f) * (Frontend ? Frontend->GetSettings().TextScalePercent / 100.0f : 1.0f);
		const FGenesisGestationPlanPoint Plan;
		const TArray<FGenesisGestationMoment> Moments;
		DrawWomb(Scale, Plan, Moments, bInRun);
		return;
	}
	if (bInRun)
	{
		const UGenesisFrontendSubsystem* Frontend = GameInstance->GetSubsystem<UGenesisFrontendSubsystem>();
		const float Scale = (Canvas->SizeY / 900.0f) * (Frontend ? Frontend->GetSettings().TextScalePercent / 100.0f : 1.0f);
		const FGenesisGestationPlanPoint Plan = Director->GetGestationPlanPoint();
		const TArray<FGenesisGestationMoment>& Moments = Director->Tuning.GestationMoments;
		if (Moments.IsValidIndex(Plan.MomentIndex))
		{
			const FGenesisGestationMoment& Moment = Moments[Plan.MomentIndex];
			const float Seconds = Plan.Alpha * Moment.Seconds;
			const float Alpha = FMath::Clamp(Seconds / 1.5f, 0.0f, 1.0f) * FMath::Clamp((9.0f - Seconds) / 2.0f, 0.0f, 1.0f);
			DrawCoverTitle(Moment.Title.ToUpper(), 0.40f * Canvas->SizeY, Scale * 1.6f, FLinearColor(0.82f, 0.60f, 0.28f, 0.9f * Alpha), 12.0f, 0.03f * Alpha);
			DrawCentered(Moment.Subtitle, 0.40f * Canvas->SizeY + 44.0f * Scale, Scale * 1.0f, 0.8f * Alpha, false);
		}
		const float Weeks = static_cast<float>(Plan.HoursAfterConception / (7.0 * 24.0)) + GenesisFetalLogic::WeeksFromConceptionToGestational;
		const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(GenesisFetalLogic::GetReference(), Weeks);
		const FString Weight = Fetal.WeightGrams >= 1000.0f ? FString::Printf(TEXT("%.2f kg"), Fetal.WeightGrams / 1000.0f)
			: FString::Printf(TEXT("%.0f g"), Fetal.WeightGrams);
		DrawCentered(FString::Printf(TEXT("SSW %d · %.0f cm · %s · Herz %.0f/min"), FMath::FloorToInt32(Weeks), Fetal.CrownHeelCm, *Weight,
			Fetal.HeartRateBpm), Canvas->SizeY - 88.0f * Scale, Scale * 0.95f, 0.55f, false);

		DrawWomb(Scale, Plan, Moments, bInRun);
		return;
	}

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
