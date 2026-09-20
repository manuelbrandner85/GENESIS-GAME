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

void AGenesisSliceHud::DrawHUD()
{
	Super::DrawHUD();

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance || !Canvas)
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

	auto FadeOf = [this, Now](float UsedSeconds)
	{
		if (UsedSeconds < 0.0f)
		{
			return 1.0f;
		}
		return FMath::Clamp(1.0f - (Now - UsedSeconds) / FMath::Max(0.1f, FadeAfterUseSeconds), 0.0f, 1.0f);
	};

	// Schreien geht immer. Suchen erst, wenn das Kind auf der Haut liegt – vorher wäre der Hinweis eine Lüge.
	DrawLine(TEXT("Leertaste: rufen"), 0.0f, FadeOf(CryUsedSeconds));
	if (State.bSkinToSkin && !State.bHasFed)
	{
		DrawLine(TEXT("E: suchen"), 1.0f, FadeOf(RootUsedSeconds));
	}
}
