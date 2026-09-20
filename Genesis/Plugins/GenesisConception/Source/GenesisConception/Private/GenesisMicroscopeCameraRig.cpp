// GENESIS: Der Kreislauf des Lebens

#include "GenesisMicroscopeCameraRig.h"
#include "CineCameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GenesisSpermSwarm.h"
#include "GenesisSpermSwimTypes.h"

AGenesisMicroscopeCameraRig::AGenesisMicroscopeCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	// Vollformatbreite mit 16:9-Ausschnitt (Video-Crop, keine Balken im Spielfenster), leichtes Tele, Blende f/8:
	// Kopf und Mittelstück scharf, Geißel und Hintergrund fallen weich ab
	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->bConstrainAspectRatio = false;
	Camera->SetCurrentFocalLength(85.0f);
	Camera->SetCurrentAperture(8.0f);
	FCameraFocusSettings Focus;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = 110.0f;
	Camera->SetFocusSettings(Focus);

	EndoscopeLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("EndoscopeLight"));
	EndoscopeLight->SetupAttachment(Camera);
	// Lichtleiter direkt neben der Optik: Kaltlicht-LED ~5.600 K, kleine Austrittsfläche
	EndoscopeLight->SetRelativeLocation(FVector(0.0, 1.5, -1.0));
	EndoscopeLight->SetMobility(EComponentMobility::Movable);
	EndoscopeLight->bUseTemperature = true;
	EndoscopeLight->SetTemperature(5600.0f);
	EndoscopeLight->SetInnerConeAngle(22.0f);
	EndoscopeLight->SetOuterConeAngle(48.0f);
	EndoscopeLight->SetSourceRadius(1.0f);
	EndoscopeLight->SetAttenuationRadius(4000.0f);
	EndoscopeLight->SetCastShadows(true);
}

void AGenesisMicroscopeCameraRig::BeginPlay()
{
	Super::BeginPlay();
	bInitialized = false;
	if (bBecomeViewTarget)
	{
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->SetViewTarget(this);
		}
	}
}

bool AGenesisMicroscopeCameraRig::ComputeDesired(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const
{
	const FGenesisSpermCell* Cell = Swarm ? Swarm->GetCell(FollowCellIndex) : nullptr;
	if (!Cell)
	{
		return false;
	}

	// Stabiler Bezugsrahmen aus der Schwimmrichtung – ohne Schlag-Pendeln und Rollen, sonst würde die Kamera zittern
	const FTransform& SwarmTransform = Swarm->GetActorTransform();
	const FVector Head = Swarm->GetCellHeadWorldPosition(FollowCellIndex);
	const FVector Forward = SwarmTransform.TransformVectorNoScale(Cell->Heading).GetSafeNormal();
	const FVector Up = FMath::Abs(Forward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
	const FVector Side = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	const FVector OrbitUp = FVector::CrossProduct(Forward, Side).GetSafeNormal();

	const float Azimuth = FMath::DegreesToRadians(OrbitAzimuthDegrees);
	const float Elevation = FMath::DegreesToRadians(OrbitElevationDegrees);
	const float Distance = OrbitDistanceUm * GenesisMicroScale::UnitsPerMicrometer;
	const FVector Offset = (Forward * FMath::Cos(Azimuth) * FMath::Cos(Elevation) + Side * FMath::Sin(Azimuth) * FMath::Cos(Elevation) + OrbitUp * FMath::Sin(Elevation)) * Distance;

	OutLocation = Head + Offset;
	const FVector LookAt = Head - Forward * LookBehindHeadUm * GenesisMicroScale::UnitsPerMicrometer;
	OutRotation = FRotationMatrix::MakeFromXZ(LookAt - OutLocation, OrbitUp).ToQuat();
	OutFocusDistance = static_cast<float>(FVector::Dist(OutLocation, Head));
	return true;
}

void AGenesisMicroscopeCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector DesiredLocation;
	FQuat DesiredRotation;
	float DesiredFocus = 0.0f;
	if (!ComputeDesired(DesiredLocation, DesiredRotation, DesiredFocus))
	{
		return;
	}

	// Umlauf am Abschnittsende: springen statt quer durch den Kanal zu fahren
	const bool bJump = Swarm && FVector::Dist(GetActorLocation(), DesiredLocation) > 0.5f * Swarm->GetChannel().LengthUm * GenesisMicroScale::UnitsPerMicrometer;
	if (!bInitialized || bJump)
	{
		SetActorLocationAndRotation(DesiredLocation, DesiredRotation);
		CurrentFocusDistance = DesiredFocus;
		bInitialized = true;
	}
	else
	{
		const float PositionAlpha = 1.0f - FMath::Exp(-DeltaSeconds / PositionSmoothingSeconds);
		const float RotationAlpha = 1.0f - FMath::Exp(-DeltaSeconds / RotationSmoothingSeconds);
		const float FocusAlpha = 1.0f - FMath::Exp(-DeltaSeconds / FocusSmoothingSeconds);
		SetActorLocationAndRotation(FMath::Lerp(GetActorLocation(), DesiredLocation, PositionAlpha), FQuat::Slerp(GetActorQuat(), DesiredRotation, RotationAlpha));
		// Fokus auf den tatsächlichen Abstand der geglätteten Kamera zum Kopf
		const float ActualDistance = static_cast<float>(FVector::Dist(GetActorLocation(), Swarm->GetCellHeadWorldPosition(FollowCellIndex)));
		CurrentFocusDistance = FMath::Lerp(CurrentFocusDistance, ActualDistance, FocusAlpha);
	}

	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = CurrentFocusDistance;
	Camera->SetFocusSettings(Focus);
}
