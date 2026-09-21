// GENESIS: Der Kreislauf des Lebens

#include "GenesisSlicePlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisBirthCameraRig.h"
#include "GenesisBootFlow.h"
#include "GenesisMicroscopeCameraRig.h"
#include "GenesisSpermSwarm.h"
#include "EngineUtils.h"
#include "GameFramework/InputSettings.h"
#include "GenesisLog.h"

AGenesisSlicePlayerController::AGenesisSlicePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	// Der Blick gehört dem Spieler, nicht der Maus: Kein Cursor, keine Menüführung in der Szene
	bShowMouseCursor = false;
	// Ohne das hier bliebe die Steuerung in der Pause stehen – und man käme nie wieder heraus.
	bShouldPerformFullTickWhenPaused = true;
}

void AGenesisSlicePlayerController::ToggleView()
{
	for (TActorIterator<AGenesisMicroscopeCameraRig> It(GetWorld()); It; ++It)
	{
		It->ToggleRaceView();
		It->PlayerOrbitDegrees = FVector2D::ZeroVector;
		bViewToggled = true;
	}
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
	InputComponent->BindAxis(TEXT("GenesisSteerRight"), this, &AGenesisSlicePlayerController::SteerRight);
	InputComponent->BindAxis(TEXT("GenesisSteerUp"), this, &AGenesisSlicePlayerController::SteerUp);
	InputComponent->BindAction(TEXT("GenesisToggleView"), IE_Pressed, this, &AGenesisSlicePlayerController::ToggleView);

	// Das Menü muss auch dann reagieren, wenn die Welt steht – sonst kommt man aus der Pause
	// nicht mehr heraus. `bExecuteWhenPaused` ist genau dafür da.
	auto BindMenu = [this](const TCHAR* Action, void (AGenesisSlicePlayerController::*Handler)())
	{
		FInputActionBinding& Binding = InputComponent->BindAction(Action, IE_Pressed, this, Handler);
		Binding.bExecuteWhenPaused = true;
	};
	// „Drücke eine beliebige Taste" heißt wirklich jede: Tastatur, Maus, jeder Controller-Knopf.
	// Achsen (Stick, Maus bewegen) zählen nicht – wer nur den Controller aufnimmt, soll nichts überspringen.
	FInputKeyBinding& AnyKey = InputComponent->BindKey(EKeys::AnyKey, IE_Pressed, this, &AGenesisSlicePlayerController::AnyKeyPressed);
	AnyKey.bExecuteWhenPaused = true;
	AnyKey.bConsumeInput = false;

	BindMenu(TEXT("GenesisMenu"), &AGenesisSlicePlayerController::MenuToggle);
	BindMenu(TEXT("GenesisAccept"), &AGenesisSlicePlayerController::MenuAccept);
	BindMenu(TEXT("GenesisBack"), &AGenesisSlicePlayerController::MenuBack);
	BindMenu(TEXT("GenesisMenuUp"), &AGenesisSlicePlayerController::MenuUp);
	BindMenu(TEXT("GenesisMenuDown"), &AGenesisSlicePlayerController::MenuDown);
	BindMenu(TEXT("GenesisMenuLeft"), &AGenesisSlicePlayerController::MenuLeft);
	BindMenu(TEXT("GenesisMenuRight"), &AGenesisSlicePlayerController::MenuRight);

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
	UE_LOG(LogGenesis, Display, TEXT("Menü: Öffnen auf %s, Bestätigen auf %s, Zurück auf %s."),
		*KeysFor(TEXT("GenesisMenu")), *KeysFor(TEXT("GenesisAccept")), *KeysFor(TEXT("GenesisBack")));
}

