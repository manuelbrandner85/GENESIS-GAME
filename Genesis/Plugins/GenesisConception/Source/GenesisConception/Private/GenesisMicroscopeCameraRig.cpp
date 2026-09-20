// GENESIS: Der Kreislauf des Lebens

#include "GenesisMicroscopeCameraRig.h"
#include "CineCameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisSpermSwarm.h"
#include "GenesisSpermSwimLogic.h"
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

	// Eine Zelle wählen, die frei im Kanal schwimmt: an der Wand klebende Zellen lassen der Kamera keinen Platz
	if (bAutoPickCell && Swarm)
	{
		const float LumenRadius = Swarm->GetChannel().LumenRadiusUm;
		double BestScore = -MAX_dbl;
		for (int32 Index = 0; Index < Swarm->GetCellCount(); ++Index)
		{
			const FGenesisSpermCell* Candidate = Swarm->GetCell(Index);
			if (!Candidate)
			{
				continue;
			}
			const double Radius = FVector2D(Candidate->Position.Y, Candidate->Position.Z).Size();
			double Score = LumenRadius - Radius;
			if (Candidate->Motility != EGenesisSpermMotility::Progressive)
			{
				Score *= 0.5;
			}
			if (Score > BestScore)
			{
				BestScore = Score;
				FollowCellIndex = Index;
			}
		}
	}
	if (bBecomeViewTarget)
	{
		if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			Controller->SetViewTarget(this);
			// Der Standard-Pawn der Engine steht sonst als graue Kugel in der Szene
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
			}
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

	// Bezugsrahmen nur aus der Schwimmrichtung: stabil. Die Schlagebene der Zelle rollt mehrmals pro Sekunde –
	// würde die Kamera ihr folgen, würde sie um die Zelle geschleudert. Stattdessen rollt die Zelle langsam (RollPerBeat),
	// sodass die Geißelwelle über Sekunden sichtbar bleibt und sich nur allmählich dreht.
	const FTransform& SwarmTransform = Swarm->GetActorTransform();
	const FVector Head = Swarm->GetCellHeadWorldPosition(FollowCellIndex);
	const FVector Forward = SwarmTransform.TransformVectorNoScale(Cell->Heading).GetSafeNormal();
	// Blickachse senkrecht zur (geglätteten) Schlagebene: nur so ist die Geißelwelle als Welle zu sehen.
	// Geglättet, weil die Zelle langsam um ihre Längsachse rollt – die Kamera wandert mit, statt zu springen.
	FVector Side = FVector::CrossProduct(SmoothedBeatNormal, Forward).GetSafeNormal();
	Side = FVector::CrossProduct(Forward, Side).GetSafeNormal();
	if (Side.IsNearlyZero())
	{
		Side = FVector::CrossProduct(FMath::Abs(Forward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector, Forward).GetSafeNormal();
	}
	const FVector OrbitUp = FVector::CrossProduct(Forward, Side).GetSafeNormal();

	const float Azimuth = FMath::DegreesToRadians(OrbitAzimuthDegrees);
	const float Elevation = FMath::DegreesToRadians(OrbitElevationDegrees);
	const float Distance = OrbitDistanceUm * GenesisMicroScale::UnitsPerMicrometer;
	const FVector Offset = (Forward * FMath::Cos(Azimuth) * FMath::Cos(Elevation) + Side * FMath::Sin(Azimuth) * FMath::Cos(Elevation) + OrbitUp * FMath::Sin(Elevation)) * Distance;

	// Die Kamera darf nicht im Gewebe landen: notfalls auf die andere Seite der Zelle und näher heran
	FVector Candidate = Head + Offset;
	const float Limit = (Swarm->GetChannel().LumenRadiusUm - 25.0f) * GenesisMicroScale::UnitsPerMicrometer;
	auto RadialInChannel = [&SwarmTransform](const FVector& World)
	{
		const FVector Local = SwarmTransform.InverseTransformPosition(World);
		return static_cast<float>(FVector2D(Local.Y, Local.Z).Size());
	};
	if (RadialInChannel(Candidate) > Limit)
	{
		// Auf die gegenüberliegende Seite ausweichen, aber den Abstand halten – sonst klebt die Kamera am Kopf
		const FVector Mirrored = Head - Offset;
		Candidate = RadialInChannel(Mirrored) < RadialInChannel(Candidate) ? Mirrored : Candidate;
	}

	OutLocation = Candidate;
	const FVector LookAt = Head - Forward * LookBehindHeadUm * GenesisMicroScale::UnitsPerMicrometer;
	OutRotation = FRotationMatrix::MakeFromXZ(LookAt - OutLocation, OrbitUp).ToQuat();
	OutFocusDistance = static_cast<float>(FVector::Dist(OutLocation, Head));
	return true;
}

