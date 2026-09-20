// GENESIS: Der Kreislauf des Lebens

#include "GenesisMicroscopeCameraRig.h"
#include "CineCameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisOocyte.h"
#include "GenesisSpermSwarm.h"
#include "GenesisSpermSwimLogic.h"
#include "GenesisSpermSwimTypes.h"

#if !UE_BUILD_SHIPPING
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisWatchOocyteCommand(
		TEXT("genesis.Conception.WatchOocyte"),
		TEXT("Kamera auf die Eizelle richten (1) oder wieder einer Zelle folgen (0)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const bool bWatch = Args.Num() == 0 || Args[0] != TEXT("0");
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				It->bWatchOocyte = bWatch;
				// Ein manueller Schwenk zurück soll nicht sofort wieder umschalten
				It->bWatchFertilization = bWatch;
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisCameraCommand(
		TEXT("genesis.Conception.Cam"),
		TEXT("Kamera einstellen: <Abstand µm> [Brennweite mm] [Blende] [Makro-Faktor]. Für Bildmessreihen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->OocyteDistanceUm = FCString::Atof(*Args[0]);
					It->OrbitDistanceUm = FCString::Atof(*Args[0]);
				}
				if (Args.Num() > 1)
				{
					It->NominalFocalLengthMm = FCString::Atof(*Args[1]);
				}
				if (Args.Num() > 2)
				{
					It->Aperture = FCString::Atof(*Args[2]);
				}
				if (Args.Num() > 3)
				{
					It->MacroScale = FCString::Atof(*Args[3]);
				}
				It->ApplyOptics();
				UE_LOG(LogTemp, Display, TEXT("GENESIS Kamera: Abstand %.0f µm, Bildwinkel wie %.1f mm, Blende f/%.1f, Makro ×%.1f (echte Brennweite %.0f mm)"),
					It->OocyteDistanceUm, It->NominalFocalLengthMm, It->Aperture, It->MacroScale,
					It->Camera ? It->Camera->CurrentFocalLength : 0.0f);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisExposureCommand(
		TEXT("genesis.Conception.Exposure"),
		TEXT("Belichtungskorrektur aller Postprocess-Volumes setzen (EV). Für Belichtungsmessreihen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				return;
			}
			const float Bias = FCString::Atof(*Args[0]);
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				It->ExposureBias = Bias;
				UE_LOG(LogTemp, Display, TEXT("GENESIS Belichtung: %.1f EV"), Bias);
			}
			// Volumes mitziehen, damit kein zweiter Wert dagegenhält
			for (TActorIterator<APostProcessVolume> It(World); It; ++It)
			{
				It->Settings.bOverride_AutoExposureBias = true;
				It->Settings.AutoExposureBias = Bias;
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisLightCommand(
		TEXT("genesis.Conception.Light"),
		TEXT("Endoskoplicht beim Bezugsabstand in Candela setzen. Für Belichtungsmessreihen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (Args.Num() == 0)
			{
				return;
			}
			const float Candelas = FCString::Atof(*Args[0]);
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				It->LightCandelasAtReference = Candelas;
				if (It->EndoscopeLight && !It->bAutoLightControl)
				{
					It->EndoscopeLight->SetIntensity(Candelas);
				}
				UE_LOG(LogTemp, Display, TEXT("GENESIS Endoskoplicht: %.0f cd bei %.0f µm (Regelung %s)"),
					Candelas, It->LightReferenceDistanceUm, It->bAutoLightControl ? TEXT("an") : TEXT("aus"));
			}
		}));
}
#endif

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

	// Streulicht der Umgebung: Im Gewebe und in der Flüssigkeit wird Licht gestreut, deshalb ist es
	// neben dem Lichtkegel nie völlig schwarz. Ohne diese Schicht funktioniert nur der Arbeitsabstand,
	// für den das Endoskoplicht gerade eingestellt ist – weite Einstellungen fallen ins Dunkle.
	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Camera);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->bUseTemperature = true;
	// Wärmer als das Endoskoplicht: Was hier ankommt, ist durch Gewebe gegangen
	FillLight->SetTemperature(3200.0f);
	FillLight->SetSourceRadius(60.0f);
	FillLight->SetAttenuationRadius(6000.0f);
	FillLight->SetCastShadows(false);
	// Kaum Volumenstreuung: Sonst legt sich ein heller Schleier über das Bild, wie beim Endoskoplicht
	FillLight->SetVolumetricScatteringIntensity(0.12f);
	FillLight->SetIntensity(FillCandelas);
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

