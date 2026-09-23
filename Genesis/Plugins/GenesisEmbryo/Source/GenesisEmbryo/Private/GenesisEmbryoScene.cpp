// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoScene.h"
#include "CineCameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenesisEmbryoSubsystem.h"
#include "GenesisWorldClockSubsystem.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Entwickler: Belichtung und Licht des Embryoskops im laufenden Spiel einstellen. */
	FAutoConsoleCommandWithWorldAndArgs GenesisEmbryoSceneExposureCommand(
		TEXT("genesis.EmbryoScene.Exposure"),
		TEXT("Belichtung der Fruchthöhle (EV) und Licht am Embryo (lx): genesis.EmbryoScene.Exposure <bias> [lux]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisEmbryoScene> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->ExposureBias = FCString::Atof(*Args[0]);
					It->Camera->PostProcessSettings.AutoExposureBias = It->ExposureBias;
				}
				if (Args.Num() > 1)
				{
					It->TargetIlluminanceLux = FCString::Atof(*Args[1]);
				}
			}
		}));

	UStaticMeshComponent* MakePart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	}
}

namespace GenesisEmbryoSceneView
{
	float HeartContraction(double CyclePhase)
	{
		const float Phase = static_cast<float>(CyclePhase - FMath::FloorToDouble(CyclePhase));
		// Rasch zusammen (erstes Achtel), langsamer erschlaffen, dann Ruhe bis zum nächsten Schlag
		return FMath::SmoothStep(0.0f, 0.12f, Phase) * (1.0f - FMath::SmoothStep(0.18f, 0.45f, Phase));
	}

	FGenesisEmbryoSceneView Compute(const FGenesisEmbryogenesisState& State, double HoursSinceFusion, double BeatCycles)
	{
		FGenesisEmbryoSceneView View;
		View.Day = static_cast<float>(HoursSinceFusion / 24.0);
		// Der Körper ist für Tag 28 gebaut; in den Tagen davor wächst er auf diese Länge zu. Kleiner als 60 % zeigt ihn
		// die Szene nicht (dafür kommen eigene Formen), größer als das Modell wird er hier nicht.
		const float Length = State.LengthMm > 0.0f ? State.LengthMm : ModelLengthMm;
		View.Scale = FMath::Clamp(Length / ModelLengthMm, 0.6f, 1.0f);
		View.bHeartBeating = State.bHeartBeating;
		View.HeartRateBpm = State.bHeartBeating ? State.HeartRateBpm : 0.0f;
		View.HeartContraction = State.bHeartBeating ? HeartContraction(BeatCycles) : 0.0f;
		return View;
	}
}

AGenesisEmbryoScene::AGenesisEmbryoScene()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetMobility(EComponentMobility::Movable);

	Shell = MakePart(this, Body, TEXT("Shell"));
	// Die Hülle ist durchscheinend: Licht geht durch sie hindurch, sie wirft keinen Schatten
	Shell->SetCastShadow(false);
	HeartPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HeartPivot"));
	HeartPivot->SetupAttachment(Body);
	HeartPivot->SetMobility(EComponentMobility::Movable);
	Heart = MakePart(this, HeartPivot, TEXT("Heart"));
	Vessels = MakePart(this, Body, TEXT("Vessels"));
	NeuralTube = MakePart(this, Body, TEXT("NeuralTube"));
	Somites = MakePart(this, Body, TEXT("Somites"));
	Liver = MakePart(this, Body, TEXT("Liver"));

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetUsingAbsoluteLocation(true);
	Camera->SetUsingAbsoluteRotation(true);

	ScopeLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("ScopeLight"));
	ScopeLight->SetupAttachment(Camera);
	// Lichtleiter neben der Optik, leicht versetzt, damit Wölbungen lesbar bleiben. Warm wie das Gold des Covers.
	ScopeLight->SetRelativeLocation(FVector(0.0, 260.0, 180.0));
	ScopeLight->SetMobility(EComponentMobility::Movable);
	ScopeLight->bUseTemperature = true;
	ScopeLight->SetTemperature(4300.0f);
	ScopeLight->SetInnerConeAngle(22.0f);
	ScopeLight->SetOuterConeAngle(46.0f);
	ScopeLight->SetSourceRadius(300.0f);
	ScopeLight->SetAttenuationRadius(40000.0f);
	ScopeLight->SetCastShadows(true);
	ScopeLight->SetIntensityUnits(ELightUnits::Candelas);
	ScopeLight->SetVolumetricScatteringIntensity(0.35f);
}

void AGenesisEmbryoScene::BeginPlay()
{
	Super::BeginPlay();
	bCameraInitialized = false;
	SceneSeconds = 0.0f;
	FadeAlpha = 0.0f;

	// Das Herz zieht sich um seine eigene Mitte zusammen: Die Teile liegen alle um den Ursprung des Embryos,
	// deshalb sitzt der Drehpunkt in der Mitte des Herzens und das Netz ist dagegen zurückversetzt.
	if (Heart && Heart->GetStaticMesh())
	{
		const FVector Center = Heart->GetStaticMesh()->GetBounds().Origin;
		HeartPivot->SetRelativeLocation(Center);
		Heart->SetRelativeLocation(-Center);
	}

	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->bConstrainAspectRatio = false;
	Camera->SetCurrentFocalLength(FocalLengthMm);
	Camera->SetCurrentAperture(11.0f);
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 5.0f;
	Camera->PostProcessSettings.bOverride_bMegaLights = true;
	Camera->PostProcessSettings.bMegaLights = false;
	// Feste Belichtung wie beim Lookdev, gegen das Blender-Bild abgeglichen (Doc 32)
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias;
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Disable;
	Camera->SetFocusSettings(Focus);
}