FString AGenesisSlicePlayerController::DescribeAction(FName Action)
{
	const UInputSettings* Settings = GetDefault<UInputSettings>();
	if (!Settings)
	{
		return FString();
	}

	// Die Engine-Namen ("SpaceBar", "Gamepad_FaceButton_Bottom") stehen so nicht im Bild.
	// Für die wenigen Tasten, die dieses Spiel hat, genügt eine kurze Übersetzung.
	static const TMap<FName, FString> Names = {
		{ TEXT("SpaceBar"), TEXT("Leertaste") },
		{ TEXT("Escape"), TEXT("Esc") },
		{ TEXT("Enter"), TEXT("Enter") },
		{ TEXT("BackSpace"), TEXT("Rücktaste") },
		{ TEXT("Gamepad_FaceButton_Bottom"), TEXT("A") },
		{ TEXT("Gamepad_FaceButton_Right"), TEXT("B") },
		{ TEXT("Gamepad_FaceButton_Left"), TEXT("X") },
		{ TEXT("Gamepad_FaceButton_Top"), TEXT("Y") },
		{ TEXT("Gamepad_Special_Right"), TEXT("Start") },
		{ TEXT("Gamepad_Special_Left"), TEXT("Zurück") },
		{ TEXT("Gamepad_RightThumbstick"), TEXT("rechter Stick drücken") }
	};
	auto Pretty = [](const FKey& Key)
	{
		if (const FString* Found = Names.Find(Key.GetFName()))
		{
			return *Found;
		}
		return Key.GetDisplayName().ToString();
	};

	FString Keyboard;
	FString Gamepad;
	for (const FInputActionKeyMapping& Mapping : Settings->GetActionMappings())
	{
		if (Mapping.ActionName != Action)
		{
			continue;
		}
		if (Mapping.Key.IsGamepadKey())
		{
			if (Gamepad.IsEmpty()) { Gamepad = Pretty(Mapping.Key); }
		}
		else if (Keyboard.IsEmpty())
		{
			Keyboard = Pretty(Mapping.Key);
		}
	}

	if (Keyboard.IsEmpty()) { return Gamepad; }
	if (Gamepad.IsEmpty()) { return Keyboard; }
	return Keyboard + TEXT(" / ") + Gamepad;
}

bool AGenesisSlicePlayerController::IsMenuOpen() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	return Frontend && Frontend->GetPage() != EGenesisMenuPage::Keine;
}

void AGenesisSlicePlayerController::AnyKeyPressed()
{
	if (UGenesisFrontendSubsystem* Frontend = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr)
	{
		Frontend->PressAnyKey();
	}
}

void AGenesisSlicePlayerController::MenuToggle()
{
	if (UGenesisFrontendSubsystem* Frontend = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr)
	{
		Frontend->ToggleMenu();
	}
}

void AGenesisSlicePlayerController::MenuAccept()
{
	if (!IsMenuOpen())
	{
		return;
	}
	if (UGenesisFrontendSubsystem* Frontend = GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>())
	{
		Frontend->Accept();
	}
}

void AGenesisSlicePlayerController::MenuBack()
{
	if (!IsMenuOpen())
	{
		return;
	}
	if (UGenesisFrontendSubsystem* Frontend = GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>())
	{
		Frontend->Back();
	}
}

void AGenesisSlicePlayerController::MenuUp()
{
	if (IsMenuOpen())
	{
		GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>()->MoveSelection(-1);
	}
}

void AGenesisSlicePlayerController::MenuDown()
{
	if (IsMenuOpen())
	{
		GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>()->MoveSelection(1);
	}
}

void AGenesisSlicePlayerController::MenuLeft()
{
	if (IsMenuOpen())
	{
		GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>()->AdjustSelection(-1);
	}
}

void AGenesisSlicePlayerController::MenuRight()
{
	if (IsMenuOpen())
	{
		GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>()->AdjustSelection(1);
	}
}

void AGenesisSlicePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Solange ein Menü offen ist, bekommt das Kind keine Eingabe: Wer im Menü nach unten geht,
	// soll nicht nebenbei schreien. Angefangene Eingaben laufen dabei aus, sie brechen nicht ab.
	if (IsMenuOpen())
	{
		bCryHeld = false;
		bRootHeld = false;
		LookInput = FVector2D::ZeroVector;
	}

	// Blickempfindlichkeit und Y-Achse kommen aus den Einstellungen – für Maus und Stick gleich
	if (const UGenesisFrontendSubsystem* Frontend = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr)
	{
		const FGenesisPlayerSettings& Settings = Frontend->GetSettings();
		LookInput.X *= Settings.LookSensitivity;
		LookInput.Y *= Settings.LookSensitivity * (Settings.bInvertLookY ? -1.0f : 1.0f);
	}

	// Schreien und Suchen schwellen an und ab – ein Neugeborenes schaltet nichts ein und aus
	const float CryTarget = bCryHeld ? 1.0f : 0.0f;
	CryInput = FMath::FInterpConstantTo(CryInput, CryTarget, DeltaTime, CryRampPerSecond);
	const float RootTarget = bRootHeld ? 1.0f : 0.0f;
	RootInput = FMath::FInterpConstantTo(RootInput, RootTarget, DeltaTime, RootRampPerSecond);

	// Das Wettrennen: Lenken und Schlagen gehen an die eigene Zelle
	if (IsMenuOpen())
	{
		SteerInput = FVector2D::ZeroVector;
		StrokePresses = 0;
	}
	if (!SteerInput.IsNearlyZero(0.2))
	{
		bSteerUsed = true;
	}
	for (TActorIterator<AGenesisSpermSwarm> It(GetWorld()); It; ++It)
	{
		if (It->IsRacing())
		{
			It->SetPlayerInput(SteerInput, StrokePresses);
		}
	}
	StrokePresses = 0;

	// In der Mikrowelt führt der Spieler das Mikroskop: Es schwenkt um das Motiv und bleibt stehen,
	// wo man es hinstellt. Nicht im Vorspann – dort gehört die Kamerafahrt dem Prolog.
	const UGenesisFrontendSubsystem* FrontendForLook = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	const EGenesisBootStage Stage = FrontendForLook ? FrontendForLook->GetBootStage() : EGenesisBootStage::Aus;
	if (Stage == EGenesisBootStage::Aus || Stage == EGenesisBootStage::Spiel)
	{
		for (TActorIterator<AGenesisMicroscopeCameraRig> It(GetWorld()); It; ++It)
		{
			It->PlayerOrbitDegrees.X = FMath::Fmod(It->PlayerOrbitDegrees.X + LookInput.X * 50.0f * DeltaTime, 360.0f);
			It->PlayerOrbitDegrees.Y = FMath::Clamp(It->PlayerOrbitDegrees.Y + LookInput.Y * 35.0f * DeltaTime,
				-It->MaxPlayerPitchDegrees, It->MaxPlayerPitchDegrees);
			if (!LookInput.IsNearlyZero())
			{
				bMicroscopeMoved = true;
			}
		}
	}

	// Der Kopf dreht sich langsam und kommt von selbst wieder zur Ruhe: Die Nackenmuskeln
	// eines Neugeborenen halten den Kopf noch nicht.
	LookOffset.X = FMath::Clamp(LookOffset.X + LookInput.X * 60.0f * DeltaTime, -MaxLookDegrees, MaxLookDegrees);
	LookOffset.Y = FMath::Clamp(LookOffset.Y + LookInput.Y * 45.0f * DeltaTime, -MaxLookDegrees * 0.6f, MaxLookDegrees * 0.6f);
	LookOffset = FMath::Vector2DInterpTo(LookOffset, FVector2D::ZeroVector, DeltaTime, 0.9f);
	LookInput = FVector2D::ZeroVector;

	// Das Gesicht der Mutter suchen: eine Weile nach oben sehen. Loslassen erst mit einem Blick nach unten.
	const float MaxPitch = MaxLookDegrees * 0.6f;
	SeekUpSeconds = LookOffset.Y > MaxPitch * 0.7f ? SeekUpSeconds + DeltaTime : 0.0f;
	if (!bSeekingFace && SeekUpSeconds >= SeekFaceSeconds)
	{
		bSeekingFace = true;
		UE_LOG(LogGenesis, Display, TEXT("Das Kind sucht das Gesicht der Mutter."));
	}
	else if (bSeekingFace && LookOffset.Y < -MaxPitch * 0.6f)
	{
		bSeekingFace = false;
		UE_LOG(LogGenesis, Display, TEXT("Das Kind sieht wieder weg."));
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr;
	if (EarlyLife && EarlyLife->HasNewborn())
	{
		EarlyLife->SetCryEffort(CryInput);
		EarlyLife->SetRootingEffort(RootInput);
	}

	// Der Blick geht an das Kamera-Rig der Szene: Es entscheidet, wie viel davon durchkommt
	bool bOnChest = false;
	for (TActorIterator<AGenesisBirthCameraRig> It(GetWorld()); It; ++It)
	{
		It->LookOffsetDegrees = LookOffset;
		bOnChest |= It->bOnMothersChest;
	}
	// Suchen gibt es nur auf ihrer Brust – ein Blick nach oben in den Händen der Hebamme zählt nicht
	if (!bOnChest)
	{
		bSeekingFace = false;
	}
}