void AGenesisMicroscopeCameraRig::ApplyOptics()
{
	if (!Camera)
	{
		return;
	}

	// Sensor und Brennweite gemeinsam vergrößern: gleicher Bildwinkel, aber die Schärfentiefe einer Makro-Optik
	const float Scale = FMath::Max(1.0f, MacroScale);
	Camera->LensSettings.MaxFocalLength = FMath::Max(Camera->LensSettings.MaxFocalLength, NominalFocalLengthMm * Scale + 1.0f);
	Camera->Filmback.SensorWidth = 36.0f * Scale;
	Camera->Filmback.SensorHeight = 20.25f * Scale;
	Camera->SetCurrentFocalLength(NominalFocalLengthMm * Scale);
	Camera->SetCurrentAperture(Aperture);
	// Nahe Zellen sollen ausblenden, nicht aufgeschnitten werden: Die Nahgrenze liegt bei einem halben Mikrometer
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 0.5f;
}

void AGenesisMicroscopeCameraRig::UpdateLight()
{
	if (FillLight)
	{
		// Das Streulicht hängt nicht am Arbeitsabstand: Die Wand des Eileiters bleibt gleich weit weg.
		FillLight->SetIntensity(FillCandelas);
	}

	if (!EndoscopeLight || !bAutoLightControl)
	{
		return;
	}

	// Automatische Lichtregelung wie an einem Endoskop: Das Licht sitzt an der Optik, die Beleuchtungsstärke
	// fällt mit dem Quadrat des Arbeitsabstands. Ohne Regelung ist jede Nahaufnahme ausgebrannt
	// (gemessen: 40 % der Fläche reinweiß bei 95 µm Abstand mit der Einstellung für 430 µm) – und
	// jede weite Einstellung fällt ins Dunkle (gemessen bei 1.100 µm: praktisch schwarz).
	// Die Spanne ist deshalb weit: vom Zwanzigstel bis zum Zwanzigfachen der Bezugsstärke.
	const float Reference = FMath::Max(1.0f, LightReferenceDistanceUm) * GenesisMicroScale::UnitsPerMicrometer;
	const float Working = FMath::Max(1.0f, CurrentFocusDistance);
	// Zwei Bereiche, weil zwei Dinge im Bild sind:
	// Unterhalb des Bezugsabstands füllt das Motiv den Ausschnitt – dort gilt das Abstandsquadrat exakt,
	// und ohne es brennt jede Nahaufnahme aus (gemessen bei 110 µm mit linearer Regelung: Median 0,63).
	// Oberhalb sieht die Kamera vor allem die nahen Falten der Schleimhaut; würde das Licht weiter
	// quadratisch steigen, überstrahlten sie das Bild (gemessen bei 1.100 µm: Median 0,68).
	const float Ratio = Working / Reference;
	const float Exponent = Ratio < 1.0f ? LightFalloffExponentNear : LightFalloffExponentFar;
	const float Factor = FMath::Clamp(FMath::Pow(Ratio, Exponent), 0.05f, 20.0f);
	EndoscopeLight->SetIntensity(LightCandelasAtReference * Factor);
}

