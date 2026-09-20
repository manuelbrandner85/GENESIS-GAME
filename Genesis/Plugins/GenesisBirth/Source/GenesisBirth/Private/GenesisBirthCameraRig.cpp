// GENESIS: Der Kreislauf des Lebens

#include "GenesisBirthCameraRig.h"
#include "CineCameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenesisBirthSubsystem.h"

#if !UE_BUILD_SHIPPING
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisBirthExposureCommand(
		TEXT("genesis.Birth.Exposure"),
		TEXT("Belichtung der Kindkamera: <im Kanal> [draußen] (EV, größer = heller)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisBirthCameraRig> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->DarkExposureBias = FCString::Atof(*Args[0]);
				}
				if (Args.Num() > 1)
				{
					It->LightExposureBias = FCString::Atof(*Args[1]);
				}
				UE_LOG(LogTemp, Display, TEXT("GENESIS Geburtskamera: Kanal %.1f EV, draußen %.1f EV"),
					It->DarkExposureBias, It->LightExposureBias);
			}
		}));
}
#endif

AGenesisBirthCameraRig::AGenesisBirthCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	// Weitwinkel: Die Wände sind wenige Zentimeter vom Gesicht entfernt, es gibt keinen Abstand zu gewinnen
	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->bConstrainAspectRatio = false;
	Camera->SetCurrentFocalLength(16.0f);
	Camera->SetCurrentAperture(2.0f);
	// In Millimetern gerechnet liegt die Wand direkt vor der Linse – die Standard-Nahgrenze würde sie wegschneiden
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 1.0f;
}

void AGenesisBirthCameraRig::BeginPlay()
{
	Super::BeginPlay();

	if (bBecomeViewTarget)
	{
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->SetViewTarget(this);
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
			}
		}
	}
}

void AGenesisBirthCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBirthSubsystem* Birth = GameInstance ? GameInstance->GetSubsystem<UGenesisBirthSubsystem>() : nullptr;
	if (!Birth || !Birth->HasLabor())
	{
		return;
	}

	ApplyPerception(Birth->GetState(), Birth->GetPerception(), DeltaSeconds);
}

void AGenesisBirthCameraRig::ApplyPerception(const FGenesisBirthState& State, const FGenesisBirthPerception& Perception, float DeltaSeconds)
{
	if (!Camera)
	{
		return;
	}

	// 1. Lage im Kanal. Vor der Austreibung liegt das Kind tief, danach schiebt es sich zum Ausgang.
	const float Travel = State.IsBorn()
		? AfterBirthTravelMm * FMath::Clamp(State.SecondsSinceBirth / 8.0f, 0.0f, 1.0f)
		: 0.0f;
	const float Depth = -CanalLengthMm * (1.0f - State.Descent) + Travel;

	// Der Kopf wird bei jeder Wehe ein Stück vorgeschoben und rutscht danach leicht zurück –
	// dieses Vor und Zurück ist der Grund, warum eine Geburt Stunden dauert.
	SmoothedPressure = FMath::FInterpTo(SmoothedPressure, Perception.Pressure, DeltaSeconds, 6.0f);
	const float Push = SmoothedPressure * 6.0f;

	// 2. Der eigene Herzschlag. Er ist das Einzige, was in dieser Dunkelheit verlässlich da ist.
	HeartPhase += DeltaSeconds * FMath::Max(40.0f, Perception.HeartRateBpm) / 60.0f;
	const float Beat = FMath::Pow(FMath::Max(0.0f, FMath::Sin(HeartPhase * 2.0f * PI)), 8.0f);
	const float BeatAmplitude = FMath::Lerp(0.4f, 1.6f, 1.0f - Perception.Oxygen);

	const FVector Local(Depth + Push, Beat * BeatAmplitude * 0.5f, Beat * BeatAmplitude);
	Camera->SetRelativeLocation(Local);

	// 3. Drehung: Das Kind dreht sich im Becken, um mit dem schmalsten Durchmesser durchzupassen
	const float Roll = FMath::Lerp(0.0f, 88.0f, State.Rotation) + SmoothedPressure * 3.0f;
	Camera->SetRelativeRotation(FRotator(-4.0f * SmoothedPressure, 0.0f, Roll));

	// 4. Belichtung: dunkel im Kanal, grell beim Durchtritt. Der Wechsel ist ein Sprung, kein Verlauf.
	SmoothedLight = FMath::FInterpTo(SmoothedLight, Perception.Light, DeltaSeconds, 2.5f);
	FPostProcessSettings& Post = Camera->PostProcessSettings;
	Post.bOverride_AutoExposureMethod = true;
	Post.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Post.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Post.AutoExposureApplyPhysicalCameraExposure = true;
	Post.bOverride_AutoExposureBias = true;
	Post.AutoExposureBias = FMath::Lerp(DarkExposureBias, LightExposureBias, SmoothedLight);

	// 5. Sauerstoffmangel: Die Sicht wird grau und eng – der Tunnelblick ist keine Erfindung,
	// sondern die Netzhaut, der das Blut fehlt.
	const float Deficit = FMath::Clamp(1.0f - Perception.Oxygen, 0.0f, 1.0f);
	Post.bOverride_ColorSaturation = true;
	Post.ColorSaturation = FVector4(FVector(FMath::Lerp(1.0f, 0.25f, Deficit)), 1.0);
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = FMath::Clamp(0.35f + 1.1f * Deficit + 0.4f * SmoothedPressure, 0.0f, 1.6f);

	// 6. Sehschärfe. Vor der Geburt hilft kein Fokus, danach sieht ein Neugeborenes nur auf Armlänge scharf.
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = State.IsBorn() ? NewbornFocusDistanceMm : FMath::Max(8.0f, 40.0f - 20.0f * Perception.Tightness);
	Camera->SetFocusSettings(Focus);
	// Weit offene Blende nach der Geburt: Alles außerhalb einer Armlänge verschwimmt
	Camera->SetCurrentAperture(State.IsBorn() ? 1.4f : 2.8f);

	// 7. Enge: Der Bildwinkel wird kleiner, wenn der Kopf im knöchernen Ring steckt.
	// Sensor und Brennweite wachsen gemeinsam um den Makro-Faktor – gleicher Bildwinkel, flache Schärfe.
	const float Scale = FMath::Max(1.0f, MacroScale);
	const float Nominal = FMath::Lerp(16.0f, 24.0f, Perception.Tightness * (1.0f - SmoothedLight));
	Camera->LensSettings.MaxFocalLength = FMath::Max(Camera->LensSettings.MaxFocalLength, Nominal * Scale + 1.0f);
	Camera->Filmback.SensorWidth = 36.0f * Scale;
	Camera->Filmback.SensorHeight = 20.25f * Scale;
	Camera->SetCurrentFocalLength(Nominal * Scale);
}
