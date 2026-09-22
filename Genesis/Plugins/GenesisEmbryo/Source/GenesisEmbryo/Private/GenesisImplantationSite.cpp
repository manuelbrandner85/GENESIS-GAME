// GENESIS: Der Kreislauf des Lebens

#include "GenesisImplantationSite.h"
#include "CineCameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenesisEmbryoActor.h"
#include "GenesisEmbryoLogic.h"
#include "GenesisEmbryoSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** Entwickler: Belichtung und Licht der Hysteroskop-Kamera im laufenden Spiel einstellen und im Bild prüfen. */
	FAutoConsoleCommandWithWorldAndArgs GenesisImplantationExposureCommand(
		TEXT("genesis.Implantation.Exposure"),
		TEXT("Belichtung der Hysteroskop-Kamera (EV): genesis.Implantation.Exposure <bias> [lux]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisImplantationSite> It(World); It; ++It)
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
}

namespace GenesisImplantationView
{
	FGenesisImplantationView Compute(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
	{
		FGenesisImplantationView View;
		const FGenesisImplantationState& Nid = State.Nidation;
		// Geschlüpfte Blastozyste ~0,2 mm; danach wächst der Keim nach der Simulation
		View.RadiusUm = 0.5f * FMath::Max(200.0f, Nid.ConceptusDiameterUm);
		const float R = View.RadiusUm;
		const float T = GenesisEmbryoLogic::NominalImplantationHours(State, Tuning);

		if (Nid.AppositionAtHours <= 0.0f || State.Stage < EGenesisEmbryoStage::Implanting)
		{
			// Noch nicht geschlüpft: frei in der Flüssigkeit über der Schleimhaut
			View.CenterHeightUm = R + 90.0f;
		}
		else if (Nid.Phase == EGenesisImplantationPhase::None)
		{
			// Geschlüpft sinkt der Keim in einigen Stunden auf die Schleimhaut (Dichte etwas über der Flüssigkeit)
			const float Hatched = Nid.AppositionAtHours - Tuning.FloatAfterHatchingHours;
			const float Settle = FMath::Clamp((static_cast<float>(State.HoursSinceFusion) - Hatched) / FMath::Max(0.1f, Tuning.FloatAfterHatchingHours), 0.0f, 1.0f);
			View.CenterHeightUm = R + 90.0f * (1.0f - FMath::SmoothStep(0.0f, 1.0f, Settle));
		}
		else
		{
			// Aufliegen, dann versinken: Bei Embedded 1 liegt der obere Pol auf Höhe der Oberfläche,
			// danach wandert der Keim mit dem Wachsen der Schleimhaut noch etwas tiefer
			const float Deeper = FMath::Max(0.0f, Nid.DepthUm - Nid.ConceptusDiameterUm);
			View.CenterHeightUm = R - 2.0f * R * Nid.Embedded - Deeper - 2.0f;
		}

		// Drehung: Beim Anlagern wendet sich der Pol mit dem Embryoblasten der Schleimhaut zu
		View.Orientation = T > 0.0f ? FMath::SmoothStep(Tuning.NominalAppositionHours - 2.0f, Tuning.AdhesionHours, T) : 0.0f;

		const float H = View.CenterHeightUm;
		View.WaterlineRadiusUm = FMath::Abs(H) < R ? FMath::Sqrt(FMath::Max(0.0f, R * R - H * H)) : 0.0f;
		View.bEmbryoVisible = H + R > -1.0f;

		// Das Epithel umschließt den einsinkenden Keim mit einem Wulst; ist er ganz unten, schließt sich der Wulst zum Defekt
		const bool bAttached = Nid.Phase >= EGenesisImplantationPhase::Adhesion;
		View.Collar = bAttached ? 1.0f - FMath::SmoothStep(0.92f, 1.0f, Nid.Embedded) : 0.0f;

		// Defekt über dem versunkenen Keim: erst offen, dann Fibrinpfropf, dann wächst das Epithel von den Rändern darüber
		const float Opening = FMath::SmoothStep(0.8f, 0.97f, Nid.Embedded);
		View.PlugRadiusUm = Opening * 0.55f * FMath::Min(R, 230.0f) * (1.0f - FMath::SmoothStep(0.5f, 1.0f, Nid.SurfaceClosure));
		// Das letzte Stück des Keims über der Oberfläche liegt unter dem Pfropf – sonst sah man die glatte Hülle als Scheibe
		View.bEmbryoVisible = View.bEmbryoVisible && View.PlugRadiusUm < 1.0f;

		// Der wachsende Keim wölbt die Schleimhaut leicht vor (Hertig-Rock, Tag 12: „leicht erhabene Stelle")
		View.DomeHeightUm = 0.07f * R * (T > 0.0f ? FMath::SmoothStep(Tuning.EmbeddedHours - 12.0f, Tuning.PrimaryVilliHours, T) : 0.0f);

		// Mütterliches Blut in den Lakunen scheint durch das dünne Gewebe über dem Keim
		View.BloodShowing = 0.85f * Nid.LacunarBlood;
		View.Hyperemia = Nid.Decidualization;

		// Bildbreite: anfangs der Keim mit etwas Umgebung, später die ganze vorgewölbte Stelle
		View.FramingWidthUm = FMath::Max(900.0f, 4.2f * R);
		return View;
	}
}

AGenesisImplantationSite::AGenesisImplantationSite()
{
	PrimaryActorTick.bCanEverTick = true;
	// Nach dem Keim-Actor: Der setzt Zellen und Material, hier werden Lage und Größe bestimmt
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	Conceptus = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Conceptus"));
	Conceptus->SetupAttachment(Root);
	Conceptus->SetUsingAbsoluteLocation(true);
	Conceptus->SetUsingAbsoluteRotation(true);
	Conceptus->SetUsingAbsoluteScale(true);
	Conceptus->SetMobility(EComponentMobility::Movable);
	Conceptus->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InnerCellMass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerCellMass"));
	InnerCellMass->SetupAttachment(Conceptus);
	InnerCellMass->SetMobility(EComponentMobility::Movable);
	InnerCellMass->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Linsenförmiger Knoten innen an der Hülle, am Embryonalpol (lokal −Z), gut ein Drittel des Keimdurchmessers
	InnerCellMass->SetRelativeLocation(FVector(0.0, 0.0, -38.0));
	InnerCellMass->SetRelativeScale3D(FVector(0.36, 0.36, 0.22));

	EmptyZona = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmptyZona"));
	EmptyZona->SetupAttachment(Root);
	EmptyZona->SetMobility(EComponentMobility::Movable);
	EmptyZona->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Eingefallen und flach liegend: Die leere Hülle trägt sich nicht selbst (Zona 58 µm innen, jetzt ~14 µm dünn gedehnt)
	EmptyZona->SetRelativeLocation(FVector(250.0, -140.0, 30.0));
	EmptyZona->SetRelativeRotation(FRotator(40.0, 35.0, 25.0));
	EmptyZona->SetRelativeScale3D(FVector(1.1, 1.0, 0.78));

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetUsingAbsoluteLocation(true);
	Camera->SetUsingAbsoluteRotation(true);

	ScopeLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("ScopeLight"));
	ScopeLight->SetupAttachment(Camera);
	// Lichtleiter als Ring von Glasfasern um die Optik, Kaltlicht-LED ~5.600 K: fast achsnahes, weiches Licht mit
	// kleinen Schatten – so sehen Hysteroskopbilder aus. Ein Hauch außerhalb der Achse, damit Wölbungen lesbar bleiben.
	ScopeLight->SetRelativeLocation(FVector(0.0, 18.0, -12.0));
	ScopeLight->SetMobility(EComponentMobility::Movable);
	ScopeLight->bUseTemperature = true;
	ScopeLight->SetTemperature(5600.0f);
	ScopeLight->SetInnerConeAngle(30.0f);
	ScopeLight->SetOuterConeAngle(58.0f);
	ScopeLight->SetSourceRadius(45.0f);
	ScopeLight->SetAttenuationRadius(20000.0f);
	ScopeLight->SetCastShadows(true);
	ScopeLight->SetIntensityUnits(ELightUnits::Candelas);
	ScopeLight->SetVolumetricScatteringIntensity(0.3f);
}

void AGenesisImplantationSite::BeginPlay()
{
	Super::BeginPlay();
	bCameraInitialized = false;
	OrbitDegrees = 0.0f;

	// Hysteroskop: Weitwinkel mit großer Schärfentiefe – die ganze Stelle ist scharf, wie in echten Aufnahmen
	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->bConstrainAspectRatio = false;
	Camera->SetCurrentFocalLength(35.0f);
	Camera->SetCurrentAperture(11.0f);
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 2.0f;
	// Wie unter dem Mikroskop: MegaLights rechnet eine einzelne Leuchte nicht besser, nur anders
	Camera->PostProcessSettings.bOverride_bMegaLights = true;
	Camera->PostProcessSettings.bMegaLights = false;
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias;

	if (Embryo)
	{
		Embryo->bMicroscopeView = false;
	}
}

void AGenesisImplantationSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisEmbryoSubsystem* EmbryoSystem = GameInstance ? GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>() : nullptr;
	if (EmbryoSystem && EmbryoSystem->HasEmbryo())
	{
		const FGenesisEmbryoState& State = EmbryoSystem->GetState();
		View = GenesisImplantationView::Compute(State, EmbryoSystem->Tuning);
		PlaceEmbryo(State, DeltaSeconds);
	}
	PushParameters();

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
}