void AGenesisEmbryoScene::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SceneSeconds += DeltaSeconds;

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisEmbryoSubsystem* EmbryoSystem = GameInstance ? GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>() : nullptr;
	if (EmbryoSystem && EmbryoSystem->HasEmbryo())
	{
		const FGenesisEmbryoState& State = EmbryoSystem->GetState();
		// Das Herz schlägt in Echtzeit, auch während des Zeitraffers: ein Schlag je Sekunde bei 60/min
		if (State.Embryogenesis.bHeartBeating)
		{
			BeatCycles += static_cast<double>(DeltaSeconds) * State.Embryogenesis.HeartRateBpm / 60.0;
		}
		// Das Alter aus der Weltuhr: Steht der Bauplan, rechnet der Embryo nicht mehr weiter und seine eigene Uhr bleibt
		// bei Tag 28 stehen – die Weltuhr springt in der Schwangerschaft in Wochen weiter. (Im gebauten Spiel gefunden:
		// Ohne das blieb das Bild über die ganze Schwangerschaft stehen und blendete nie ab.)
		double Hours = State.HoursSinceFusion;
		if (const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Hours = FMath::Max(Hours, static_cast<double>(Clock->GetNow() - State.FusionTime) / static_cast<double>(FGenesisTimestamp::SecondsPerHour));
		}
		View = GenesisEmbryoSceneView::Compute(State.Embryogenesis, Hours, BeatCycles);
	}
	else
	{
		// Ohne Simulation (Editor, Prüfkarte): das fertige Modell mit dem Takt vom Ende der vierten Woche
		BeatCycles += static_cast<double>(DeltaSeconds) * 111.0 / 60.0;
		View = FGenesisEmbryoSceneView();
		View.bHeartBeating = true;
		View.HeartRateBpm = 111.0f;
		View.Day = 28.0f;
		View.HeartContraction = GenesisEmbryoSceneView::HeartContraction(BeatCycles);
	}

	Body->SetRelativeScale3D(FVector(View.Scale));
	HeartPivot->SetRelativeScale3D(FVector(1.0f - HeartAmplitude * View.HeartContraction));

	if (bTakeView)
	{
		APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		if (Controller && Controller->GetViewTarget() != this)
		{
			Controller->SetViewTarget(this);
			// Der Standard-Pawn der Engine stünde sonst als graue Kugel in der Szene
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
			}
		}
	}
	UpdateCamera(DeltaSeconds, !bCameraInitialized);
	bCameraInitialized = true;
	UpdateFade(DeltaSeconds);
}

void AGenesisEmbryoScene::UpdateCamera(float DeltaSeconds, bool bSnap)
{
	const float HalfFov = FMath::Atan(0.5f * Camera->Filmback.SensorWidth / FMath::Max(1.0f, Camera->CurrentFocalLength));
	const float Distance = FramingWidthUm * View.Scale / (2.0f * FMath::Tan(HalfFov));
	const FVector Focus = GetActorLocation() + FocusOffset * View.Scale;
	const float Sway = SwayDegrees * FMath::Sin(2.0f * PI * SceneSeconds / FMath::Max(1.0f, SwayPeriodSeconds));
	const float Azimuth = FMath::DegreesToRadians(AzimuthDegrees + Sway);
	const float Elevation = FMath::DegreesToRadians(ElevationDegrees);
	const FVector Direction(FMath::Cos(Elevation) * FMath::Cos(Azimuth), FMath::Cos(Elevation) * FMath::Sin(Azimuth), FMath::Sin(Elevation));
	const FVector Desired = Focus + Direction * Distance;

	const float Alpha = bSnap ? 1.0f : 1.0f - FMath::Exp(-DeltaSeconds / FMath::Max(0.05f, FollowSeconds));
	CameraLocation = FMath::Lerp(CameraLocation, Desired, Alpha);
	CameraFocus = FMath::Lerp(CameraFocus, Focus, Alpha);
	Camera->SetWorldLocationAndRotation(CameraLocation, (CameraFocus - CameraLocation).Rotation());

	// Der Lichtleiter regelt nach: gleiche Beleuchtungsstärke am Embryo bei jedem Abstand (E = I / d², d in m; 1 µm = 1 cm)
	const float Meters = static_cast<float>(FVector::Dist(CameraLocation, CameraFocus)) / 100.0f;
	ScopeLight->SetIntensity(TargetIlluminanceLux * Meters * Meters);
}

void AGenesisEmbryoScene::UpdateFade(float DeltaSeconds)
{
	// In der Schwangerschaft springt die Zeit in Wochen; ein Embryo vom Ende der vierten Woche wäre dann falsch.
	// Die Kamera blendet ab, statt etwas zu zeigen, das nicht stimmt.
	const float Target = View.Day >= FadeFromDay ? 1.0f : 0.0f;
	FadeAlpha = FMath::FInterpConstantTo(FadeAlpha, Target, DeltaSeconds, 1.0f / FMath::Max(0.1f, FadeSeconds));
	APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->PlayerCameraManager && (FadeAlpha > 0.0f || Target > 0.0f))
	{
		Controller->PlayerCameraManager->SetManualCameraFade(FadeAlpha, FLinearColor::Black, false);
	}
}