bool AGenesisMicroscopeCameraRig::ComputeOocyteView(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const
{

	const AGenesisOocyte* Egg = Swarm ? Swarm->GetOocyte() : nullptr;
	if (!Egg)
	{
		return false;
	}

	const FTransform& SwarmTransform = Swarm->GetActorTransform();
	const FVector Center = Egg->GetActorLocation();
	const FVector Axis = SwarmTransform.GetUnitAxis(EAxis::X);
	const FVector Side = SwarmTransform.GetUnitAxis(EAxis::Y);
	const FVector Up = SwarmTransform.GetUnitAxis(EAxis::Z);

	// Leicht gegen die Schwimmrichtung: Die ankommenden Zellen kommen der Kamera entgegen, statt ihr davonzulaufen
	// Überwiegend entlang der Kanalachse, denn quer ist im Lumen (Radius 450 µm) kein Platz für Abstand
	const FVector Direction = (-Axis * 1.15 + Side * FMath::Cos(OocyteOrbitPhase) * 0.45 + Up * FMath::Sin(OocyteOrbitPhase) * 0.30).GetSafeNormal();

	float Distance = OocyteDistanceUm * GenesisMicroScale::UnitsPerMicrometer;
	FVector Candidate = Center + Direction * Distance;
	// Die Kamera darf nicht in der Schleimhaut stehen: notfalls näher an die Eizelle heran
	const float Limit = (Swarm->GetChannel().LumenRadiusUm - 25.0f) * GenesisMicroScale::UnitsPerMicrometer;
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		const FVector Local = SwarmTransform.InverseTransformPosition(Candidate);
		if (FVector2D(Local.Y, Local.Z).Size() <= Limit)
		{
			break;
		}
		Distance *= 0.8f;
		Candidate = Center + Direction * Distance;
	}

	OutLocation = Candidate;
	OutRotation = FRotationMatrix::MakeFromXZ(Center - OutLocation, Up).ToQuat();
	OutFocusDistance = static_cast<float>(FVector::Dist(OutLocation, Center));
	return true;
}

bool AGenesisMicroscopeCameraRig::ComputeDesired(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const
{
	if (bWatchOocyte && ComputeOocyteView(OutLocation, OutRotation, OutFocusDistance))
	{
		return true;
	}

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
	ApplyOptics();
	UpdateBeatNormal(DeltaSeconds);
	OocyteOrbitPhase += DeltaSeconds * OocyteOrbitSpeed;

	// Sobald eine Zelle an der Zona hängt, gehört das Bild ihr: Die Kamera wechselt auf die Zelle,
	// die es geschafft hat, und begleitet sie beim Bohren durch die Zona. Von außen wäre davon nichts zu sehen –
	// gebundene Zellen stecken unter dem Zellkranz.
	if (bCloseUpOnBinding && Swarm && Swarm->GetOocyte())
	{
		const FGenesisOocyteState& Egg = Swarm->GetOocyte()->GetState();
		const int32 Attached = Swarm->FindAttachedCell();
		if (Egg.IsFertilized())
		{
			// Nach der Verschmelzung gehört das Bild der ganzen Eizelle: Dort läuft die Cortikalreaktion sichtbar ab.
			if (!bWatchOocyte)
			{
				bWatchOocyte = true;
				bInitialized = false;
			}
		}
		else if (Attached != INDEX_NONE && Attached != FollowCellIndex)
		{
			// Bis dahin begleitet die Kamera die Zelle, die es geschafft hat – durch den Zellkranz hindurch.
			// Ein Blick von außen auf die Eintrittsstelle ist anatomisch unmöglich: Der Cumulus ist dicht.
			FollowCellIndex = Attached;
			bWatchOocyte = false;
			bInitialized = false; // harter Schnitt statt Fahrt quer durch das Gewebe
		}
	}
	else if (bWatchFertilization && !bWatchOocyte)
	{
		if (const AGenesisOocyte* Egg = Swarm ? Swarm->GetOocyte() : nullptr)
		{
			bWatchOocyte = Egg->GetState().BoundCells > 0 || Egg->GetState().IsFertilized();
		}
	}

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
		// Fokus auf den tatsächlichen Abstand der geglätteten Kamera zum Motiv (Zellkopf oder Eizelle)
		const AGenesisOocyte* Egg = bWatchOocyte && Swarm ? Swarm->GetOocyte() : nullptr;
		const FVector FocusTarget = Egg ? Egg->GetActorLocation() : Swarm->GetCellHeadWorldPosition(FollowCellIndex);
		const float ActualDistance = static_cast<float>(FVector::Dist(GetActorLocation(), FocusTarget));
		CurrentFocusDistance = FMath::Lerp(CurrentFocusDistance, ActualDistance, FocusAlpha);
	}

	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = CurrentFocusDistance;
	Camera->SetFocusSettings(Focus);
	UpdateLight();

	// Die Kamera belichtet wie eine echte Kamera: feste Belichtung, Blende und Verschlusszeit wirken auf die Helligkeit.
	// Die Korrektur gleicht den Maßstabssprung aus – im Mikrometerraum trifft die Optik nur wenige Lux.
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias;

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