void AGenesisImplantationSite::PlaceZona(const FGenesisEmbryoState& State)
{
	if (!EmptyZona)
	{
		return;
	}
	// Die leere Zona zerfällt in den Tagen nach dem Schlüpfen (Proteasen der Gebärmutter); nach der Anheftung ist sie fort
	const float Hatched = State.Nidation.AppositionAtHours > 0.0f ? State.Nidation.AppositionAtHours : 1.0e9f;
	const float Age = static_cast<float>(State.HoursSinceFusion) - Hatched;
	EmptyZona->SetVisibility(State.Stage >= EGenesisEmbryoStage::Implanting && Age < 30.0f);
}

void AGenesisImplantationSite::PlaceEmbryo(const FGenesisEmbryoState& State, float DeltaSeconds)
{
	PlaceZona(State);

	// Der Keim als Hülle: Größe, Lage und das Einsinken
	if (Conceptus && Conceptus->GetStaticMesh())
	{
		constexpr float MeshRadiusUm = 55.0f;
		Conceptus->SetVisibility(View.bEmbryoVisible);
		Conceptus->SetWorldScale3D(FVector(View.RadiusUm / MeshRadiusUm));
		Conceptus->SetWorldLocation(GetActorLocation() + FVector(0.0, 0.0, View.CenterHeightUm));
		// Frei treibend liegt der Embryoblast irgendwo – hier seitlich, durch die Hülle zu sehen. Beim Anlagern dreht
		// sich der Keim, bis dieser Pol unten an der Schleimhaut liegt. Angeheftet steht er still.
		const float Seconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		const float Spin = (1.0f - View.Orientation) * 3.0f * Seconds;
		Conceptus->SetWorldRotation(FRotator(FMath::Lerp(105.0f, 0.0f, View.Orientation), 20.0f + Spin, 0.0f));
	}
	if (!Embryo)
	{
		return;
	}
	// Die Einzelzellen der Simulation gehören unter das Mikroskop der ersten Woche
	if (Conceptus && Conceptus->GetStaticMesh())
	{
		Embryo->SetActorHiddenInGame(true);
		return;
	}

	Embryo->SetActorHiddenInGame(!View.bEmbryoVisible);
	const float CellRadius = FMath::Max(1.0f, State.GetOuterRadiusUm());
	Embryo->SetActorScale3D(FVector(View.RadiusUm / CellRadius));
	Embryo->SetActorLocation(GetActorLocation() + FVector(0.0, 0.0, View.CenterHeightUm));

	// Richtung des Embryoblasten im Keim: Dieser Pol legt sich an die Schleimhaut
	FVector Inner = FVector::ZeroVector;
	for (const FGenesisBlastomere& Cell : State.Cells)
	{
		Inner += Cell.bInnerCellMass ? Cell.Position : FVector::ZeroVector;
	}
	const FVector Pole = Inner.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
	const FQuat Attached = FQuat::FindBetweenNormals(Pole, -FVector::UpVector);

	// Frei treibend dreht sich der Keim langsam in der Strömung der Uterusflüssigkeit
	const float Seconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FQuat Drift = FQuat(FVector(0.3, 1.0, 0.2).GetSafeNormal(), FMath::DegreesToRadians(4.0f * Seconds)) * FQuat(FVector::UpVector, 0.6);
	Embryo->SetActorRotation(FQuat::Slerp(Drift, Attached, View.Orientation));
}