void AGenesisMicroscopeCameraRig::UpdateBeatNormal(float DeltaSeconds)
{
	const FGenesisSpermCell* Cell = Swarm ? Swarm->GetCell(FollowCellIndex) : nullptr;
	if (!Cell)
	{
		return;
	}
	FVector BeatSide = FVector::RightVector;
	FVector BeatNormal = FVector::UpVector;
	GenesisSpermSwimLogic::ComputeBeatFrame(*Cell, BeatSide, BeatNormal);
	const FVector WorldNormal = Swarm->GetActorTransform().TransformVectorNoScale(BeatNormal).GetSafeNormal();
	// Vorzeichen egal (Ebene, nicht Richtung): immer die näher liegende Seite nehmen, sonst kippt die Kamera bei jeder halben Drehung
	const FVector Target = FVector::DotProduct(WorldNormal, SmoothedBeatNormal) < 0.0 ? -WorldNormal : WorldNormal;
	const float Alpha = 1.0f - FMath::Exp(-DeltaSeconds / 1.5f);
	SmoothedBeatNormal = FMath::Lerp(SmoothedBeatNormal, Target, Alpha).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
}

void AGenesisMicroscopeCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBeatNormal(DeltaSeconds);

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

#if !UE_BUILD_SHIPPING
	if (!bDebugPageRegistered)
	{
		bDebugPageRegistered = true;
		TWeakObjectPtr<AGenesisMicroscopeCameraRig> WeakThis(this);
		GenesisDebug::RegisterPage({
			TEXT("MicroCam"),
			TEXT("Mikroskop-Kamera"),
			[WeakThis](const UWorld*, TArray<FString>& OutLines)
			{
				const AGenesisMicroscopeCameraRig* Self = WeakThis.Get();
				const FGenesisSpermCell* Cell = Self && Self->Swarm ? Self->Swarm->GetCell(Self->FollowCellIndex) : nullptr;
				if (!Cell)
				{
					OutLines.Add(TEXT("Keine Zelle verfolgt"));
					return;
				}
				const FVector Head = Self->Swarm->GetCellHeadWorldPosition(Self->FollowCellIndex);
				const FVector ToHead = (Head - Self->GetActorLocation());
				const FVector ViewDirection = Self->GetActorQuat().GetForwardVector();
				const double Radius = FVector2D(Cell->Position.Y, Cell->Position.Z).Size();
				OutLines.Add(FString::Printf(TEXT("Zelle %d | Position im Kanal: Radius %.0f von %.0f µm | Bewegungsart %s"),
					Self->FollowCellIndex, Radius, Self->Swarm->GetChannel().LumenRadiusUm,
					*StaticEnum<EGenesisSpermMotility>()->GetNameStringByValue(static_cast<int64>(Cell->Motility))));
				OutLines.Add(FString::Printf(TEXT("Abstand Kamera–Kopf %.0f µm | Winkel zur Blickachse %.1f° | Fokus %.0f µm | Blende f/%.1f, %.0f mm"),
					ToHead.Size(), FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ToHead.GetSafeNormal(), ViewDirection), -1.0, 1.0))),
					Self->CurrentFocusDistance, Self->Camera->CurrentAperture, Self->Camera->CurrentFocalLength));
				OutLines.Add(FString::Printf(TEXT("Kamera im Kanal: Radius %.0f µm | Licht %.0f cd | Schlagphase %.2f"),
					FVector2D(Self->GetActorLocation().Y, Self->GetActorLocation().Z).Size(), Self->EndoscopeLight->Intensity, Cell->BeatPhase));
			}
		});
	}
#endif
}
