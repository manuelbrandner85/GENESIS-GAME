// GENESIS: Der Kreislauf des Lebens

#include "GenesisMicroscopeCameraRig.h"
#include "CineCameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisFertilizationLogic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
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

	FAutoConsoleCommandWithWorldAndArgs GenesisWatchEmbryoCommand(
		TEXT("genesis.Conception.WatchEmbryo"),
		TEXT("Kamera sofort auf den Keim, mit der Optik der ersten Woche (für Bildprüfungen mit genesis.Embryo.Start)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				It->DebugWatchEmbryo();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisEgoViewCommand(
		TEXT("genesis.Race.EgoView"),
		TEXT("Rennen: halbe Ich-Perspektive (1) oder Verfolgeransicht (0). Optional: <Abstand µm> <Höhe °> <Vorausblick µm> <Seite °> <Brennweite mm>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				const bool bEgo = Args.Num() == 0 || Args[0] != TEXT("0");
				if (It->IsRaceEgoView() != bEgo)
				{
					It->ToggleRaceView();
				}
				float* Targets[] = { &It->RaceEgoDistanceUm, &It->RaceEgoElevationDegrees, &It->RaceEgoLookAheadUm, &It->RaceEgoAzimuthDegrees, &It->RaceEgoFocalLengthMm };
				for (int32 Index = 1; Index < Args.Num() && Index <= UE_ARRAY_COUNT(Targets); ++Index)
				{
					*Targets[Index - 1] = FCString::Atof(*Args[Index]);
				}
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

	FAutoConsoleCommandWithWorldAndArgs GenesisProfileCommand(
		TEXT("genesis.Conception.Profile"),
		TEXT("Profilansicht an der Eizelle: <Abstand µm> [Schichtdicke µm]. Für Bildprüfungen."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisMicroscopeCameraRig> It(World); It; ++It)
			{
				if (Args.Num() > 0)
				{
					It->ProfileDistanceUm = FCString::Atof(*Args[0]);
				}
				if (Args.Num() > 1)
				{
					It->SectionThicknessUm = FCString::Atof(*Args[1]);
				}
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

	// Im Rennen, solange die Kamera der eigenen Zelle folgt: mehr Schärfentiefe
	const bool bFollowingPlayer = Swarm && Swarm->IsRacing() && !bWatchOocyte && FollowCellIndex == Swarm->GetPlayerCellIndex();
	// Nach der Verschmelzung zeigt die Kamera den Keim wie ein Zeitraffer-Brutschrank: der ganze Keim scharf
	const bool bWatchingEmbryo = SecondsSinceFusion >= 0.0f && bWatchOocyte;
	// Profilansicht an der Eizelle (GENESIS-047 Teil 2b): eigene Optik, sonst verschwimmt die 3 µm dünne Zelle ganz
	const bool bProfile = SectionWeight > 0.5f && !bWatchingEmbryo;
	const float EffectiveAperture = bProfile ? ProfileAperture : (bFollowingPlayer ? RaceAperture : (bWatchingEmbryo ? EmbryoAperture : Aperture));

	// Sensor und Brennweite gemeinsam vergrößern: gleicher Bildwinkel, aber die Schärfentiefe einer Makro-Optik
	const bool bEgo = bFollowingPlayer && bRaceEgoView;
	const float Scale = bEgo ? FMath::Max(0.1f, RaceEgoMacroScale)
		: FMath::Max(1.0f, bProfile ? ProfileMacroScale : (bFollowingPlayer ? RaceMacroScale : (bWatchingEmbryo ? EmbryoMacroScale : MacroScale)));
	const float Focal = (bEgo ? RaceEgoFocalLengthMm : NominalFocalLengthMm) * Scale;
	Camera->LensSettings.MinFocalLength = FMath::Min(Camera->LensSettings.MinFocalLength, Focal - 0.1f);
	Camera->LensSettings.MaxFocalLength = FMath::Max(Camera->LensSettings.MaxFocalLength, Focal + 1.0f);
	Camera->Filmback.SensorWidth = 36.0f * Scale;
	Camera->Filmback.SensorHeight = 20.25f * Scale;
	Camera->SetCurrentFocalLength(Focal);
	Camera->SetCurrentAperture(EffectiveAperture);
	// Nahe Zellen sollen ausblenden, nicht aufgeschnitten werden: Die Nahgrenze liegt bei einem halben Mikrometer
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 0.5f;
}

bool AGenesisMicroscopeCameraRig::IsShowingEgoView() const
{
	return bRaceEgoView && Swarm && Swarm->IsRacing() && !bWatchOocyte && FollowCellIndex == Swarm->GetPlayerCellIndex();
}

void AGenesisMicroscopeCameraRig::UpdateLight()
{
	// Ich-Perspektive: Das Licht sitzt nicht in der Linse, sondern ein Stück dahinter und darüber. Direkt
	// an der Optik träfe es die eigene Geißel aus 4 µm, den Kopf aus 14 – die Geißel wäre 13-mal heller
	// und brennt weiß aus (gesehen im ersten Test). Aus 40 µm Abstand sind beide fast gleich hell.
	const bool bEgo = IsShowingEgoView();
	if (EndoscopeLight)
	{
		EndoscopeLight->SetRelativeLocation(bEgo ? FVector(-RaceEgoLightBackUm, 1.5, RaceEgoLightBackUm * 0.5) : FVector(0.0, 1.5, -1.0));
	}

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
	// Die Spanne ist deshalb weit: vom Hundertstel bis zum Zwanzigfachen der Bezugsstärke. Die Untergrenze lag bei einem
	// Zwanzigstel – aus 65 µm (Profilansicht an der Zona, GENESIS-047 Teil 2b) war das Bild dann doppelt überbelichtet.
	const float Reference = FMath::Max(1.0f, LightReferenceDistanceUm) * GenesisMicroScale::UnitsPerMicrometer;
	const float Working = FMath::Max(1.0f, bEgo ? CurrentFocusDistance + RaceEgoLightBackUm * GenesisMicroScale::UnitsPerMicrometer : CurrentFocusDistance);
	// Zwei Bereiche, weil zwei Dinge im Bild sind:
	// Unterhalb des Bezugsabstands füllt das Motiv den Ausschnitt – dort gilt das Abstandsquadrat exakt,
	// und ohne es brennt jede Nahaufnahme aus (gemessen bei 110 µm mit linearer Regelung: Median 0,63).
	// Oberhalb sieht die Kamera vor allem die nahen Falten der Schleimhaut; würde das Licht weiter
	// quadratisch steigen, überstrahlten sie das Bild (gemessen bei 1.100 µm: Median 0,68).
	const float Ratio = Working / Reference;
	const float Exponent = Ratio < 1.0f ? LightFalloffExponentNear : LightFalloffExponentFar;
	const float Factor = FMath::Clamp(FMath::Pow(Ratio, Exponent), 0.01f, 20.0f);
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
	// Überwiegend entlang der Kanalachse, denn quer ist im Lumen (Radius 900 µm) kein Platz für Abstand
	FVector Direction = (-Axis * 1.15 + Side * FMath::Cos(OocyteOrbitPhase) * 0.45 + Up * FMath::Sin(OocyteOrbitPhase) * 0.30).GetSafeNormal();

	// Während gebohrt wird: Blick auf die Eintrittsstelle der führenden Zelle, langsam heran
	const float PushIn = SecondsSinceBinding >= 0.0f ? FMath::SmoothStep(0.0f, 1.0f, SecondsSinceBinding / FMath::Max(1.0f, PushInSeconds)) : 0.0f;
	const float PullBack = SecondsSinceFusion >= 0.0f ? FMath::SmoothStep(0.0f, 1.0f, SecondsSinceFusion / FMath::Max(0.5f, PullBackSeconds)) : 0.0f;
	const float Approach = PushIn * (1.0f - PullBack);
	if (!SmoothedLeaderDirection.IsNearlyZero())
	{
		// Nicht ganz frontal: etwas gegen die Kanalachse versetzt, damit die Kamera im Lumen bleibt
		const FVector Toward = (SmoothedLeaderDirection - Axis * 0.35).GetSafeNormal();
		Direction = FMath::Lerp(Direction, Toward, 0.85f * Approach).GetSafeNormal();
	}

	// Der Spieler führt das Mikroskop: um die Hochachse und auf und ab
	const float PlayerPitch = FMath::Clamp(PlayerOrbitDegrees.Y, -MaxPlayerPitchDegrees, MaxPlayerPitchDegrees);
	Direction = FQuat(Up, FMath::DegreesToRadians(PlayerOrbitDegrees.X)).RotateVector(Direction);
	const FVector PitchAxis = FVector::CrossProduct(Up, Direction).GetSafeNormal();
	if (!PitchAxis.IsNearlyZero())
	{
		Direction = FQuat(PitchAxis, FMath::DegreesToRadians(-PlayerPitch)).RotateVector(Direction);
	}

	// Nach der Verschmelzung geht die Kamera auf den Keim zurück, nicht auf den ganzen Cumulus: Der löst
	// sich in den nächsten Stunden auf, übrig bleibt eine Zelle von 110 µm in ihrer Hülle
	const float Resting = SecondsSinceFusion >= 0.0f ? EmbryoViewDistanceUm : OocyteDistanceUm;
	float Distance = FMath::Lerp(Resting, PushInDistanceUm, Approach) * GenesisMicroScale::UnitsPerMicrometer;
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
	// Blickpunkt: die Eizellmitte – beim Heranfahren wandert er zur Oberfläche des Cumulus, wo die führende
	// Zelle steckt. Dorthin geht auch die Schärfe: Die schlagenden Schwänze sind das Motiv, nicht die Mitte.
	FVector LookAt = Center;
	if (!SmoothedLeaderDirection.IsNearlyZero())
	{
		const FVector Surface = Center + SmoothedLeaderDirection * Egg->GetState().CoronaRadiusUm * GenesisMicroScale::UnitsPerMicrometer;
		LookAt = FMath::Lerp(Center, Surface, 0.6f * Approach);
	}
	const float SurfaceDistance = static_cast<float>(FVector::Dist(OutLocation, Center)) - Egg->GetState().CoronaRadiusUm * GenesisMicroScale::UnitsPerMicrometer;
	OutFocusDistance = FMath::Lerp(static_cast<float>(FVector::Dist(OutLocation, Center)), FMath::Max(10.0f, SurfaceDistance), Approach);

	// Profilansicht (GENESIS-047 Teil 2b): Hängt die eigene Zelle an der Eizelle, blickt die Kamera quer zur Radialen –
	// die Zelle liegt dann auf dem Umriss der Eizelle, die Zona ist ein Band im Querschnitt, dahinter bleibt es dunkel.
	// So zeigen Mikroskopaufnahmen ein Spermium in der Zona. Von vorn lag die Zelle hell vor dem hellen Zellleib
	// und war nicht zu sehen.
	if (SectionWeight > 0.001f && Swarm->IsRacing() && Swarm->GetCell(Swarm->GetPlayerCellIndex()))
	{
		const FVector Head = Swarm->GetCellHeadWorldPosition(Swarm->GetPlayerCellIndex());
		const FVector Radial = (Head - Center).GetSafeNormal(UE_SMALL_NUMBER, Axis);
		// Die Zelle liegt schräg bis flach an der Oberfläche. Blickte die Kamera entlang ihrer Längsachse, sähe man
		// nur einen Punkt – genau das geschah zuerst (die Zelle war im Bild nicht zu finden). Deshalb quer zu ihr:
		// Sie liegt dann ganz in der Bildebene, Kopf, Mittelstück und Geißel nebeneinander.
		const FVector Heading = SwarmTransform.TransformVectorNoScale(Swarm->GetCell(Swarm->GetPlayerCellIndex())->Heading);
		const FVector Along = (Heading - Radial * FVector::DotProduct(Heading, Radial)).GetSafeNormal(UE_SMALL_NUMBER, FVector::CrossProduct(Radial, Up));
		const FVector A = FVector::CrossProduct(Radial, Along).GetSafeNormal(UE_SMALL_NUMBER, Side);
		const FVector B = FVector::CrossProduct(Radial, A).GetSafeNormal();
		const float Profile = ProfileDistanceUm * GenesisMicroScale::UnitsPerMicrometer;
		auto Lateral = [&SwarmTransform](const FVector& World)
		{
			const FVector Local = SwarmTransform.InverseTransformPosition(World);
			return FVector2D(Local.Y, Local.Z).Size();
		};
		// Die Seite behalten, solange die Kamera dort im Lumen bleibt – sonst die, die der Kanalmitte am nächsten ist
		const bool bKeep = !ProfileDirection.IsNearlyZero() && FMath::Abs(FVector::DotProduct(ProfileDirection, A)) > 0.8
			&& Lateral(Head + ProfileDirection * Profile) <= Limit;
		if (!bKeep)
		{
			// Quer zur Zelle, von der Seite, die näher an der Kanalmitte liegt. Nur wenn beide nicht ins Lumen passen,
			// entlang der Zelle (dann ist sie verkürzt zu sehen, aber die Kamera steht nicht in der Schleimhaut)
			ProfileDirection = Lateral(Head + A * Profile) <= Lateral(Head - A * Profile) ? A : -A;
			if (Lateral(Head + ProfileDirection * Profile) > Limit)
			{
				ProfileDirection = Lateral(Head + B * Profile) <= Lateral(Head - B * Profile) ? B : -B;
			}
		}
		// Quer zur Radialen nachführen, wenn die Zelle weiterwandert
		ProfileDirection = (ProfileDirection - Radial * FVector::DotProduct(ProfileDirection, Radial)).GetSafeNormal(UE_SMALL_NUMBER, A);
		const FVector ProfileLocation = Head + ProfileDirection * Profile;
		OutLocation = FMath::Lerp(OutLocation, ProfileLocation, SectionWeight);
		LookAt = FMath::Lerp(LookAt, Head, SectionWeight);
		OutFocusDistance = FMath::Lerp(OutFocusDistance, static_cast<float>(FVector::Dist(OutLocation, Head)), SectionWeight);
	}
	OutRotation = FRotationMatrix::MakeFromXZ(LookAt - OutLocation, Up).ToQuat();
	// Im Rennen steckt die eigene Zelle in der Zona oder liegt im Spalt darunter – 40 µm unter dem Rand des
	// Cumulus. Die Schärfe gehört dorthin; auf dem Cumulusrand lag sie im Leeren, und das ganze Bild war
	// weich, solange der Zeitraffer lief (gesehen in GENESIS-047).
	if (Swarm->IsRacing() && Swarm->GetCell(Swarm->GetPlayerCellIndex())
		&& GenesisFertilizationLogic::IsAttached(*Swarm->GetCell(Swarm->GetPlayerCellIndex())))
	{
		const float PlayerDistance = static_cast<float>(FVector::Dist(OutLocation, Swarm->GetCellHeadWorldPosition(Swarm->GetPlayerCellIndex())));
		OutFocusDistance = FMath::Lerp(OutFocusDistance, PlayerDistance, Approach);
	}
	return true;
}

void AGenesisMicroscopeCameraRig::UpdateSection(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	UMaterialParameterCollectionInstance* Instance = (World && SectionParameters) ? World->GetParameterCollectionInstance(SectionParameters) : nullptr;
	if (!Instance)
	{
		return;
	}
	// Nur im Rennen, solange die eigene Zelle an der Eizelle hängt und noch nichts entschieden ist. Nach der
	// Verschmelzung fährt die Kamera zurück auf den ganzen Keim – dann ist der Kranz wieder ganz zu sehen.
	const FGenesisSpermCell* Mine = Swarm && Swarm->IsRacing() ? Swarm->GetCell(Swarm->GetPlayerCellIndex()) : nullptr;
	const bool bWanted = bWatchOocyte && Mine && Swarm->GetRaceOutcome() == EGenesisRaceOutcome::Running
		&& GenesisFertilizationLogic::IsAttached(*Mine);
	SectionWeight = FMath::FInterpConstantTo(SectionWeight, bWanted ? 1.0f : 0.0f, DeltaSeconds, 1.0f / 1.5f);

	float Front = -1000000.0f;
	float Back = 1000000.0f;
	if (SectionWeight > 0.001f && Camera)
	{
		// Tiefe des eigenen Kopfes entlang der Blickachse – so misst auch das Material (Pixeltiefe)
		const FVector Head = Swarm->GetCellHeadWorldPosition(Swarm->GetPlayerCellIndex());
		const float HeadDepth = static_cast<float>(FVector::DotProduct(Head - Camera->GetComponentLocation(), Camera->GetForwardVector()));
		Front = SectionWeight * FMath::Max(0.0f, HeadDepth - SectionMarginUm);
		// Hinten schließt die Schicht auch: Dahinter wird es dunkel, und die Zelle am Rand der Eizelle hebt sich ab
		Back = HeadDepth + SectionThicknessUm + (1.0f - SectionWeight) * 100000.0f;
	}
	Instance->SetScalarParameterValue(TEXT("SectionFrontUm"), Front);
	Instance->SetScalarParameterValue(TEXT("SectionBackUm"), Back);
	Instance->SetScalarParameterValue(TEXT("SectionActive"), SectionWeight);
}

FVector AGenesisMicroscopeCameraRig::AvoidCumulus(const FVector& Subject, const FVector& Desired) const
{
	const AGenesisOocyte* Egg = Swarm ? Swarm->GetOocyte() : nullptr;
	const FGenesisCumulusField* Field = Egg ? Egg->GetState().Cumulus.Get() : nullptr;
	if (!Field)
	{
		return Desired;
	}
	// Vom Motiv zur Kamera: die erste Zelle, die den Weg schneidet, bestimmt, wie weit die Kamera zurück darf.
	// Kugeltest mit der längsten Halbachse und 2 µm Luft – lieber etwas zu nah als eine Zelle im Bild.
	const FVector Center = Egg->GetActorLocation();
	const FVector Ray = Desired - Subject;
	const double Length = Ray.Size();
	if (Length < UE_KINDA_SMALL_NUMBER)
	{
		return Desired;
	}
	const FVector Direction = Ray / Length;
	double Allowed = Length;
	TSet<int32> Seen;
	for (double Along = 0.0; Along <= Length + Field->CellSizeUm; Along += 0.5 * Field->CellSizeUm)
	{
		const FIntVector Key = Field->Key((Subject + Direction * FMath::Min(Along, Length)) - Center);
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				for (int32 Z = -1; Z <= 1; ++Z)
				{
					const TArray<int32>* Bucket = Field->Grid.Find(Key + FIntVector(X, Y, Z));
					if (!Bucket)
					{
						continue;
					}
					for (const int32 Index : *Bucket)
					{
						bool bAlready = false;
						Seen.Add(Index, &bAlready);
						if (bAlready)
						{
							continue;
						}
						const FGenesisCumulusCell& Cell = Field->Cells[Index];
						const double Radius = Cell.HalfAxes.GetMax() + 2.0;
						const FVector ToCell = (Center + Cell.Center) - Subject;
						const double Projection = FVector::DotProduct(ToCell, Direction);
						const double Miss = (ToCell - Direction * Projection).SizeSquared();
						if (Projection < 0.0 || Miss > Radius * Radius)
						{
							continue;
						}
						const double Entry = Projection - FMath::Sqrt(Radius * Radius - Miss);
						Allowed = FMath::Min(Allowed, Entry);
					}
				}
			}
		}
	}
	// Auch seitlich darf keine Zelle an der Linse kleben: Eine Zelle 3 µm vor dem Glas füllt das halbe Bild.
	// Die Kamera rückt so lange zum Motiv, bis 6 µm Luft zu jeder Membran bleiben.
	constexpr double Clearance = 6.0;
	auto Crowded = [Field, &Center](const FVector& Where)
	{
		const FIntVector Key = Field->Key(Where - Center);
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				for (int32 Z = -1; Z <= 1; ++Z)
				{
					if (const TArray<int32>* Bucket = Field->Grid.Find(Key + FIntVector(X, Y, Z)))
					{
						for (const int32 Index : *Bucket)
						{
							const FGenesisCumulusCell& Cell = Field->Cells[Index];
							if (FVector::DistSquared(Where, Center + Cell.Center) < FMath::Square(Cell.HalfAxes.GetMax() + Clearance))
							{
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	};
	while (Allowed > 4.0 && Crowded(Subject + Direction * Allowed))
	{
		Allowed -= 1.0;
	}
	// Nie näher als 4 µm an die eigene Zelle: Dann ist sie noch zu sehen, auch wenn dahinter eine Zelle steht
	return Subject + Direction * FMath::Max(4.0, Allowed);
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

	// Der Spieler kann das Mikroskop um die Zelle schwenken. Im Rennen steht die Kamera hinter der
	// eigenen Zelle – wer lenkt, muss sehen, wohin.
	const bool bRacing = Swarm->IsRacing() && FollowCellIndex == Swarm->GetPlayerCellIndex();

	if (bRacing && bRaceEgoView)
	{
		// Halbe Ich-Perspektive: Bezug ist die Zellmitte ohne die seitliche Kopfauslenkung – die Kamera sitzt
		// „auf" der Zelle wie ein Beobachter, der mitschwimmt, nicht am zitternden Kopf. Der Kopf schlägt
		// dadurch sichtbar im Takt vor der Linse hin und her, der Horizont bleibt ruhig.
		const FVector Body = SwarmTransform.TransformPosition(Cell->Position * GenesisMicroScale::UnitsPerMicrometer);
		// Ruhiger Horizont: „oben" ist die Hochachse des Kanals, nicht die Schlagebene. Die Zelle rollt
		// um ihre Längsachse; eine Kamera, die mitrollte, drehte die ganze Welt mehrmals pro Sekunde.
		// So sieht man stattdessen die Geißel sich unter einem wegdrehen – wie es wirklich ist.
		const FVector ChannelUp = SwarmTransform.GetUnitAxis(EAxis::Z);
		FVector EgoUp = (ChannelUp - Forward * FVector::DotProduct(ChannelUp, Forward)).GetSafeNormal();
		if (EgoUp.IsNearlyZero())
		{
			EgoUp = OrbitUp;
		}
		const FVector EgoSide = FVector::CrossProduct(EgoUp, Forward).GetSafeNormal();
		// Umschauen nur begrenzt: Wer über die Schulter blickt, dreht nicht die Kamera vor den eigenen Kopf
		const float EgoAzimuth = FMath::DegreesToRadians(RaceEgoAzimuthDegrees + 0.5f * FMath::Clamp(FRotator::NormalizeAxis(PlayerOrbitDegrees.X), -60.0f, 60.0f));
		const float EgoElevation = FMath::DegreesToRadians(RaceEgoElevationDegrees
			+ 0.35f * FMath::Clamp(PlayerOrbitDegrees.Y, -MaxPlayerPitchDegrees, MaxPlayerPitchDegrees));
		const float EgoDistance = RaceEgoDistanceUm * GenesisMicroScale::UnitsPerMicrometer;
		OutLocation = Body + (Forward * FMath::Cos(EgoAzimuth) * FMath::Cos(EgoElevation) + EgoSide * FMath::Sin(EgoAzimuth) * FMath::Cos(EgoElevation)
			+ EgoUp * FMath::Sin(EgoElevation)) * EgoDistance;
		const FVector Ahead = Body + Forward * RaceEgoLookAheadUm * GenesisMicroScale::UnitsPerMicrometer;
		OutRotation = FRotationMatrix::MakeFromXZ(Ahead - OutLocation, EgoUp).ToQuat();
		OutFocusDistance = static_cast<float>(FVector::Dist(OutLocation, Body + Forward * RaceEgoFocusAheadUm * GenesisMicroScale::UnitsPerMicrometer));
		return true;
	}

	const float Azimuth = FMath::DegreesToRadians((bRacing ? RaceAzimuthDegrees : OrbitAzimuthDegrees) + PlayerOrbitDegrees.X);
	const float Elevation = FMath::DegreesToRadians((bRacing ? RaceElevationDegrees : OrbitElevationDegrees)
		+ FMath::Clamp(PlayerOrbitDegrees.Y, -MaxPlayerPitchDegrees, MaxPlayerPitchDegrees));
	const float Distance = (bRacing ? RaceDistanceUm : OrbitDistanceUm) * GenesisMicroScale::UnitsPerMicrometer;
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
	const FVector LookAt = Head - Forward * (bRacing ? RaceLookBehindHeadUm : LookBehindHeadUm) * GenesisMicroScale::UnitsPerMicrometer;
	OutRotation = FRotationMatrix::MakeFromXZ(LookAt - OutLocation, OrbitUp).ToQuat();
	OutFocusDistance = static_cast<float>(FVector::Dist(OutLocation, bRacing ? LookAt : Head));
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

	// Im Rennen gehört das Bild der eigenen Zelle: hinter ihr, bis sie am Cumulus ankommt – dann die
	// weite Einstellung auf die Eizelle, wo gebohrt wird. Andere Zellen, die sich binden, sind kein Grund
	// wegzuschneiden: Der Spieler soll sehen, wohin er lenkt.
	const bool bRacing = Swarm && Swarm->IsRacing() && Swarm->GetOocyte();
	if (bRacing)
	{
		const int32 Player = Swarm->GetPlayerCellIndex();
		const bool bDecided = Swarm->GetRaceOutcome() == EGenesisRaceOutcome::Won || Swarm->GetRaceOutcome() == EGenesisRaceOutcome::Lost;
		const bool bAtEgg = Swarm->GetPlayerDistanceToZonaUm() < RaceEggViewDistanceUm || Swarm->GetPlayerPhase() != EGenesisSpermPhase::Swimming;
		if (FollowCellIndex != Player && !bWatchOocyte)
		{
			FollowCellIndex = Player;
			bInitialized = false;
		}
		if ((bAtEgg || bDecided) && !bWatchOocyte)
		{
			bWatchOocyte = true;
		}
		else if (!bAtEgg && !bDecided && bWatchOocyte)
		{
			// Abgetrieben oder steckengeblieben und wieder frei: zurück hinter die Zelle
			bWatchOocyte = false;
			SecondsSinceBinding = -1.0f;
		}
	}
	// Sobald eine Zelle an der Zona hängt, gehört das Bild ihr: Die Kamera wechselt auf die Zelle,
	// die es geschafft hat, und begleitet sie beim Bohren durch die Zona. Von außen wäre davon nichts zu sehen –
	// gebundene Zellen stecken unter dem Zellkranz.
	else if (bCloseUpOnBinding && Swarm && Swarm->GetOocyte())
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

	// Dramaturgie der weiten Einstellung: ab der ersten Bindung heranfahren, ab der Verschmelzung zurück
	if (const AGenesisOocyte* Egg = Swarm ? Swarm->GetOocyte() : nullptr)
	{
		const FGenesisOocyteState& EggState = Egg->GetState();
		if (EggState.IsFertilized())
		{
			SecondsSinceFusion = SecondsSinceFusion < 0.0f ? 0.0f : SecondsSinceFusion + DeltaSeconds;
		}
		else if (bRacing ? bWatchOocyte : EggState.BoundCells > 0)
		{
			SecondsSinceBinding = SecondsSinceBinding < 0.0f ? 0.0f : SecondsSinceBinding + DeltaSeconds;
		}
		// Im Rennen fährt die Kamera an die eigene Zelle heran – bis eine andere verschmolzen ist
		const int32 Leader = bRacing && !EggState.IsFertilized() ? Swarm->GetPlayerCellIndex() : Swarm->FindAttachedCell();
		if (Leader != INDEX_NONE)
		{
			const FVector ToLeader = (Swarm->GetCellHeadWorldPosition(Leader) - Egg->GetActorLocation()).GetSafeNormal();
			// Wechselt die Führung, schwenkt die Kamera ruhig hinüber – kein Schnitt mitten im Wettlauf
			const float Alpha = SmoothedLeaderDirection.IsNearlyZero() ? 1.0f : 1.0f - FMath::Exp(-DeltaSeconds / 2.5f);
			SmoothedLeaderDirection = FMath::Lerp(SmoothedLeaderDirection, ToLeader, Alpha).GetSafeNormal();
		}
	}

	FVector DesiredLocation;
	FQuat DesiredRotation;
	float DesiredFocus = 0.0f;
	if (!ComputeDesired(DesiredLocation, DesiredRotation, DesiredFocus))
	{
		return;
	}
	// Im Rennen hinter der eigenen Zelle: Keine Cumuluszelle darf sich vor die Linse schieben (GENESIS-047 Teil 2)
	if (Swarm && Swarm->IsRacing() && !bWatchOocyte && FollowCellIndex == Swarm->GetPlayerCellIndex())
	{
		const FVector Subject = Swarm->GetCellHeadWorldPosition(FollowCellIndex);
		const FVector Avoided = AvoidCumulus(Subject, DesiredLocation);
		DesiredFocus = FMath::Max(1.0f, DesiredFocus - static_cast<float>(FVector::Dist(Avoided, DesiredLocation)));
		DesiredLocation = Avoided;
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
		// Im Rennen folgt die Kamera enger: Mit 0,9 s Nachlauf lief die eigene Zelle aus dem Bild
		const bool bTight = Swarm && Swarm->IsRacing() && !bWatchOocyte;
		// Ich-Perspektive: Die Kamera schwimmt mit. Jeder Nachlauf wäre bei 16 µm Abstand sofort sichtbar
		// (0,35 s bei 50 µm/s = 17 µm – die Kamera fiele auf doppelten Abstand zurück). Die Lage folgt
		// deshalb fast starr, nur die Blickrichtung wird geglättet, damit Lenken weich wirkt.
		const bool bEgo = bTight && bRaceEgoView && FollowCellIndex == Swarm->GetPlayerCellIndex();
		const float PositionSeconds = bEgo ? 0.04f : (bTight ? RacePositionSmoothingSeconds : PositionSmoothingSeconds);
		const float RotationSeconds = bEgo ? 0.25f : (bTight ? RacePositionSmoothingSeconds : RotationSmoothingSeconds);
		const float PositionAlpha = 1.0f - FMath::Exp(-DeltaSeconds / PositionSeconds);
		const float RotationAlpha = 1.0f - FMath::Exp(-DeltaSeconds / RotationSeconds);
		const float FocusAlpha = 1.0f - FMath::Exp(-DeltaSeconds / FocusSmoothingSeconds);
		SetActorLocationAndRotation(FMath::Lerp(GetActorLocation(), DesiredLocation, PositionAlpha), FQuat::Slerp(GetActorQuat(), DesiredRotation, RotationAlpha));
		// Fokus auf den tatsächlichen Abstand der geglätteten Kamera zum Motiv (Zellkopf oder Eizelle)
		const AGenesisOocyte* Egg = bWatchOocyte && Swarm ? Swarm->GetOocyte() : nullptr;
		// Auf die Eizelle: Die Einstellung bestimmt, wohin die Schärfe gehört (Mitte oder Cumulusrand),
		// korrigiert um den Weg, den die gedämpfte Kamera noch vor sich hat
		const FVector FocusTarget = Egg ? Egg->GetActorLocation() : Swarm->GetCellHeadWorldPosition(FollowCellIndex);
		float ActualDistance = static_cast<float>(FVector::Dist(GetActorLocation(), FocusTarget));
		if (Egg)
		{
			ActualDistance += DesiredFocus - static_cast<float>(FVector::Dist(DesiredLocation, FocusTarget));
		}
		else if (bEgo)
		{
			// Schärfe knapp vor dem Kopf, nicht auf dem im Takt pendelnden Kopf selbst
			ActualDistance = DesiredFocus;
		}
		CurrentFocusDistance = FMath::Lerp(CurrentFocusDistance, FMath::Max(1.0f, ActualDistance), FocusAlpha);
	}

	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = CurrentFocusDistance;
	Camera->SetFocusSettings(Focus);
	UpdateLight();
	UpdateSection(DeltaSeconds);

	// Kein MegaLights unter dem Mikroskop: Die dünnen, durchscheinenden Spermien (Kopf und Geißel) blieben damit fast
	// unbeleuchtet und verschwanden im dunklen Eileiter (Vergleichsbild GENESIS-039 Teil 6). Im Kreißsaal ist es an.
	Camera->PostProcessSettings.bOverride_bMegaLights = true;
	Camera->PostProcessSettings.bMegaLights = false;
	// Die Kamera belichtet wie eine echte Kamera: feste Belichtung, Blende und Verschlusszeit wirken auf die Helligkeit.
	// Die Korrektur gleicht den Maßstabssprung aus – im Mikrometerraum trifft die Optik nur wenige Lux.
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	// Abgeblendet im Rennen: gleiche Helligkeit wie beim Zuschauen – je Blendenstufe ein EV zurück
	const float ApertureCompensation = 2.0f * FMath::Log2(FMath::Max(1.0f, Camera->CurrentAperture) / FMath::Max(1.0f, Aperture));
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias + ApertureCompensation;

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