void AGenesisImplantationSite::PushParameters()
{
	UWorld* World = GetWorld();
	UMaterialParameterCollectionInstance* Instance = (World && Parameters) ? World->GetParameterCollectionInstance(Parameters) : nullptr;
	if (!Instance)
	{
		return;
	}
	const FVector Site = GetActorLocation();
	Instance->SetVectorParameterValue(TEXT("Site"), FLinearColor(Site.X, Site.Y, Site.Z, 0.0f));
	Instance->SetScalarParameterValue(TEXT("EmbryoRadius"), View.RadiusUm);
	Instance->SetScalarParameterValue(TEXT("CenterHeight"), View.CenterHeightUm);
	Instance->SetScalarParameterValue(TEXT("Waterline"), View.WaterlineRadiusUm);
	Instance->SetScalarParameterValue(TEXT("Collar"), View.Collar);
	Instance->SetScalarParameterValue(TEXT("PlugRadius"), View.PlugRadiusUm);
	Instance->SetScalarParameterValue(TEXT("DomeHeight"), View.DomeHeightUm);
	Instance->SetScalarParameterValue(TEXT("Blood"), View.BloodShowing);
	Instance->SetScalarParameterValue(TEXT("Hyperemia"), View.Hyperemia);
}

void AGenesisImplantationSite::UpdateCamera(float DeltaSeconds, bool bSnap)
{
	if (!Camera)
	{
		return;
	}
	OrbitDegrees += OrbitDegreesPerSecond * DeltaSeconds;

	const float HalfFov = FMath::Atan(0.5f * Camera->Filmback.SensorWidth / FMath::Max(1.0f, Camera->CurrentFocalLength));
	const float Distance = View.FramingWidthUm / (2.0f * FMath::Tan(HalfFov));
	const FVector Focus = GetActorLocation() + FVector(0.0, 0.0, FMath::Max(0.0f, View.CenterHeightUm) * 0.6f + View.DomeHeightUm);
	const float Azimuth = FMath::DegreesToRadians(AzimuthDegrees + OrbitDegrees);
	const float Elevation = FMath::DegreesToRadians(ElevationDegrees);
	const FVector Direction(FMath::Cos(Elevation) * FMath::Cos(Azimuth), FMath::Cos(Elevation) * FMath::Sin(Azimuth), FMath::Sin(Elevation));
	const FVector Desired = Focus + Direction * Distance;

	// Gedämpft folgen, nicht springen: Die Optik wird von einer Hand geführt
	const float Alpha = bSnap ? 1.0f : 1.0f - FMath::Exp(-DeltaSeconds / FMath::Max(0.05f, FollowSeconds));
	CameraLocation = FMath::Lerp(CameraLocation, Desired, Alpha);
	CameraFocus = FMath::Lerp(CameraFocus, Focus, Alpha);
	Camera->SetWorldLocationAndRotation(CameraLocation, (CameraFocus - CameraLocation).Rotation());

	const float FocusDistance = static_cast<float>(FVector::Dist(CameraLocation, CameraFocus));
	FCameraFocusSettings Settings = Camera->FocusSettings;
	Settings.FocusMethod = ECameraFocusMethod::Manual;
	Settings.ManualFocusDistance = FocusDistance;
	Camera->SetFocusSettings(Settings);

	// Das Licht regelt nach: gleiche Beleuchtungsstärke am Keim bei jedem Abstand (E = I / d², d in m; 1 µm = 1 cm)
	if (ScopeLight)
	{
		const float Meters = FocusDistance / 100.0f;
		ScopeLight->SetIntensity(TargetIlluminanceLux * Meters * Meters);
	}
}
