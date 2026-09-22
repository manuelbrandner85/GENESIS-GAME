// GENESIS: Der Kreislauf des Lebens

#include "GenesisBirthCameraRig.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
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

	Towel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Towel"));
	Towel->SetupAttachment(Camera);
	Towel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Towel->SetVisibility(false);
	// Ein Tuch auf dem Kopf wirft Schatten auf das Gesicht des Kindes – nicht in den Raum, den es ansieht
	Towel->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TowelMesh(TEXT("/Game/Genesis/Birth/Meshes/SM_GEN_BabyTowel.SM_GEN_BabyTowel"));
	if (TowelMesh.Succeeded())
	{
		Towel->SetStaticMesh(TowelMesh.Object);
	}
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

	// 3. Drehung: Das Kind dreht sich im Becken, um mit dem schmalsten Durchmesser durchzupassen
	float Roll = FMath::Lerp(0.0f, 88.0f, State.Rotation) + SmoothedPressure * 3.0f;
	// Nach der Geburt: Die Hebamme nimmt das Kind auf und hält es mit dem Gesicht nach oben. Die
	// Drehung aus dem Becken löst sich in den Sekunden, in denen es aus dem Kanal kommt – sonst
	// stünde der Raum für das Kind minutenlang auf der Seite.
	if (State.IsBorn())
	{
		const float Righting = FMath::SmoothStep(0.0f, 8.0f, State.SecondsSinceBirth);
		Roll = FMath::Lerp(Roll, 8.0f, Righting);
	}
	// Dazu der eigene Blick: Das Kind wendet sich der Stimme zu, soweit es das kann
	FRotator ExitRotation(-4.0f * SmoothedPressure + LookOffsetDegrees.Y, LookOffsetDegrees.X, Roll);
	FVector Local(Depth + Push, Beat * BeatAmplitude * 0.5f, Beat * BeatAmplitude);

	// 3a. In den Händen der Hebamme: Sie fängt das Kind auf und hebt es zu sich hoch. Ohne sie lag das Kind nach
	// der Geburt tief am Fußende und sah einen leeren Raum – in Wirklichkeit ist ihr Gesicht das Erste, was es sieht.
	const float Held = FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(MidwifeHoldBlend, 0.0f, 1.0f));
	if (State.IsBorn() && Held > KINDA_SMALL_NUMBER)
	{
		const FVector HeldLocation = GetActorTransform().InverseTransformPosition(MidwifeHeldView.GetLocation());
		const FQuat HeldRotation = GetActorQuat().Inverse() * MidwifeHeldView.GetRotation()
			* FRotator(LookOffsetDegrees.Y, LookOffsetDegrees.X, 0.0f).Quaternion();
		// Ein kleiner Bogen nach oben beim Hochheben, dazu der eigene Herzschlag
		const FVector Arc(0.0f, 0.0f, 80.0f * FMath::Sin(Held * PI));
		Local = FMath::Lerp(Local, HeldLocation + FVector(0.0f, 0.0f, Beat * BeatAmplitude * 0.3f), Held) + Arc;
		ExitRotation = FQuat::Slerp(ExitRotation.Quaternion(), HeldRotation, Held).Rotator();
	}

	// 3b. Auf die Brust. Die Hebamme hebt das Kind in einem Bogen über den Bauch – nicht auf
	// gerader Linie, und nicht in einem Schnitt: Der Weg ist das Erste, was das Kind vom Raum sieht.
	const float ChestTarget = State.IsBorn() && bOnMothersChest ? 1.0f : 0.0f;
	ChestBlend = FMath::FInterpConstantTo(ChestBlend, ChestTarget, DeltaSeconds, 1.0f / FMath::Max(0.5f, LiftSeconds));
	const float Lift = FMath::SmoothStep(0.0f, 1.0f, ChestBlend);
	if (Lift <= KINDA_SMALL_NUMBER)
	{
		Camera->SetRelativeLocation(Local);
		Camera->SetRelativeRotation(ExitRotation);
	}
	else
	{
		// Die Brust hebt und senkt sich mit dem Atem der Mutter – das Kind liegt auf einem Menschen, nicht auf einem Tisch
		BreathPhase += DeltaSeconds * MotherBreathsPerMinute / 60.0f;
		const float BreathLift = MotherBreathLift >= 0.0f
			? FMath::Clamp(MotherBreathLift, 0.0f, 1.0f)
			: 0.5f * (1.0f - FMath::Cos(BreathPhase * 2.0f * PI));
		const FVector ChestNormal(0.707f, 0.0f, 0.707f);
		const FVector Breath = ChestNormal * (MotherBreathMm * BreathLift);
		const FVector ChestLocation = ChestEyeLocation + Breath + FVector(0.0f, 0.0f, Beat * BeatAmplitude * 0.3f);
		const FVector Arc(0.0f, 0.0f, LiftArcMm * FMath::Sin(Lift * PI));

		const FQuat Look = FRotator(LookOffsetDegrees.Y, LookOffsetDegrees.X, 0.0f).Quaternion();
		const FQuat ChestBase = FRotationMatrix::MakeFromXZ(ChestViewForward.GetSafeNormal(), ChestViewUp.GetSafeNormal()).ToQuat();
		FVector HeldLocation = ChestLocation;
		FQuat HeldLook = ChestBase * Look;

		// Vor ihrem Gesicht: Sie hebt das Kind mit beiden Händen, in einem flachen Bogen zu sich hoch.
		// Der Blick bleibt beim Kind – es kann sich umsehen, aber geradeaus liegen ihre Augen.
		const float Face = FMath::Clamp(EnFaceBlend, 0.0f, 1.0f);
		if (Face > KINDA_SMALL_NUMBER)
		{
			const FVector FaceLocation = GetActorTransform().InverseTransformPosition(EnFaceView.GetLocation());
			const FQuat FaceRotation = GetActorQuat().Inverse() * EnFaceView.GetRotation();
			const FVector Raise = ChestNormal * (60.0f * FMath::Sin(Face * PI));
			HeldLocation = FMath::Lerp(ChestLocation, FaceLocation, Face) + Raise;
			HeldLook = FQuat::Slerp(ChestBase, FaceRotation, Face) * Look;
		}

		// Abrubbeln: Der kleine Körper geht unter der Hand mit – ein paar Millimeter vor und zurück, ein wenig Drehung
		const float Rub = FMath::Clamp(DryingRub, 0.0f, 1.0f);
		RubPhase += DeltaSeconds * 1.6f;
		const float Stroke = FMath::Sin(RubPhase * 2.0f * PI);
		const FVector RubOffset(3.0f * Rub * Stroke, 0.0f, 0.8f * Rub * FMath::Abs(Stroke));
		const FQuat RubRoll = FRotator(0.0f, 0.0f, 1.2f * Rub * Stroke).Quaternion();

		Camera->SetRelativeLocation(FMath::Lerp(Local, HeldLocation, Lift) + Arc + RubOffset * Lift);
		Camera->SetRelativeRotation((FQuat::Slerp(ExitRotation.Quaternion(), HeldLook, Lift) * RubRoll).Rotator());
	}

	// Das Tuch: beim Abrubbeln liegt es weiter hinten auf dem Rücken und fährt mit der Hand hin und her,
	// zugedeckt reicht es über den Hinterkopf bis an die Stirn – dann sieht das Kind oben im Bild seinen Rand.
	if (Towel)
	{
		const float Cover = FMath::Clamp(TowelCover, 0.0f, 1.0f);
		const float Rub = FMath::Clamp(DryingRub, 0.0f, 1.0f);
		const bool bShow = Lift > 0.99f && (Rub > 0.01f || Cover > 0.01f);
		Towel->SetVisibility(bShow);
		if (bShow)
		{
			const FVector Drying(-70.0f + 25.0f * FMath::Sin(RubPhase * 2.0f * PI), 0.0f, 6.0f);
			// Zugedeckt liegt das Tuch auf Hinterkopf und Rücken; vom Kind aus nur ein schmaler Rand oben im Bild (weiter
			// vorn verdeckte es als graues Band ein Drittel des Bildes – auch das Gesicht der Mutter)
			const FVector Lowering(-14.0f, 0.0f, 20.0f + 90.0f * (1.0f - FMath::SmoothStep(0.0f, 0.7f, Cover)));
			Towel->SetRelativeLocation(Rub > 0.01f && Cover <= 0.01f ? Drying : Lowering);
		}
	}

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
