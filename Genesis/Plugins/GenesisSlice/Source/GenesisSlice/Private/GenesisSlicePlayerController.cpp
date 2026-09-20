// GENESIS: Der Kreislauf des Lebens

#include "GenesisSlicePlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisBirthCameraRig.h"
#include "EngineUtils.h"
#include "GameFramework/InputSettings.h"
#include "GenesisLog.h"

AGenesisSlicePlayerController::AGenesisSlicePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	// Der Blick gehört dem Spieler, nicht der Maus: Kein Cursor, keine Menüführung in der Szene
	bShowMouseCursor = false;
}

void AGenesisSlicePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindAction(TEXT("GenesisCry"), IE_Pressed, this, &AGenesisSlicePlayerController::PressCry);
	InputComponent->BindAction(TEXT("GenesisCry"), IE_Released, this, &AGenesisSlicePlayerController::ReleaseCry);
	InputComponent->BindAction(TEXT("GenesisRoot"), IE_Pressed, this, &AGenesisSlicePlayerController::PressRoot);
	InputComponent->BindAction(TEXT("GenesisRoot"), IE_Released, this, &AGenesisSlicePlayerController::ReleaseRoot);
	InputComponent->BindAxis(TEXT("GenesisLookRight"), this, &AGenesisSlicePlayerController::LookRight);
	InputComponent->BindAxis(TEXT("GenesisLookUp"), this, &AGenesisSlicePlayerController::LookUp);

	// Nachsehen, welche Tasten die Einstellungen dafür wirklich hergeben – eine Bindung ohne
	// Mapping wäre eine Steuerung, die es nur im Code gibt.
	const UInputSettings* Settings = GetDefault<UInputSettings>();
	auto KeysFor = [Settings](const FName& Action)
	{
		FString Keys;
		if (Settings)
		{
			for (const FInputActionKeyMapping& Mapping : Settings->GetActionMappings())
			{
				if (Mapping.ActionName == Action)
				{
					Keys += (Keys.IsEmpty() ? TEXT("") : TEXT(", ")) + Mapping.Key.ToString();
				}
			}
		}
		return Keys.IsEmpty() ? FString(TEXT("keine Taste")) : Keys;
	};

	UE_LOG(LogGenesis, Display, TEXT("Steuerung: Rufen auf %s, Suchen auf %s – mehr kann ein Neugeborenes nicht."),
		*KeysFor(TEXT("GenesisCry")), *KeysFor(TEXT("GenesisRoot")));
}

void AGenesisSlicePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Schreien und Suchen schwellen an und ab – ein Neugeborenes schaltet nichts ein und aus
	const float CryTarget = bCryHeld ? 1.0f : 0.0f;
	CryInput = FMath::FInterpConstantTo(CryInput, CryTarget, DeltaTime, CryRampPerSecond);
	const float RootTarget = bRootHeld ? 1.0f : 0.0f;
	RootInput = FMath::FInterpConstantTo(RootInput, RootTarget, DeltaTime, RootRampPerSecond);

	// Der Kopf dreht sich langsam und kommt von selbst wieder zur Ruhe: Die Nackenmuskeln
	// eines Neugeborenen halten den Kopf noch nicht.
	LookOffset.X = FMath::Clamp(LookOffset.X + LookInput.X * 60.0f * DeltaTime, -MaxLookDegrees, MaxLookDegrees);
	LookOffset.Y = FMath::Clamp(LookOffset.Y + LookInput.Y * 45.0f * DeltaTime, -MaxLookDegrees * 0.6f, MaxLookDegrees * 0.6f);
	LookOffset = FMath::Vector2DInterpTo(LookOffset, FVector2D::ZeroVector, DeltaTime, 0.9f);
	LookInput = FVector2D::ZeroVector;

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr;
	if (EarlyLife && EarlyLife->HasNewborn())
	{
		EarlyLife->SetCryEffort(CryInput);
		EarlyLife->SetRootingEffort(RootInput);
	}

	// Der Blick geht an das Kamera-Rig der Szene: Es entscheidet, wie viel davon durchkommt
	for (TActorIterator<AGenesisBirthCameraRig> It(GetWorld()); It; ++It)
	{
		It->LookOffsetDegrees = LookOffset;
	}
}
