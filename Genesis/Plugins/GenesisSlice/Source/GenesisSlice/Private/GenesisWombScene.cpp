// GENESIS: Der Kreislauf des Lebens

#include "GenesisWombScene.h"
#include "CineCameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenesisAudioCoreLogic.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisBodySubsystem.h"
#include "GenesisFetalLogic.h"
#include "GenesisFrontendSubsystem.h"
#include "GenesisSlicePlayerController.h"
#include "GenesisLog.h"
#include "GenesisSliceDirector.h"
#include "GenesisWorldClockSubsystem.h"
#include "GenesisWorldSoundActor.h"
#include "GenesisWorldSoundComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	/** Entwickler: Woche und Uhrzeit der Prüfansicht setzen – genesis.Womb.Preview <SSW> [Uhrzeit]. */
	FAutoConsoleCommandWithWorldAndArgs GenesisWombPreviewCommand(
		TEXT("genesis.Womb.Preview"),
		TEXT("Mutterleib aus Sicht des Kindes ohne Durchlauf: genesis.Womb.Preview <SSW> [Uhrzeit] [lx im Mutterleib]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisWombScene> It(World); It; ++It)
			{
				if (Args.Num() > 0) { It->PreviewWeeks = FCString::Atof(*Args[0]); }
				if (Args.Num() > 1) { It->PreviewHour = FCString::Atof(*Args[1]); }
				if (Args.Num() > 2) { It->PreviewWombLux = FCString::Atof(*Args[2]); }
			}
		}));

	/** Entwickler: die Kamerafahrt von außen in die Augen des Kindes starten – genesis.Womb.Exterior. */
	FAutoConsoleCommandWithWorldAndArgs GenesisWombExteriorCommand(
		TEXT("genesis.Womb.Exterior"),
		TEXT("Mutterleib: das Kind von außen zeigen und in seine Augen fahren; mit Sekunden: so lange draußen bleiben"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisWombScene> It(World); It; ++It)
			{
				It->StartExterior(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.0f);
			}
		}));

	/** Entwickler: eine Handlung des Kindes auslösen – genesis.Womb.Do kick|stretch|yawn|thumb|swallow|grasp|eyes|touch|hand <x> <y>. */
	FAutoConsoleCommandWithWorldAndArgs GenesisWombDoCommand(
		TEXT("genesis.Womb.Do"),
		TEXT("Handlung des Kindes im Mutterleib: genesis.Womb.Do kick|stretch|yawn|thumb|swallow|grasp|eyes|touch|hand <x> <y>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisWombScene> It(World); It; ++It)
			{
				It->DebugAction(Args.Num() > 0 ? Args[0] : FString(), Args.Num() > 2 ? FVector2D(FCString::Atof(*Args[1]), FCString::Atof(*Args[2])) : FVector2D::ZeroVector);
			}
		}));

}

namespace GenesisWombPerception
{
	float CavityRadiusCm(float Weeks)
	{
		// Stützpunkte (SSW, Innenradius cm) – Näherung, siehe Deklaration
		// Die Höhle ist längs gut 2 × 1,05 × dieser Radius lang: SSW 28 ≈ 28 cm, am Termin ≈ 36 cm innen – entsprechend
		// dem Symphysen-Fundus-Abstand (≈ SSW in cm ab SSW 20) abzüglich Wand; das gebeugte Kind (längste Ausdehnung ≈
		// Scheitel-Steiß-Länge) füllt sie am Termin fast aus
		static const FVector2D Points[] = {
			FVector2D(8.0f, 1.5f), FVector2D(12.0f, 3.5f), FVector2D(16.0f, 6.5f), FVector2D(20.0f, 9.5f), FVector2D(24.0f, 11.5f),
			FVector2D(28.0f, 13.3f), FVector2D(32.0f, 15.0f), FVector2D(36.0f, 16.3f), FVector2D(40.0f, 17.2f) };
		if (Weeks <= Points[0].X) { return Points[0].Y; }
		for (int32 Index = 1; Index < UE_ARRAY_COUNT(Points); ++Index)
		{
			if (Weeks <= Points[Index].X)
			{
				const float Alpha = (Weeks - Points[Index - 1].X) / (Points[Index].X - Points[Index - 1].X);
				return FMath::Lerp(Points[Index - 1].Y, Points[Index].Y, Alpha);
			}
		}
		return Points[UE_ARRAY_COUNT(Points) - 1].Y;
	}

	FGenesisWombPerception Compute(const FGenesisFetalView& Fetal, float WombLux)
	{
		FGenesisWombPerception Result;
		Result.Presence = Fetal.ConsciousAccess;
		Result.EyesOpen = Fetal.EyesOpen;
		// Auch mit offenen Augen sieht ein Fetus nur Helligkeit und diffuse Flächen
		Result.Blur = 1.0f - 0.5f * Fetal.EyesOpen;
		const float Light = WombLux <= 0.05f ? 0.0f
			: FMath::Clamp(FMath::LogX(10.0f, WombLux / 0.05f) / FMath::LogX(10.0f, 50.0f / 0.05f), 0.0f, 1.0f);
		Result.Brightness = Light * Fetal.LightPerception;
		Result.CavityRadiusCm = CavityRadiusCm(Fetal.GestationalWeeks);
		return Result;
	}
}

AGenesisWombScene::AGenesisWombScene()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Cavity = CreateDefaultSubobject<USceneComponent>(TEXT("Cavity"));
	Cavity->SetupAttachment(Root);
	Cavity->SetMobility(EComponentMobility::Movable);

	auto MakePart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Cavity);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	Wall = MakePart(TEXT("Wall"));
	Placenta = MakePart(TEXT("Placenta"));
	PlacentaVessels = MakePart(TEXT("PlacentaVessels"));
	// Die Nabelschnur ist weich: ein Skelettnetz, dessen Knochenkette die Szene selbst simuliert (Teil 2a)
	Cord = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Cord"));
	Cord->SetupAttachment(Cavity);
	Cord->SetMobility(EComponentMobility::Movable);
	Cord->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	// Kaltlicht an der Optik für die frühen Wochen von außen (Teil 2c): Bis SSW ~12–16 liegt die Gebärmutter im Becken,
	// durch den Bauch kommt kein Licht. Wer dann hineinsieht, bringt das Licht mit – wie bei einer Embryoskopie.
	ScopeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ScopeLight"));
	ScopeLight->SetupAttachment(Camera);
	ScopeLight->SetMobility(EComponentMobility::Movable);
	ScopeLight->bUseTemperature = true;
	ScopeLight->SetTemperature(5600.0f);
	ScopeLight->SetCastShadows(false);
	ScopeLight->SetIntensityUnits(ELightUnits::Candelas);
	ScopeLight->SetIntensity(0.0f);
	ScopeLight->SetVisibility(false);

	// Das Kind selbst – von außen zu sehen, bevor die Kamera in seine Augen fährt (Teil 2b)
	Fetus = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fetus"));
	Fetus->SetupAttachment(Root);
	Fetus->SetMobility(EComponentMobility::Movable);
	Fetus->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fetus->SetVisibility(false);

	OwnHand = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("OwnHand"));
	OwnHand->SetupAttachment(Camera);
	OwnHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OwnHand->SetCastShadow(false);

	MotherVoice = CreateDefaultSubobject<UAudioComponent>(TEXT("MotherVoice"));
	MotherVoice->SetupAttachment(Root);
	MotherVoice->bAutoActivate = false;
	MotherVoice->bAllowSpatialization = false;
}

void AGenesisWombScene::BeginPlay()
{
	Super::BeginPlay();
	Random.Initialize(1234);
	// Alles in der Höhle hängt am Licht durch den Bauch: Die Wand leuchtet, das Gewebe davor lässt es durch
	for (UMeshComponent* Part : { static_cast<UMeshComponent*>(Wall.Get()), static_cast<UMeshComponent*>(Placenta.Get()),
		static_cast<UMeshComponent*>(PlacentaVessels.Get()), static_cast<UMeshComponent*>(Cord.Get()) })
	{
		if (Part && Part->GetMaterial(0))
		{
			LitMaterials.Add(Part->CreateAndSetMaterialInstanceDynamic(0));
		}
	}
	CordMaterial = LitMaterials.Num() > 3 ? LitMaterials[3] : nullptr;
	if (Fetus && Fetus->GetMaterial(0))
	{
		FetusMaterial = Fetus->CreateAndSetMaterialInstanceDynamic(0);
		LitMaterials.Add(FetusMaterial);
	}
	InitCord();
	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->SetCurrentFocalLength(20.0f);
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 0.1f * WorldScale;
	// Die Blende wird mit dem Maßstab kleiner gerechnet (siehe WorldScale)
	Camera->LensSettings.MinFStop = 0.05f;
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = 4.0f * WorldScale;       // wenige Zentimeter vor dem Gesicht
	Camera->SetFocusSettings(Focus);

	// Die Hand: Ruhelage merken und für jedes Fingerglied die Beugeachse bestimmen – quer zum Glied und zum Handrücken,
	// so dass sich die Finger zur Handfläche hin schließen (Handrücken = +Z des Modells)
	const USkeletalMesh* HandMesh = OwnHand ? Cast<USkeletalMesh>(OwnHand->GetSkinnedAsset()) : nullptr;
	if (HandMesh)
	{
		for (int32 Slot = 0; Slot < OwnHand->GetNumMaterials(); ++Slot)
		{
			if (UMaterialInterface* Skin = OwnHand->GetMaterial(Slot))
			{
				LitMaterials.Add(OwnHand->CreateDynamicMaterialInstance(Slot, Skin));
				HandMaterials.Add(LitMaterials.Last());
			}
		}
		const FReferenceSkeleton& Ref = HandMesh->GetRefSkeleton();
		HandRefPose = Ref.GetRefBonePose();
		TArray<FTransform> Component;
		Component.SetNum(HandRefPose.Num());
		for (int32 Bone = 0; Bone < HandRefPose.Num(); ++Bone)
		{
			const int32 Parent = Ref.GetParentIndex(Bone);
			Component[Bone] = Parent == INDEX_NONE ? HandRefPose[Bone] : HandRefPose[Bone] * Component[Parent];
		}
		HandRefComponent = Component;
		FString Names;
		for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone) { Names += Ref.GetBoneName(Bone).ToString() + TEXT(" "); }
		UE_LOG(LogGenesis, Display, TEXT("Hand: %d Knochen: %s"), Ref.GetNum(), *Names);
		HandParents.SetNum(HandRefPose.Num());
		for (int32 Bone = 0; Bone < HandRefPose.Num(); ++Bone)
		{
			HandParents[Bone] = Ref.GetParentIndex(Bone);
		}
		static const TCHAR* Fingers[] = { TEXT("Index"), TEXT("Middle"), TEXT("Ring"), TEXT("Little"), TEXT("Thumb") };
		static const float Degrees[] = { 72.0f, 88.0f, 58.0f };
		for (int32 FingerIndex = 0; FingerIndex < 5; ++FingerIndex)
		{
			const TCHAR* Finger = Fingers[FingerIndex];
			for (int32 Segment = 1; Segment <= 3; ++Segment)
			{
				const int32 Bone = Ref.FindBoneIndex(FName(*FString::Printf(TEXT("%s%d"), Finger, Segment)));
				const int32 Child = Segment < 3 ? Ref.FindBoneIndex(FName(*FString::Printf(TEXT("%s%d"), Finger, Segment + 1))) : INDEX_NONE;
				const int32 Parent = Bone == INDEX_NONE ? INDEX_NONE : Ref.GetParentIndex(Bone);
				if (Bone == INDEX_NONE || Parent == INDEX_NONE)
				{
					continue;
				}
				const FVector Along = (Child != INDEX_NONE ? Component[Child].GetLocation() - Component[Bone].GetLocation()
					: Component[Bone].GetLocation() - Component[Parent].GetLocation()).GetSafeNormal();
				const FVector Axis = FVector::CrossProduct(FVector::UpVector, Along).GetSafeNormal();
				CurlBones.Add(Bone);
				CurlAxes.Add(Component[Bone].GetRotation().Inverse().RotateVector(Axis));
				const bool bThumb = FCString::Strcmp(Finger, TEXT("Thumb")) == 0;
				CurlDegrees.Add(Degrees[Segment - 1] * (bThumb ? 0.5f : 1.0f));
				CurlIsThumb.Add(bThumb);
				CurlFinger.Add(FingerIndex);
				CurlSegment.Add(Segment);
				if (Segment == 3)
				{
					// Fingerkuppe: das Endglied etwa so lang wie der Abstand zum Mittelglied
					FingerTip[FingerIndex] = Bone;
					FingerTipLength[FingerIndex] = 0.9f * FVector::Dist(Component[Bone].GetLocation(), Component[Parent].GetLocation());
				}
			}
		}
	}
}

float AGenesisWombScene::HandLengthCm() const
{
	// Fußlänge nach Hebbar et al. 2013: FL (mm) = −14,02 + 2,361 × SSW; die Hand ist rund 0,8 davon
	// (Neugeborene: Hand 64 mm, Fuß ~80 mm; Halder 1999, Honoré 2016). SSW 28: ~4,2 cm, Termin: ~6,4 cm.
	const float FootMm = FMath::Max(8.0f, -14.02f + 2.361f * Weeks);
	return 0.8f * FootMm / 10.0f;
}

TArray<FTransform> AGenesisWombScene::HandComponentPose(const TArray<float>& Curl) const
{
	// Vorwärtskinematik: dieselbe Rechnung wie im Skelett, damit die Kollision der Finger vor dem Zeichnen bekannt ist
	TArray<FTransform> Local = HandRefPose;
	for (int32 Index = 0; Index < CurlBones.Num(); ++Index)
	{
		const int32 Bone = CurlBones[Index];
		Local[Bone].SetRotation(Local[Bone].GetRotation() * FQuat(CurlAxes[Index], FMath::DegreesToRadians(CurlDegrees[Index] * Curl[Index])));
	}
	TArray<FTransform> Component;
	Component.SetNum(Local.Num());
	for (int32 Bone = 0; Bone < Local.Num(); ++Bone)
	{
		Component[Bone] = HandParents[Bone] == INDEX_NONE ? Local[Bone] : Local[Bone] * Component[HandParents[Bone]];
	}
	return Component;
}

void AGenesisWombScene::PhalanxSegment(const TArray<FTransform>& Component, int32 CurlIndex, FVector& OutStart, FVector& OutEnd) const
{
	// Ein Fingerglied von seinem Gelenk zum nächsten (das Endglied bis zur Kuppe), in Weltkoordinaten
	const FTransform& ToWorld = OwnHand->GetComponentTransform();
	const int32 Bone = CurlBones[CurlIndex];
	OutStart = ToWorld.TransformPosition(Component[Bone].GetLocation());
	if (CurlSegment[CurlIndex] != 3 && CurlBones.IsValidIndex(CurlIndex + 1))
	{
		OutEnd = ToWorld.TransformPosition(Component[CurlBones[CurlIndex + 1]].GetLocation());
		return;
	}
	const FVector Along = ToWorld.TransformVector(Component[Bone].GetLocation() - Component[HandParents[Bone]].GetLocation()).GetSafeNormal();
	OutEnd = OutStart + Along * FingerTipLength[CurlFinger[CurlIndex]] * ToWorld.GetScale3D().X;
}

float AGenesisWombScene::DistanceToCord(const FVector& A, const FVector& B, FVector& OutOnSegment, FVector& OutOnCord) const
{
	// Kürzester Abstand eines Segments (Welt) zur Mittellinie der Schnur – die Schnur als Kette von Kapseln
	float Best = TNumericLimits<float>::Max();
	const FTransform& CordToWorld = Cord->GetComponentTransform();
	for (int32 Index = 0; Index + 1 < CordX.Num(); ++Index)
	{
		FVector P, Q;
		FMath::SegmentDistToSegmentSafe(A, B, CordToWorld.TransformPosition(CordX[Index]), CordToWorld.TransformPosition(CordX[Index + 1]), P, Q);
		const float Distance = FVector::Dist(P, Q);
		if (Distance < Best)
		{
			Best = Distance;
			OutOnSegment = P;
			OutOnCord = Q;
		}
	}
	return Best;
}

float AGenesisWombScene::CordRadiusWorld() const
{
	return CordRadius * (Cord ? Cord->GetComponentTransform().GetScale3D().X : 1.0f);
}

bool AGenesisWombScene::PhalanxTouchesCord(const TArray<FTransform>& Component, int32 CurlIndex, float Radius) const
{
	if (!Cord || CordX.Num() < 2 || !CurlBones.IsValidIndex(CurlIndex))
	{
		return false;
	}
	FVector A, B, OnFinger, OnCord;
	PhalanxSegment(Component, CurlIndex, A, B);
	// Die Sulze gibt nach: Je fester das Kind zudrückt, desto tiefer darf das Glied eindrücken (bis ein Viertel des Radius)
	const float Allowed = 0.25f * CordRadiusWorld() * HandClosed;
	return DistanceToCord(A, B, OnFinger, OnCord) < Radius + CordRadiusWorld() - Allowed;
}

void AGenesisWombScene::InitCord()
{
	const USkeletalMesh* Mesh = Cord ? Cast<USkeletalMesh>(Cord->GetSkinnedAsset()) : nullptr;
	if (!Mesh)
	{
		return;
	}
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	CordRefLocal = Ref.GetRefBonePose();
	CordRefComponent.SetNum(CordRefLocal.Num());
	CordParents.SetNum(CordRefLocal.Num());
	for (int32 Bone = 0; Bone < CordRefLocal.Num(); ++Bone)
	{
		CordParents[Bone] = Ref.GetParentIndex(Bone);
		CordRefComponent[Bone] = CordParents[Bone] == INDEX_NONE ? CordRefLocal[Bone] : CordRefLocal[Bone] * CordRefComponent[CordParents[Bone]];
	}
	// Teilchen: der Anfang jedes Knochens, dazu das Ende der Kette (Nabel) – in Richtung des letzten Knochens verlängert
	CordRest.Reset();
	for (int32 Bone = 0; Bone < CordRefComponent.Num(); ++Bone)
	{
		CordRest.Add(CordRefComponent[Bone].GetLocation());
	}
	const int32 Count = CordRest.Num();
	if (Count >= 2)
	{
		CordRest.Add(CordRest[Count - 1] + (CordRest[Count - 1] - CordRest[Count - 2]));
	}
	CordX = CordRest;
	CordPrev = CordRest;
	CordRestLength.SetNum(FMath::Max(0, CordRest.Num() - 1));
	for (int32 Index = 0; Index + 1 < CordRest.Num(); ++Index)
	{
		CordRestLength[Index] = FVector::Dist(CordRest[Index], CordRest[Index + 1]);
	}
	UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Nabelschnur mit %d Teilchen, Radius %.2f"), CordX.Num(), CordRadius);
}

void AGenesisWombScene::SimulateCord(float DeltaSeconds, const TArray<FVector>& FingerA, const TArray<FVector>& FingerB, float FingerRadius)
{
	if (!Cord || CordX.Num() < 3)
	{
		return;
	}
	// Die Nabelschnur im Fruchtwasser (XPBD-artig, im Raum des Netzes): Sie behält Länge und Schlingenform (die Sulze ist
	// elastisch), das Wasser bremst jede Bewegung, beide Enden sind fest (Plazenta, Nabel). Finger und Handteller schieben
	// sie weg; wo sie drücken, gibt die Sulze nach (Delle im Material).
	const FTransform CordToWorld = Cord->GetComponentTransform();
	const float Scale = FMath::Max(KINDA_SMALL_NUMBER, CordToWorld.GetScale3D().X);
	TArray<FVector> A, B;
	for (int32 Index = 0; Index < FingerA.Num(); ++Index)
	{
		A.Add(CordToWorld.InverseTransformPosition(FingerA[Index]));
		B.Add(CordToWorld.InverseTransformPosition(FingerB[Index]));
	}
	// Das Ende der Schnur sitzt am Nabel des Kindes (wo auch immer sein Körper gerade liegt)
	if (bHasNavel)
	{
		const FVector Navel = CordToWorld.InverseTransformPosition(NavelWorld);
		CordRest.Last() = Navel;
		CordX.Last() = Navel;
		CordPrev.Last() = Navel;
	}
	const float Reach = FingerRadius / Scale + CordRadius;
	const float Allowed = 0.25f * CordRadius * FMath::Max(0.2f, HandClosed);
	const int32 Steps = 4;
	const float H = FMath::Min(DeltaSeconds, 0.05f) / Steps;
	const int32 Last = CordX.Num() - 1;
	CordDents.Reset();
	for (int32 Step = 0; Step < Steps; ++Step)
	{
		for (int32 Index = 1; Index < Last; ++Index)
		{
			// Trägheit mit starker Dämpfung (Wasser), dazu der Zug zurück in die eigene Form
			const FVector Velocity = (CordX[Index] - CordPrev[Index]) * FMath::Exp(-6.0f * H);
			CordPrev[Index] = CordX[Index];
			CordX[Index] += Velocity + (CordRest[Index] - CordX[Index]) * (1.0f - FMath::Exp(-0.5f * H));
		}
		// Hält die Hand die Schnur, folgt das gegriffene Stück ihr
		if (bHoldingCord && CordX.IsValidIndex(GraspParticle) && GraspParticle > 0 && GraspParticle < Last)
		{
			const FVector Goal = CordToWorld.InverseTransformPosition(GraspPointWorld);
			CordX[GraspParticle] += (Goal - CordX[GraspParticle]) * 0.5f;
		}
		for (int32 Iteration = 0; Iteration < 3; ++Iteration)
		{
			for (int32 Index = 0; Index < Last; ++Index)
			{
				const FVector Delta = CordX[Index + 1] - CordX[Index];
				const float Length = FMath::Max(KINDA_SMALL_NUMBER, Delta.Size());
				const FVector Fix = Delta * ((Length - CordRestLength[Index]) / Length) * 0.5f;
				if (Index > 0) { CordX[Index] += Fix * (Index + 1 == Last ? 2.0f : 1.0f); }
				if (Index + 1 < Last) { CordX[Index + 1] -= Fix * (Index == 0 ? 2.0f : 1.0f); }
			}
			// Biegesteife: Nachbarn zweiten Grades halten ihren Abstand halbwegs
			for (int32 Index = 1; Index + 1 < Last; ++Index)
			{
				const float Rest = FVector::Dist(CordRest[Index - 1], CordRest[Index + 1]);
				const FVector Delta = CordX[Index + 1] - CordX[Index - 1];
				const float Length = FMath::Max(KINDA_SMALL_NUMBER, Delta.Size());
				const FVector Fix = Delta * ((Length - Rest) / Length) * 0.15f;
				CordX[Index - 1] += Index - 1 > 0 ? Fix : FVector::ZeroVector;
				CordX[Index + 1] -= Index + 1 < Last ? Fix : FVector::ZeroVector;
			}
			// Kontakt: Kein Glied der Hand in die Schnur – bis auf die weiche Eindrücktiefe
			for (int32 Index = 1; Index < Last; ++Index)
			{
				for (int32 Part = 0; Part < A.Num(); ++Part)
				{
					const FVector Closest = FMath::ClosestPointOnSegment(CordX[Index], A[Part], B[Part]);
					const FVector Away = CordX[Index] - Closest;
					const float Distance = Away.Size();
					if (Distance < Reach - Allowed && Distance > KINDA_SMALL_NUMBER)
					{
						CordX[Index] = Closest + Away / Distance * (Reach - Allowed);
					}
				}
			}
		}
	}
	// Dellen fürs Material: wo ein Glied näher ist als Finger- plus Schnurradius
	for (int32 Part = 0; Part < A.Num() && CordDents.Num() < 4; ++Part)
	{
		float Best = TNumericLimits<float>::Max();
		FVector OnCord = FVector::ZeroVector;
		FVector OnFinger = FVector::ZeroVector;
		for (int32 Index = 0; Index < Last; ++Index)
		{
			FVector P, Q;
			FMath::SegmentDistToSegmentSafe(A[Part], B[Part], CordX[Index], CordX[Index + 1], P, Q);
			if (FVector::Dist(P, Q) < Best)
			{
				Best = FVector::Dist(P, Q);
				OnFinger = P;
				OnCord = Q;
			}
		}
		const float Depth = Reach - Best;
		if (Depth > 0.01f * CordRadius)
		{
			const FVector Toward = (OnFinger - OnCord).GetSafeNormal();
			const FVector Surface = CordToWorld.TransformPosition(OnCord + Toward * CordRadius);
			CordDents.Add(FVector4(Surface.X, Surface.Y, Surface.Z, FMath::Min(Depth, 0.4f * CordRadius) * Scale));
		}
	}

	// Knochen schreiben: Lage aus den Teilchen, Drehung so, dass jeder Knochen zum nächsten Teilchen zeigt
	TArray<FTransform> Component = CordRefComponent;
	for (int32 Bone = 0; Bone < CordRefComponent.Num() && Bone + 1 < CordX.Num(); ++Bone)
	{
		const FVector RefDir = (CordRest[Bone + 1] - CordRest[Bone]).GetSafeNormal();
		const FVector NewDir = (CordX[Bone + 1] - CordX[Bone]).GetSafeNormal();
		Component[Bone].SetLocation(CordX[Bone]);
		Component[Bone].SetRotation(FQuat::FindBetweenNormals(RefDir, NewDir) * CordRefComponent[Bone].GetRotation());
	}
	if (Cord->BoneSpaceTransforms.Num() != CordRefLocal.Num())
	{
		Cord->BoneSpaceTransforms = CordRefLocal;
	}
	for (int32 Bone = 0; Bone < Component.Num(); ++Bone)
	{
		const int32 Parent = CordParents[Bone];
		Cord->BoneSpaceTransforms[Bone] = Parent == INDEX_NONE ? Component[Bone] : Component[Bone].GetRelativeTransform(Component[Parent]);
	}
	Cord->RefreshBoneTransforms();
	Cord->MarkRefreshTransformDirty();
	Cord->UpdateBounds();

	// Die Wharton-Sulze federt nicht schlagartig zurück, sondern über Sekunden (wie ein nasser Schwamm, Gervaso 2014):
	// Dellen bleiben stehen und klingen ab; neue Kontakte ersetzen die schwächsten
	for (FVector4& Old : ShownDents)
	{
		Old.W *= FMath::Exp(-FMath::Min(DeltaSeconds, 0.05f) / 2.5f);
	}
	for (const FVector4& New : CordDents)
	{
		int32 Slot = INDEX_NONE;
		for (int32 Index = 0; Index < ShownDents.Num(); ++Index)
		{
			if (FVector::Dist(FVector(ShownDents[Index]), FVector(New)) < 1.2f * CordRadiusWorld()) { Slot = Index; break; }
		}
		if (Slot == INDEX_NONE && ShownDents.Num() < 4) { Slot = ShownDents.Add(New); }
		if (Slot == INDEX_NONE)
		{
			Slot = 0;
			for (int32 Index = 1; Index < ShownDents.Num(); ++Index) { Slot = ShownDents[Index].W < ShownDents[Slot].W ? Index : Slot; }
		}
		ShownDents[Slot] = FVector4(New.X, New.Y, New.Z, FMath::Max(New.W, ShownDents[Slot].W * 0.5f));
	}
	if (CordMaterial)
	{
		for (int32 Dent = 0; Dent < 4; ++Dent)
		{
			const FVector4 Value = ShownDents.IsValidIndex(Dent) ? ShownDents[Dent] : FVector4(0.0, 0.0, -1.0e6, 0.0);
			CordMaterial->SetVectorParameterValue(*FString::Printf(TEXT("Delle%d"), Dent), FLinearColor(Value.X, Value.Y, Value.Z, 1.0f));
			CordMaterial->SetScalarParameterValue(*FString::Printf(TEXT("Tiefe%d"), Dent), Value.W);
		}
		CordMaterial->SetScalarParameterValue(TEXT("DellenRadius"), 1.1f * CordRadiusWorld());
	}
}

void AGenesisWombScene::UpdateOwnHand(float DeltaSeconds)
{
	if (!OwnHand || !OwnHand->GetSkinnedAsset() || HandRefPose.Num() == 0)
	{
		return;
	}
	const float Dt = FMath::Min(DeltaSeconds, 0.05f);
	// Größe und Abstände wachsen mit dem Kind (das Modell ist 6 cm lang gebaut)
	const float Body = HandLengthCm() / 6.0f;

	// Ziel: vor dem Gesicht (Kamera: X vorn, Y rechts, Z oben), mit der Hand-Taste bewegt – oder am Mund.
	// Dazu ein langsames Treiben: Die Hand schwebt im Fruchtwasser, sie steht nie ganz still.
	const FVector InView(6.5f, 2.8f * HandPosition.X, 2.2f * HandPosition.Y);
	const FVector AtMouth(1.4f, 0.3f, -2.3f);
	// Wie viel sich das Kind von selbst bewegt: im ruhigen Schlaf fast nichts, in aktiven Phasen mehr (Nijhuis 1982)
	const float ActivityGoal = Behaviour.State == EGenesisFetalState::Quiet ? 0.25f : Behaviour.State == EGenesisFetalState::QuietAwake ? 0.5f : 1.0f;
	Activity = FMath::FInterpTo(Activity, ActivityGoal, Dt, 0.5f);
	const FVector Drift = Activity * FVector(0.25f * FMath::PerlinNoise1D(0.21f * SceneSeconds + 3.1f),
		0.35f * FMath::PerlinNoise1D(0.17f * SceneSeconds + 7.7f), 0.3f * FMath::PerlinNoise1D(0.23f * SceneSeconds + 12.9f));
	FVector Target = (FMath::Lerp(InView, AtMouth, HandToMouth) + Drift * (1.0f - HandToMouth)) * Body;
	// Will das Kind greifen und ist die Nabelschnur in Reichweite, wendet es die Handfläche zu ihr und legt sie an
	const FTransform CameraToWorld = Camera->GetComponentTransform();
	const FVector PalmFromWrist = OwnHand->GetRelativeRotation().RotateVector(FVector(1.5f, 0.0f, -0.2f)) * Body;
	float ReachGoal = 0.0f;
	if (Cord && CordX.Num() >= 2 && bGraspIntent && !bHoldingCord)
	{
		const FVector PalmWorld = CameraToWorld.TransformPosition((HandPos + PalmFromWrist) * WorldScale);
		FVector OnPalm, OnCord;
		const float Distance = DistanceToCord(PalmWorld, PalmWorld, OnPalm, OnCord);
		if (Distance < 3.0f * Body * WorldScale)
		{
			const FVector Surface = OnCord + (PalmWorld - OnCord).GetSafeNormal() * CordRadiusWorld();
			CordContact = CameraToWorld.InverseTransformPosition(Surface) / WorldScale;
			CordSide = CameraToWorld.InverseTransformVectorNoScale((OnCord - PalmWorld).GetSafeNormal());
			ReachGoal = 1.0f;
		}
	}
	ReachAlpha = FMath::FInterpTo(ReachAlpha, bHoldingCord ? 1.0f : ReachGoal, Dt, 3.0f);
	if (ReachAlpha > 0.01f)
	{
		// Die Handfläche liegt etwa eine halbe Handdicke vor der Schnur
		const FVector PalmGoal = CordContact - CordSide * 0.55f * Body;
		Target = FMath::Lerp(Target, PalmGoal - PalmFromWrist, ReachAlpha);
	}

	// Hält die Hand die Nabelschnur, bleibt sie dort – sie zieht nur ein wenig daran (die Schnur gibt nach)
	if (bHoldingCord)
	{
		const FVector Pull = Target - GraspAnchor;
		Target = GraspAnchor + Pull.GetClampedToMaxSize(0.8f * Body);
	}

	// Wie ein Körper im Wasser: gedämpfte Feder zum Ziel, dazu Wasserwiderstand – träge, weich, mit leichtem Nachschwingen
	if (!bHandPlaced)
	{
		HandPos = Target;
		HandVelocity = FVector::ZeroVector;
		bHandPlaced = true;
	}
	const float Stiffness = 22.0f;
	const float Damping = 2.0f * 0.65f * FMath::Sqrt(Stiffness);
	HandVelocity += (Stiffness * (Target - HandPos) - Damping * HandVelocity) * Dt;
	FVector Next = HandPos + HandVelocity * Dt;

	// Die Nabelschnur ist ein Hindernis: Handteller und Finger dürfen nur so weit in sie hinein, wie die Sulze nachgibt –
	// den Rest schiebt die Schnur selbst beiseite (sie schwimmt)
	if (Cord && CordX.Num() >= 2)
	{
		const FVector FingersFromWrist = OwnHand->GetRelativeRotation().RotateVector(FVector(3.6f, 0.0f, -0.35f)) * Body;
		const float PartRadius = 0.55f * Body * WorldScale;
		bool bBump = false;
		for (const FVector& Part : { PalmFromWrist, FingersFromWrist })
		{
			const FVector Point = CameraToWorld.TransformPosition((Next + Part) * WorldScale);
			FVector OnPart, OnCord;
			const float Distance = DistanceToCord(Point, Point, OnPart, OnCord);
			const float Limit = PartRadius + 0.5f * CordRadiusWorld();
			if (Distance < Limit && Distance > KINDA_SMALL_NUMBER)
			{
				const FVector Push = (Point - OnCord) / Distance * (Limit - Distance);
				Next += CameraToWorld.InverseTransformVector(Push) / WorldScale;
				const FVector NormalLocal = CameraToWorld.InverseTransformVectorNoScale((Point - OnCord) / Distance);
				HandVelocity -= FMath::Min(0.0f, FVector::DotProduct(HandVelocity, NormalLocal)) * NormalLocal;
				bBump = true;
			}
		}
		if (bBump && !bCordBumped)
		{
			Feel(0.15f, 0.12f, false);
		}
		bCordBumped = bBump;
	}
	HandPos = Next;

	// Der Unterarm kommt von unten und leicht von der Seite – so zeigt die Handfläche zum Gesicht. Die Hand neigt sich mit
	// der Bewegung ein wenig nach (das Wasser bremst sie)
	const FVector Shoulder = FVector(HandPos.X - 1.0f * Body, 2.6f * Body, -4.6f * Body);
	const FVector Along = (HandPos - Shoulder + HandVelocity * 0.08f).GetSafeNormal();
	FVector Back = FMath::Lerp(FVector::ForwardVector, FVector::RightVector, 0.5f * HandToMouth).GetSafeNormal();
	// Beim Greifen zeigt die Handfläche zur Schnur, der Handrücken von ihr weg
	Back = FMath::Lerp(Back, -CordSide, ReachAlpha).GetSafeNormal();
	// Fetale Bewegungen drehen leicht um die Achse der Gliedmaße (Prechtl): der Unterarm dreht langsam hin und her
	const float Roll = 14.0f * Activity * FMath::PerlinNoise1D(0.13f * SceneSeconds + 21.7f);
	const FQuat Oriented = FQuat(Along, FMath::DegreesToRadians(Roll)) * FRotationMatrix::MakeFromXZ(Along, Back).ToQuat();
	OwnHand->SetRelativeLocationAndRotation(HandPos * WorldScale, Oriented.Rotator());
	OwnHand->SetRelativeScale3D(FVector(Body * WorldScale));
	TArray<UMaterialInstanceDynamic*> Skins(HandMaterials);
	if (FetusMaterial) { Skins.Add(FetusMaterial); }
	for (UMaterialInstanceDynamic* Skin : Skins)
	{
		const float Fat = FMath::SmoothStep(28.0f, 40.0f, Weeks);
		Skin->SetScalarParameterValue(TEXT("Durchlass"), FMath::Lerp(0.8f, 0.45f, Fat));
		Skin->SetVectorParameterValue(TEXT("Farbe"), FMath::Lerp(FLinearColor(0.58f, 0.27f, 0.25f), FLinearColor(0.60f, 0.38f, 0.34f), Fat));
		// Käseschmiere: entsteht im letzten Drittel (Nishijima 2019), zum Termin löst sich ein Teil ins Fruchtwasser
		// (Lamberti 1978) – es bleiben Flecken und die Falten. Lanugo ab SSW ~20, zum Termin größtenteils abgestoßen.
		Skin->SetScalarParameterValue(TEXT("Kaeseschmiere"), FMath::SmoothStep(24.0f, 34.0f, Weeks) * (1.0f - 0.45f * FMath::SmoothStep(37.0f, 40.0f, Weeks)));
		Skin->SetScalarParameterValue(TEXT("Lanugo"), FMath::SmoothStep(19.0f, 22.0f, Weeks) * (1.0f - 0.8f * FMath::SmoothStep(32.0f, 38.0f, Weeks)));
	}

	// Finger: in Ruhe gestaffelt gebeugt (Zeigefinger am gestrecktesten, kleiner Finger am stärksten) mit langsamem
	// Eigenleben; mit Greifen zur Faust; am Mund steht der Daumen ab
	const float Tucked = FMath::SmoothStep(34.0f, 40.0f, Weeks);
	const float Rest[5] = { 0.30f, 0.40f, 0.50f, 0.62f, FMath::Lerp(0.25f, 0.6f, Tucked) };
	float Goal[5];
	for (int32 Finger = 0; Finger < 5; ++Finger)
	{
		const float Life = 0.08f * Activity * FMath::PerlinNoise1D(0.35f * SceneSeconds + 5.3f * Finger);
		const bool bThumb = Finger == 4;
		const float Fist = bThumb ? 0.85f : 1.0f;
		Goal[Finger] = FMath::Lerp(Rest[Finger] + Life, Fist, HandClosed);
		Goal[Finger] = bThumb ? FMath::Lerp(Goal[Finger], 0.05f, HandToMouth) : FMath::Lerp(Goal[Finger], 0.6f, HandToMouth * (1.0f - HandClosed));
	}
	if (JointCurl.Num() != CurlBones.Num())
	{
		JointCurl.SetNum(CurlBones.Num());
		for (int32 Index = 0; Index < CurlBones.Num(); ++Index)
		{
			JointCurl[Index] = Rest[CurlFinger[Index]];
		}
	}

	// Greifen wie eine echte Hand: Jedes Gelenk beugt sich, bis sein Glied (oder ein weiter außen liegendes) die Schnur
	// berührt – dann bleibt es stehen, die äußeren Glieder beugen sich weiter und legen sich um sie. Keine Kapsel darf in
	// die Schnur hinein; Öffnen geht immer.
	const float PhalanxRadius = 0.30f * Body * WorldScale;
	bool Contact[5] = { false, false, false, false, false };
	for (int32 Index = 0; Index < CurlBones.Num(); ++Index)
	{
		const int32 Finger = CurlFinger[Index];
		// schließen schneller als öffnen; die äußeren Glieder folgen weicher (sie hängen etwas nach)
		const float Rate = (Goal[Finger] > JointCurl[Index] ? 4.0f : 2.4f) * (1.25f - 0.2f * CurlSegment[Index]);
		const float Wanted = FMath::FInterpTo(JointCurl[Index], Goal[Finger], Dt, Rate);
		if (Wanted <= JointCurl[Index] || !Cord)
		{
			JointCurl[Index] = Wanted;
			continue;
		}
		TArray<float> Try = JointCurl;
		Try[Index] = Wanted;
		const TArray<FTransform> Pose = HandComponentPose(Try);
		bool bBlocked = false;
		for (int32 Outer = Index; Outer < CurlBones.Num() && CurlFinger[Outer] == Finger; ++Outer)
		{
			bBlocked |= PhalanxTouchesCord(Pose, Outer, PhalanxRadius);
		}
		if (bBlocked)
		{
			Contact[Finger] = true;
		}
		else
		{
			JointCurl[Index] = Wanted;
		}
	}
	if (OwnHand->BoneSpaceTransforms.Num() != HandRefPose.Num())
	{
		OwnHand->BoneSpaceTransforms = HandRefPose;
	}
	for (int32 Index = 0; Index < CurlBones.Num(); ++Index)
	{
		const int32 Bone = CurlBones[Index];
		FTransform Local = HandRefPose[Bone];
		Local.SetRotation(Local.GetRotation() * FQuat(CurlAxes[Index], FMath::DegreesToRadians(CurlDegrees[Index] * JointCurl[Index])));
		if (OwnHand->BoneSpaceTransforms.IsValidIndex(Bone))
		{
			OwnHand->BoneSpaceTransforms[Bone] = Local;
		}
	}
	// Sofort neu rechnen: Die Komponente tickt nicht zuverlässig selbst (dann stünde die Hand in der Ruhepose)
	OwnHand->RefreshBoneTransforms();
	OwnHand->MarkRefreshTransformDirty();

	// Gehalten ist die Schnur, wenn sich beim Greifen mindestens drei Finger an sie gelegt haben (mit Hysterese)
	int32 Around = 0;
	for (int32 Finger = 0; Finger < 5; ++Finger)
	{
		Around += Contact[Finger] ? 1 : 0;
	}
	HeldFingers = FMath::FInterpTo(HeldFingers, static_cast<float>(Around), Dt, 6.0f);
	const bool bHolding = bHoldingCord ? (HandClosed > 0.35f && HeldFingers > 1.5f) : (HandClosed > 0.5f && HeldFingers > 2.5f);
	if (bHolding && !bHoldingCord)
	{
		HoldLimit = Random.FRandRange(3.0f, 7.0f);
		GraspAnchor = HandPos;
		// Das Teilchen der Schnur nahe der Handfläche folgt ab jetzt der Hand
		const FVector PalmWorld = CameraToWorld.TransformPosition((HandPos + PalmFromWrist) * WorldScale);
		float Best = TNumericLimits<float>::Max();
		for (int32 Index = 1; Index + 1 < CordX.Num(); ++Index)
		{
			const float Distance = FVector::Dist(Cord->GetComponentTransform().TransformPosition(CordX[Index]), PalmWorld);
			if (Distance < Best) { Best = Distance; GraspParticle = Index; }
		}
		GraspOffset = Cord->GetComponentTransform().TransformPosition(CordX[GraspParticle]) - PalmWorld;
		Feel(0.3f, 0.3f, false);
		ShowCaption(TEXT("Die Nabelschnur in der Hand – weich, und sie pulsiert."));
		UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Das Kind greift die Nabelschnur (SSW %.1f, %d Finger liegen an)."), Weeks, Around);
	}
	bHoldingCord = bHolding;

	// Die Schnur simulieren – Fingerglieder und Handteller sind die Hindernisse
	TArray<FVector> PartA, PartB;
	const TArray<FTransform> FinalPose = HandComponentPose(JointCurl);
	for (int32 Index = 0; Index < CurlBones.Num(); ++Index)
	{
		FVector A, B;
		PhalanxSegment(FinalPose, Index, A, B);
		PartA.Add(A);
		PartB.Add(B);
	}
	const FVector PalmStart = CameraToWorld.TransformPosition((HandPos + OwnHand->GetRelativeRotation().RotateVector(FVector(0.4f, 0.0f, -0.2f)) * Body) * WorldScale);
	const FVector PalmEnd = CameraToWorld.TransformPosition((HandPos + OwnHand->GetRelativeRotation().RotateVector(FVector(2.6f, 0.0f, -0.2f)) * Body) * WorldScale);
	PartA.Add(PalmStart);
	PartB.Add(PalmEnd);
	GraspPointWorld = CameraToWorld.TransformPosition((HandPos + PalmFromWrist) * WorldScale) + GraspOffset;
	SimulateCord(Dt, PartA, PartB, PhalanxRadius);
	if (bHoldingCord)
	{
		CordPulsePhase += Dt * FMath::Max(60.0f, FetalNow.HeartRateBpm) / 60.0f;
		if (CordPulsePhase >= 1.0f)
		{
			CordPulsePhase -= 1.0f;
			Feel(0.12f, 0.05f, false);
		}
	}
}

void AGenesisWombScene::UpdateTime()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisSliceDirector* Director = GameInstance ? GameInstance->GetSubsystem<UGenesisSliceDirector>() : nullptr;
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisBodyState* BodyState = Director && Body && Director->GetState().EntityId.IsValid() ? Body->FindBody(Director->GetState().EntityId) : nullptr;
	if (Director && Clock && BodyState && !BodyState->bBorn && Director->GetState().Phase == EGenesisSlicePhase::Gestation)
	{
		const double Hours = static_cast<double>(Clock->GetNow() - BodyState->ConceptionTime) / FGenesisTimestamp::SecondsPerHour;
		Weeks = static_cast<float>(Hours / (7.0 * 24.0) + GenesisFetalLogic::WeeksFromConceptionToGestational);
		HourOfDay = FMath::Fmod(Hours, 24.0);
		DayIndex = FMath::FloorToInt32(Hours / 24.0);
		Seed = static_cast<int32>(Director->GetState().RunSeed & 0x7fffffff);
		const int32 MomentIndex = Director->GetGestationPlanPoint().MomentIndex;
		if (MomentIndex != LastMomentIndex && MomentIndex >= 0)
		{
			// Ein neuer Moment: Sie spricht bald, wenn sie wach ist und spricht (erst ab SSW 16) – und zuerst sieht man das
			// Kind von außen, dann fährt die Kamera in seine Augen
			SinceVoice = 17.0f;
			const TArray<FGenesisGestationMoment>& Moments = Director->Tuning.GestationMoments;
			const bool bOutside = Moments.IsValidIndex(MomentIndex) && Moments[MomentIndex].bOnlyFromOutside;
			StartExterior(bOutside ? Moments[MomentIndex].Seconds : 0.0f);
			const FGenesisMotherMoment Now = GenesisMotherDay::Evaluate(FGenesisMotherDayTuning(), HourOfDay, DayIndex, Weeks, Seed);
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Moment %d – SSW %.1f, %02d:%02d Uhr, Mutter: %s, %.2f lx im Mutterleib"),
				MomentIndex, Weeks, FMath::FloorToInt32(HourOfDay), FMath::FloorToInt32(FMath::Fmod(HourOfDay, 1.0) * 60.0),
				*GenesisMotherDay::GetActivityName(Now.Activity), Now.WombLux);
		}
		// Ein Moment von außen endet mit dem Moment: danach wieder der Zeitraffer hinter geschlossenen Lidern
		if (MomentIndex < 0 && ExteriorStaySeconds > 0.0f && ExteriorAge >= 0.0f)
		{
			ExteriorAge = -1.0f;
			ExteriorStaySeconds = 0.0f;
		}
		LastMomentIndex = MomentIndex;
		bInRun = true;
		return;
	}
	// Ohne Durchlauf: die Prüfansicht, die Zeit läuft langsam weiter
	bInRun = false;
	Weeks = PreviewWeeks;
	HourOfDay = FMath::Fmod(static_cast<double>(PreviewHour) + SceneSeconds * 8.0 / 3600.0, 24.0);
	DayIndex = 100;
}

void AGenesisWombScene::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SceneSeconds += DeltaSeconds;
	SinceVoice += DeltaSeconds;
	UpdateTime();

	const FGenesisFetalReference& Reference = GenesisFetalLogic::GetReference();
	const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(Reference, Weeks);
	Mother = GenesisMotherDay::Evaluate(FGenesisMotherDayTuning(), HourOfDay, DayIndex, Weeks, Seed);
	if (!bInRun && PreviewWombLux >= 0.0f)
	{
		Mother.WombLux = PreviewWombLux;
	}
	Perception = GenesisWombPerception::Compute(Fetal, Mother.WombLux);
	FetalNow = Fetal;
	TArray<EGenesisFetalEvent> Events = GenesisFetalLogic::AdvanceBehaviour(Behaviour, Fetal, Reference.Milestones, DeltaSeconds, Mother.Rocking, Random);
	UpdatePlayer(DeltaSeconds, Fetal, Events);
	UpdateMotherTouch(DeltaSeconds, Fetal);
	// Lider zu (vom Spieler oder beim Gähnen): nur noch ein rotes Leuchten durch die Haut, keine Formen
	if (bLidsClosedByPlayer || (YawnAge > 0.3f && YawnAge < 1.6f))
	{
		Perception.EyesOpen = 0.0f;
		Perception.Blur = 1.0f;
		Perception.Brightness *= 0.4f;
	}

	// Die Höhle wächst mit der Woche (gebaut für 10 cm Innenradius)
	Cavity->SetRelativeScale3D(FVector(Perception.CavityRadiusCm / 10.0f * WorldScale));

	// Licht: Die vordere Wand leuchtet mit dem Licht, das durch den Bauch kommt (nur Rot); Gewebe davor scheint durch
	for (UMaterialInstanceDynamic* Material : LitMaterials)
	{
		Material->SetScalarParameterValue(TEXT("Glow"), Mother.WombLux * GlowPerLux);
		Material->SetVectorParameterValue(TEXT("BellyDirection"), FLinearColor(BellyDirection.GetSafeNormal()));
		Material->SetVectorParameterValue(TEXT("Center"), FLinearColor(GetActorLocation()));
		// Ihre Hand auf dem Bauch hält Licht ab: ein weicher Schatten dort, wo sie liegt
		const float Press = MotherTouchAge < 0.0f ? 0.0f : FMath::SmoothStep(0.0f, 0.8f, MotherTouchAge)
			* (1.0f - FMath::SmoothStep(MotherTouchSeconds - 1.5f, MotherTouchSeconds, MotherTouchAge));
		Material->SetVectorParameterValue(TEXT("HandDirection"), FLinearColor(MotherTouchDirection));
		Material->SetScalarParameterValue(TEXT("HandPress"), Press);
	}

	UpdateCamera(DeltaSeconds, Events);
	UpdateOwnHand(DeltaSeconds);
	UpdateSound(DeltaSeconds);

	// Diese Kamera ist das Kind
	if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (Controller->GetViewTarget() != this)
		{
			Controller->SetViewTarget(this);
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
			}
		}
	}
}

void AGenesisWombScene::UpdateCamera(float DeltaSeconds, const TArray<EGenesisFetalEvent>& Events)
{
	const float Radius = Perception.CavityRadiusCm;
	// Eigene Bewegungen als Ruck: Schreck stark, Tritt mittel, Schluckauf klein und nach oben
	for (EGenesisFetalEvent Event : Events)
	{
		float Strength = 0.0f;
		FVector Direction = FVector(Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-0.5f, 0.5f));
		switch (Event)
		{
		case EGenesisFetalEvent::Startle: Strength = 0.06f; break;
		case EGenesisFetalEvent::Kick:    Strength = 0.025f; break;
		case EGenesisFetalEvent::Stretch: Strength = 0.03f; break;
		case EGenesisFetalEvent::Hiccup:  Strength = 0.012f; Direction = FVector(0.0, 0.0, 1.0); break;
		case EGenesisFetalEvent::HeadTurn: Strength = 0.02f; break;
		default: break;
		}
		if (Strength > JoltStrength * FMath::Clamp(1.0f - JoltSeconds / 0.6f, 0.0f, 1.0f))
		{
			JoltStrength = Strength;
			JoltDirection = Direction.GetSafeNormal();
			JoltSeconds = 0.0f;
		}
	}
	JoltSeconds += DeltaSeconds;
	const float Decay = FMath::Exp(-JoltSeconds / 0.18f) * FMath::Sin(FMath::Min(JoltSeconds / 0.08f, PI));
	const FVector Jolt = JoltDirection * JoltStrength * Radius * Decay;

	// Wiegen, wenn sie geht (Schrittfrequenz ~1,8 Hz), dazu ihr Atem (~0,25 Hz)
	const float Walk = Mother.Rocking * 0.03f * Radius * FMath::Sin(2.0f * PI * 1.8f * SceneSeconds);
	const float Breath = 0.01f * Radius * FMath::Sin(2.0f * PI * 0.25f * SceneSeconds);
	// Strecken: der Körper schiebt sich nach hinten, der Kopf geht in den Nacken; Gähnen: Kopf zurück, Mund auf
	const float Room = GenesisFetalLogic::RoomToMove(Weeks, GenesisFetalLogic::GetReference().Milestones);
	const float Stretch = StretchAge < 0.0f ? 0.0f : FMath::Sin(PI * FMath::Clamp(StretchAge / 1.6f, 0.0f, 1.0f));
	const float Yawn = YawnAge < 0.0f ? 0.0f : FMath::Sin(PI * FMath::Clamp(YawnAge / 2.2f, 0.0f, 1.0f));
	// Ihre Hand drückt: das Kind weicht ein wenig aus
	const float Press = MotherTouchAge < 0.0f ? 0.0f : FMath::SmoothStep(0.0f, 0.8f, MotherTouchAge)
		* (1.0f - FMath::SmoothStep(MotherTouchSeconds - 1.5f, MotherTouchSeconds, MotherTouchAge));
	const FVector Pushed = -MotherTouchDirection * 0.04f * Radius * Press;
	// Lage des Kindes: längs in der Gebärmutter, das Gesicht zum Bauch der Mutter (dorthin kommt das Licht). Bis zum
	// letzten Drittel liegt es oft mit dem Kopf oben, dann dreht es sich: Beckenendlage bei SSW 28 noch ~20 %, am Termin
	// 3–4 % (RCOG 2017) – hier ab SSW 32 kopfunter, die Drehung über zwei Wochen.
	const float HeadDown = FMath::SmoothStep(31.0f, 33.0f, Weeks);
	const FVector HeadUp = FVector(0.0f, 0.0f, 1.0f).RotateAngleAxis(180.0f * HeadDown, FVector(1.0f, 0.0f, 0.0f));
	FQuat BodyFrame = FRotationMatrix::MakeFromXZ(LookDirection.GetSafeNormal(), HeadUp).ToQuat();
	const FGenesisFetusStage* Stage = CurrentFetusStage();
	if (Stage && Stage->Hull.Num() > 3)
	{
		// Die Längsachse des Körpers (Kopf bis Steiß) liegt längs in der Gebärmutter, das Gesicht so weit es geht zum Bauch
		BodyFrame = FetusOrientation(*Stage, HeadUp);
	}
	const FRotator BodyLook = (BodyFrame * FRotator(-6.0f + 3.0f * FMath::Sin(0.07f * SceneSeconds), 8.0f * FMath::Sin(0.05f * SceneSeconds), 0.0f).Quaternion()).Rotator();
	// Die Augen liegen dort, wo der Körper des Kindes in der Höhle Platz hat: seine Mitte in der Mitte der Höhle
	FVector Eyes(-0.25f * Radius, 0.0f, 0.1f * Radius);
	if (Stage)
	{
		// Die Lage nur bei Wechsel von Woche oder Drehung neu suchen (die Wackel-Bewegungen des Kopfes bleiben klein)
		if (FMath::Abs(Weeks - FittedWeeks) > 0.05f || FMath::Abs(HeadDown - FittedHeadDown) > 0.02f)
		{
			FittedEyes = FitFetus(*Stage, BodyFrame, Radius);
			FittedWeeks = Weeks;
			FittedHeadDown = HeadDown;
		}
		Eyes = FittedEyes;
	}
	const FVector Head = Eyes + FVector(-0.04f * Radius * Room * Stretch, BodyOffset.X * Radius, BodyOffset.Y * Radius + Walk + Breath);
	const FVector FirstPersonLocation = (Head + Jolt + Pushed) * WorldScale;
	const FRotator Look = (BodyLook.Quaternion() * FRotator(HeadLook.Y + 5.0f * Stretch + 8.0f * Yawn, HeadLook.X, 0.0f).Quaternion()).Rotator();
	const FRotator FirstPersonRotation = (Look.Quaternion() * FRotator(Jolt.Z * 20.0f / FMath::Max(1.0f, Radius), Jolt.Y * 20.0f / FMath::Max(1.0f, Radius), 0.0f).Quaternion()).Rotator();
	// Das Kind: Ursprung zwischen den Augen, +X der Blick – es bewegt sich mit (Atem, Wiegen, eigene Rucke)
	if (Stage && Fetus)
	{
		if (Fetus->GetStaticMesh() != Stage->Mesh)
		{
			Fetus->SetStaticMesh(Stage->Mesh);
		}
		Fetus->SetRelativeLocationAndRotation((Head + Jolt + Pushed) * WorldScale, BodyLook);
		Fetus->SetRelativeScale3D(FVector(FetusScale * WorldScale));
		NavelWorld = Fetus->GetComponentTransform().TransformPosition(Stage->Navel);
		bHasNavel = true;
	}
	UpdateExterior(DeltaSeconds, FirstPersonLocation, FirstPersonRotation);

	// Wahrnehmung: Das Auge im Dunkeln passt sich an die Lichtmenge an (die Wand leuchtet linear mit den lx) –
	// wie hell es sich anfühlt, folgt der Wahrnehmung (logarithmisch, Lider, Pupille). Was nicht ankommt, bleibt
	// dunkel; vor dem bewussten Erleben (Thalamus–Rinde) gedämpft.
	const float Adapted = -FMath::Log2(FMath::Max(0.05f, Mother.WombLux) / ReferenceLux);
	const float Felt = FMath::Log2(FMath::Max(0.01f, Perception.Brightness) / ReferenceBrightness);
	if (ExteriorAge >= 0.0f)
	{
		// Von außen: ein Beobachter mit gewöhnlichen Augen – die Wahrnehmung des Kindes beginnt erst in seinen Augen
		const float Inside = ExteriorStaySeconds > 0.0f ? 0.0f : FMath::SmoothStep(ExteriorSeconds - 1.2f, ExteriorSeconds, ExteriorAge);
		// Mit Kaltlicht belichtet der Beobachter auf dessen Licht, nicht auf das (fast fehlende) Licht durch den Bauch
		const float Observed = ExteriorStaySeconds > 0.0f ? -FMath::Log2(FMath::Max(Mother.WombLux, ScopeLux) / ReferenceLux) : Adapted;
		// +1,8 EV gilt für das schwache Durchlicht des Bauchs; unter Kaltlicht brannte damit die Haut weiß aus (P90 0,94)
		const float Lift = ExteriorStaySeconds > 0.0f ? 0.3f : 1.8f;
		Camera->PostProcessSettings.AutoExposureBias = ExposureBias + Observed + FMath::Lerp(Lift, Felt - 2.0f * (1.0f - Perception.Presence), Inside);
		Camera->PostProcessSettings.bOverride_ColorOffset = true;
		Camera->PostProcessSettings.ColorOffset = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
		Camera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
		Camera->PostProcessSettings.FilmGrainIntensity = 0.0f;
		Camera->PostProcessSettings.bOverride_ColorSaturation = true;
		Camera->PostProcessSettings.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, 0.95f);
		return;
	}
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias + Adapted + Felt - 2.0f * (1.0f - Perception.Presence);
	// Eigengrau: Auch ohne Licht ist das Dunkel nicht schwarz, sondern ein schwaches, rauschendes Dunkelrotbraun
	const float Dark = 1.0f - FMath::Clamp(Perception.Brightness / 0.3f, 0.0f, 1.0f);
	Camera->PostProcessSettings.bOverride_ColorOffset = true;
	Camera->PostProcessSettings.ColorOffset = FVector4(0.03f, 0.021f, 0.019f, 0.0f) * Dark * (0.4f + 0.6f * Perception.Presence)
		* (0.85f + 0.15f * FMath::Sin(2.0f * PI * 0.07f * SceneSeconds));
	Camera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
	Camera->PostProcessSettings.FilmGrainIntensity = 0.3f * Dark;
	Camera->PostProcessSettings.bOverride_FilmGrainIntensityShadows = true;
	Camera->PostProcessSettings.FilmGrainIntensityShadows = 1.0f;
	// Unschärfe: Ein Fetus sieht Flächen und große Formen, keine Einzelheiten. Hinter geschlossenen Lidern nur
	// diffuses Leuchten (Fokus dicht vor dem Auge), mit offenen Augen das Grobe wenige Zentimeter vor dem Gesicht.
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.ManualFocusDistance = FMath::Lerp(0.6f, 4.0f, Perception.EyesOpen) * WorldScale;
	Camera->SetFocusSettings(Focus);
	// Gleiche Unschärfe wie im echten Maßstab: Öffnung (Brennweite / Blende) mit dem Maßstab vergrößern
	Camera->SetCurrentAperture(FMath::Lerp(5.6f, 1.4f, Perception.Blur) / FMath::Max(1.0f, WorldScale));
	Camera->PostProcessSettings.bOverride_ColorSaturation = true;
	Camera->PostProcessSettings.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, FMath::Lerp(0.5f, 0.9f, Perception.EyesOpen));
}

const FGenesisFetusStage* AGenesisWombScene::CurrentFetusStage()
{
	// Das nächstkleinere gebaute Alter, dazwischen wächst das Kind stetig (Scheitel-Steiß-Länge)
	const FGenesisFetusStage* Best = nullptr;
	const FGenesisFetusStage* Next = nullptr;
	for (const FGenesisFetusStage& Stage : FetusStages)
	{
		if (!Stage.Mesh) { continue; }
		if (Stage.Weeks <= Weeks && (!Best || Stage.Weeks > Best->Weeks)) { Best = &Stage; }
		if (Stage.Weeks > Weeks && (!Next || Stage.Weeks < Next->Weeks)) { Next = &Stage; }
	}
	if (!Best) { Best = Next; }
	if (!Best)
	{
		return nullptr;
	}
	float Length = Best->CrownRumpCm;
	if (Next && Next != Best && Next->Weeks > Best->Weeks)
	{
		Length = FMath::Lerp(Best->CrownRumpCm, Next->CrownRumpCm, FMath::Clamp((Weeks - Best->Weeks) / (Next->Weeks - Best->Weeks), 0.0f, 1.0f));
	}
	FetusScale = Length / FMath::Max(0.1f, Best->CrownRumpCm);
	return Best;
}

FQuat AGenesisWombScene::FetusOrientation(const FGenesisFetusStage& Stage, const FVector& HeadUp) const
{
	// Hauptachse der Hülle (Potenzverfahren auf der Kovarianz), gerichtet von der Körpermitte zu den Augen (zum Kopf)
	FVector Mid = FVector::ZeroVector;
	for (const FVector& P : Stage.Hull) { Mid += P; }
	Mid /= Stage.Hull.Num();
	FMatrix Cov(ForceInitToZero);
	for (const FVector& P : Stage.Hull)
	{
		const FVector D = P - Mid;
		for (int32 R = 0; R < 3; ++R)
		{
			for (int32 C = 0; C < 3; ++C)
			{
				Cov.M[R][C] += D[R] * D[C];
			}
		}
	}
	FVector Axis(0.2f, 0.1f, 1.0f);
	for (int32 Iteration = 0; Iteration < 40; ++Iteration)
	{
		Axis = FVector(Cov.M[0][0] * Axis.X + Cov.M[0][1] * Axis.Y + Cov.M[0][2] * Axis.Z,
			Cov.M[1][0] * Axis.X + Cov.M[1][1] * Axis.Y + Cov.M[1][2] * Axis.Z,
			Cov.M[2][0] * Axis.X + Cov.M[2][1] * Axis.Y + Cov.M[2][2] * Axis.Z).GetSafeNormal();
	}
	if (FVector::DotProduct(Axis, -Mid) < 0.0f)
	{
		Axis = -Axis;                                   // die Augen liegen am Kopfende
	}
	// Im Körper: Achse zum Kopf und Blickrichtung (senkrecht dazu); in der Höhle: nach oben bzw. unten und zum Bauch
	const FVector Belly = (LookDirection - HeadUp * FVector::DotProduct(LookDirection, HeadUp)).GetSafeNormal();
	const FQuat FromBody = FRotationMatrix::MakeFromZX(Axis, FVector::ForwardVector).ToQuat();     // die Körperachse exakt
	const FQuat ToCavity = FRotationMatrix::MakeFromZX(HeadUp, Belly).ToQuat();
	return ToCavity * FromBody.Inverse();
}

FVector AGenesisWombScene::FitFetus(const FGenesisFetusStage& Stage, const FQuat& Orientation, float Radius) const
{
	// Höhle als Ellipsoid (Blender build_mutterleib.py: 8,6 × 8,2 × 10,5 cm bei 10 cm), mit Rand für Wand und Plazenta
	const FVector Semi = FVector(8.6f, 8.2f, 10.5f) * (Radius / 10.0f) * 0.96f;
	TArray<FVector> Points;
	for (const FVector& P : Stage.Hull)
	{
		Points.Add(Orientation.RotateVector(P * FetusScale));
	}
	if (Points.Num() == 0)
	{
		return -Orientation.RotateVector(Stage.Center * FetusScale);
	}
	// Start: die Mitte des Körpers (aus der Hülle) in die Mitte der Höhle
	FVector Mid = FVector::ZeroVector;
	for (const FVector& P : Points) { Mid += P; }
	FVector Eyes = -Mid / Points.Num();
	// Dann schrittweise vom am weitesten herausragenden Punkt weg – gedämpft, damit nichts aufschaukelt
	float Worst = 0.0f;
	for (int32 Iteration = 0; Iteration < 200; ++Iteration)
	{
		Worst = 0.0f;
		FVector WorstQ = FVector::ZeroVector;
		for (const FVector& P : Points)
		{
			const FVector Q = (P + Eyes) / Semi;
			const float Out = Q.Size();
			if (Out > Worst) { Worst = Out; WorstQ = Q; }
		}
		if (Worst <= 1.0f)
		{
			break;
		}
		const FVector Step = (WorstQ / Worst) * Semi * FMath::Min(Worst - 1.0f, 0.2f) * 0.3f;
		Eyes -= Step;
	}
	if (!Eyes.ContainsNaN() && Worst > 1.02f)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Mutterleib: Das Kind (SSW %.1f) passt nicht ganz in die Höhle (%.0f %% zu groß)."), Weeks, (Worst - 1.0f) * 100.0f);
	}
	if (Eyes.ContainsNaN())
	{
		Eyes = -Mid / Points.Num();
	}
	return Eyes;
}

void AGenesisWombScene::StartExterior(float StaySeconds)
{
	if (CurrentFetusStage())
	{
		ExteriorAge = 0.0f;
		ExteriorSpin = Random.FRandRange(-1.0f, 1.0f);
		ExteriorStaySeconds = FMath::Max(0.0f, StaySeconds);
	}
}

void AGenesisWombScene::UpdateExterior(float DeltaSeconds, const FVector& FirstPersonLocation, const FRotator& FirstPersonRotation)
{
	const FGenesisFetusStage* Stage = CurrentFetusStage();
	if (ExteriorAge < 0.0f || !Stage || !Fetus)
	{
		if (ScopeLight) { ScopeLight->SetVisibility(false); }
		Camera->SetRelativeLocation(FirstPersonLocation);
		Camera->SetRelativeRotation(FirstPersonRotation);
		if (Fetus) { Fetus->SetVisibility(false); }
		if (OwnHand) { OwnHand->SetVisibility(true); }
		return;
	}
	// Von außen: die Kamera schwebt langsam um das Kind (wie eine Fetoskop-Aufnahme, nur mit dem Licht des Bauchs),
	// dann gleitet sie zu seinem Gesicht und in seine Augen. Dort übernimmt die Ich-Sicht.
	ExteriorAge += DeltaSeconds;
	const float Radius = Perception.CavityRadiusCm;
	const FVector Center = (Fetus->GetRelativeLocation() / WorldScale) + Fetus->GetRelativeRotation().RotateVector(Stage->Center * FetusScale);
	const FVector Face = Fetus->GetRelativeLocation() / WorldScale;
	const float Size = Stage->CrownRumpCm * FetusScale;
	// Bleibt die Kamera draußen (frühe Wochen), kreist sie über den ganzen Moment, statt nach 6,5 s ins Gesicht zu gleiten
	const bool bStay = ExteriorStaySeconds > 0.0f;
	const float Orbit = FMath::Clamp(ExteriorAge / (bStay ? ExteriorStaySeconds : ExteriorSeconds - 2.5f), 0.0f, 1.0f);
	const FRotator Gaze = Fetus->GetRelativeRotation();
	// vorn-seitlich vor dem Kind, das Gesicht im Blick; innerhalb der Höhle
	const FVector Around = Gaze.RotateVector(FVector(1.0f, 0.0f, 0.25f)).RotateAngleAxis(-45.0f + (25.0f + 20.0f * ExteriorSpin) * Orbit, FVector::UpVector).GetSafeNormal();
	// Wie ein 3D-Ultraschall: Der Blick geht von außen durch die Wand (sie ist nur von innen sichtbar) auf das ganze Kind,
	// langsam näher an sein Gesicht
	const float Distance = FMath::Lerp(1.6f, 0.9f, Orbit) * Size;
	const FVector Look = FMath::Lerp(Center, Face, 0.35f + 0.4f * Orbit);
	const FVector Location = Look + Around * Distance;
	const FRotator Rotation = (Look - Location).Rotation();
	// Hineinfahren: die letzten 2,5 s zum Gesicht und durch die Augen in die Ich-Sicht
	const float Dive = bStay ? 0.0f : FMath::SmoothStep(ExteriorSeconds - 2.5f, ExteriorSeconds, ExteriorAge);
	const FVector FinalLocation = FMath::Lerp(Location * WorldScale, FirstPersonLocation, Dive);
	const FQuat FinalRotation = FQuat::Slerp(Rotation.Quaternion(), FirstPersonRotation.Quaternion(), Dive);
	Camera->SetRelativeLocation(FinalLocation);
	Camera->SetRelativeRotation(FinalRotation.Rotator());
	// Schärfe auf das Gesicht; beim Eintauchen in die Augen wird es rot und weich (die Lider)
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.ManualFocusDistance = FMath::Max(0.5f * WorldScale, FVector::Dist(FinalLocation, Face * WorldScale));
	Camera->SetFocusSettings(Focus);
	Camera->SetCurrentAperture(2.8f / FMath::Max(1.0f, WorldScale));
	// Kaltlicht: gleiche Beleuchtungsstärke am Gesicht, egal wie nah die Kamera kreist (I = E · d², d in m)
	if (ScopeLight)
	{
		const float DistanceMeters = FVector::Dist(FinalLocation, Face * WorldScale) / 100.0f;
		ScopeLight->SetVisibility(bStay);
		ScopeLight->SetIntensity(bStay ? ScopeLux * DistanceMeters * DistanceMeters : 0.0f);
		ScopeLight->SetAttenuationRadius(FMath::Max(10.0f, 4.0f * DistanceMeters * 100.0f));
	}
	Fetus->SetVisibility(Dive < 0.92f);
	if (OwnHand) { OwnHand->SetVisibility(false); }
	if (!bStay && ExteriorAge >= ExteriorSeconds)
	{
		ExteriorAge = -1.0f;
		Fetus->SetVisibility(false);
		if (OwnHand) { OwnHand->SetVisibility(true); }
	}
}

void AGenesisWombScene::DebugAction(const FString& Action, const FVector2D& Value)
{
	// Wie ein Tastendruck: wirkt über dieselben Wege wie die Eingabe des Spielers
	if (Action == TEXT("kick"))         { ++DebugInput.Kicks; }
	else if (Action == TEXT("stretch")) { ++DebugInput.StretchTaps; }
	else if (Action == TEXT("yawn"))    { DebugHoldSeconds = 1.0f; DebugHold = TEXT("stretch"); }
	else if (Action == TEXT("thumb"))   { DebugHoldSeconds = 6.0f; DebugHold = TEXT("mouth"); }
	else if (Action == TEXT("swallow")) { ++DebugInput.MouthTaps; }
	else if (Action == TEXT("grasp"))   { DebugHoldSeconds = 6.0f; DebugHold = TEXT("grasp"); }
	else if (Action == TEXT("eyes"))    { ++DebugInput.EyeToggles; }
	else if (Action == TEXT("touch"))   { MotherTouchIn = 0.01f; MotherTouchAge = -1.0f; }
	else if (Action == TEXT("hand"))    { HandPosition = Value; UsedActions.Add(EGenesisFetalAction::MoveHand); }
}

bool AGenesisWombScene::Can(EGenesisFetalAction Action) const
{
	return GenesisFetalLogic::CanPerform(Action, Weeks, GenesisFetalLogic::GetReference().Milestones);
}

void AGenesisWombScene::ShowCaption(const FString& Text)
{
	Caption = Text;
	CaptionAge = 0.0f;
}

FString AGenesisWombScene::GetCaption(float& OutAlpha) const
{
	OutAlpha = FMath::Clamp(CaptionAge / 0.5f, 0.0f, 1.0f) * FMath::Clamp((6.0f - CaptionAge) / 1.5f, 0.0f, 1.0f);
	return Caption;
}

void AGenesisWombScene::Feel(float Intensity, float Seconds, bool bLarge)
{
	// Tasten ist der erste Sinn (Mund ab SSW 8, der ganze Körper bis ~14): was das Kind spürt, spürt der Spieler in den Händen
	const float Felt = Intensity * FetalNow.Touch;
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisFrontendSubsystem* Frontend = GameInstance ? GameInstance->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr;
	APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Felt < 0.01f || !Controller || (Frontend && !Frontend->GetSettings().bVibration))
	{
		return;
	}
	Controller->PlayDynamicForceFeedback(FMath::Clamp(Felt, 0.0f, 1.0f), Seconds, bLarge, !bLarge, bLarge, !bLarge);
}

TArray<FGenesisWombHint> AGenesisWombScene::GetHints() const
{
	// Nur, was der Körper in dieser Woche kann (Docs/37)
	TArray<FGenesisWombHint> Hints;
	auto Add = [this, &Hints](EGenesisFetalAction Action, const FString& Keys, const FString& Verb)
	{
		if (Can(Action))
		{
			FGenesisWombHint& Hint = Hints.AddDefaulted_GetRef();
			Hint.Verb = Keys + TEXT(": ") + Verb;
			Hint.bUsed = UsedActions.Contains(Action);
		}
	};
	auto Keys = [](const TCHAR* Action) { return AGenesisSlicePlayerController::DescribeAction(Action); };
	Add(EGenesisFetalAction::MoveBody, TEXT("WASD / linker Stick"), TEXT("sich bewegen"));
	Add(EGenesisFetalAction::TurnHead, TEXT("Maus / rechter Stick"), TEXT("den Kopf drehen"));
	Add(EGenesisFetalAction::MoveHand, Keys(TEXT("GenesisWombHand")) + TEXT(" halten + bewegen"), TEXT("die Hand"));
	Add(EGenesisFetalAction::Kick, Keys(TEXT("GenesisKick")), TEXT("strampeln"));
	Add(EGenesisFetalAction::Stretch, Keys(TEXT("GenesisStretch")), Can(EGenesisFetalAction::Yawn) ? TEXT("strecken, halten: gähnen") : TEXT("strecken"));
	Add(EGenesisFetalAction::HandToMouth, Keys(TEXT("GenesisMouth")), Can(EGenesisFetalAction::Swallow) ? TEXT("halten: Daumen zum Mund, tippen: schlucken") : TEXT("Hand zum Mund"));
	Add(EGenesisFetalAction::Grasp, Keys(TEXT("GenesisGrasp")), TEXT("greifen"));
	Add(EGenesisFetalAction::OpenEyes, Keys(TEXT("GenesisEyes")), TEXT("Augen auf / zu"));
	return Hints;
}

void AGenesisWombScene::UpdatePlayer(float DeltaSeconds, const FGenesisFetalView& Fetal, TArray<EGenesisFetalEvent>& Events)
{
	// Entwicklerbefehle wirken wie Tasten (genesis.Womb.Do)
	Input.Kicks += DebugInput.Kicks;
	Input.StretchTaps += DebugInput.StretchTaps;
	Input.MouthTaps += DebugInput.MouthTaps;
	Input.EyeToggles += DebugInput.EyeToggles;
	DebugInput = FGenesisWombInput();
	if (DebugHoldSeconds > 0.0f)
	{
		DebugHeld += DeltaSeconds;
		Input.MouthHeldSeconds = DebugHold == TEXT("mouth") ? DebugHeld : Input.MouthHeldSeconds;
		Input.StretchHeldSeconds = DebugHold == TEXT("stretch") ? DebugHeld : Input.StretchHeldSeconds;
		Input.bGraspHeld |= DebugHold == TEXT("grasp");
		DebugHoldSeconds -= DeltaSeconds;
	}
	else
	{
		DebugHeld = 0.0f;
	}

	const float Room = GenesisFetalLogic::RoomToMove(Weeks, GenesisFetalLogic::GetReference().Milestones);
	const FVector2D Move(FMath::Clamp(Input.Move.X, -1.0f, 1.0f), FMath::Clamp(Input.Move.Y, -1.0f, 1.0f));
	CaptionAge += DeltaSeconds;

	// Sich bewegen – oder, mit gehaltener Hand-Taste, die Hand. Das Kind schwebt; spät im Drittel wird es eng.
	if (Input.bHandHeld && Can(EGenesisFetalAction::MoveHand))
	{
		HandPosition.X = FMath::Clamp(HandPosition.X + Move.X * 1.2f * DeltaSeconds, -1.0f, 1.0f);
		HandPosition.Y = FMath::Clamp(HandPosition.Y + Move.Y * 1.2f * DeltaSeconds, -1.0f, 1.0f);
		if (!Move.IsNearlyZero(0.2f)) { UsedActions.Add(EGenesisFetalAction::MoveHand); }
	}
	else if (Can(EGenesisFetalAction::MoveBody))
	{
		BodyOffset = FMath::Vector2DInterpTo(BodyOffset, Move * 0.12f * Room, DeltaSeconds, 1.2f);
		if (!Move.IsNearlyZero(0.2f)) { UsedActions.Add(EGenesisFetalAction::MoveBody); }
	}
	if (Can(EGenesisFetalAction::TurnHead))
	{
		const float Limit = FMath::Lerp(18.0f, 40.0f, Room);
		HeadLook.X = FMath::Clamp(HeadLook.X + Input.Look.X * 50.0f * DeltaSeconds, -Limit, Limit);
		HeadLook.Y = FMath::Clamp(HeadLook.Y + Input.Look.Y * 35.0f * DeltaSeconds, -0.6f * Limit, 0.6f * Limit);
		if (!Input.Look.IsNearlyZero(0.1f)) { UsedActions.Add(EGenesisFetalAction::TurnHead); }
	}
	// Die Nackenmuskeln halten den Kopf nicht lange: er kehrt langsam zurück
	HeadLook = FMath::Vector2DInterpTo(HeadLook, FVector2D::ZeroVector, DeltaSeconds, 0.25f);

	// Strampeln – die Mutter spürt es ab SSW ~20
	for (int32 Kick = 0; Kick < Input.Kicks; ++Kick)
	{
		if (Can(EGenesisFetalAction::Kick))
		{
			Events.Add(EGenesisFetalEvent::Kick);
			UsedActions.Add(EGenesisFetalAction::Kick);
			Feel(0.45f, 0.18f);
		}
	}

	// Strecken (tippen), gähnen (halten)
	if (Input.StretchTaps > 0 && Can(EGenesisFetalAction::Stretch) && StretchAge < 0.0f)
	{
		StretchAge = 0.0f;
		UsedActions.Add(EGenesisFetalAction::Stretch);
		Feel(0.2f, 0.6f);
	}
	if (Input.StretchHeldSeconds > 0.6f && YawnAge < 0.0f && Can(EGenesisFetalAction::Yawn))
	{
		YawnAge = 0.0f;
		UsedActions.Add(EGenesisFetalAction::Yawn);
	}
	StretchAge = StretchAge < 0.0f ? StretchAge : (StretchAge + DeltaSeconds > 1.6f ? -1.0f : StretchAge + DeltaSeconds);
	YawnAge = YawnAge < 0.0f ? YawnAge : (YawnAge + DeltaSeconds > 2.2f ? -1.0f : YawnAge + DeltaSeconds);

	// Hand zum Mund, am Daumen saugen (Mund ist die erste Stelle, die tastet)
	const bool bToMouth = Input.MouthHeldSeconds > 0.25f && Can(EGenesisFetalAction::HandToMouth);
	// Hand zum Mund dauert beim Fetus 1,6–2 s, gut zwei Drittel davon Abbremsen (Zoia 2013): schneller Anfang, langes Ankommen
	MouthProgress = FMath::FInterpConstantTo(MouthProgress, bToMouth ? 1.0f : 0.0f, DeltaSeconds, 1.0f / 1.8f);
	HandToMouth = bToMouth ? 1.0f - FMath::Pow(1.0f - MouthProgress, 2.3f) : FMath::Pow(MouthProgress, 1.6f);
	if (bToMouth) { UsedActions.Add(EGenesisFetalAction::HandToMouth); }
	if (HandToMouth > 0.95f && !bThumbFelt)
	{
		bThumbFelt = true;
		Feel(0.25f, 0.3f, false);
		if (!CaptionsShown.Contains(TEXT("Daumen")))
		{
			CaptionsShown.Add(TEXT("Daumen"));
			ShowCaption(Can(EGenesisFetalAction::Swallow) ? TEXT("Der Daumen am Mund – das Kind saugt.") : TEXT("Die Hand am Mund – die Lippen spüren sie."));
		}
	}
	bThumbFelt &= HandToMouth > 0.5f;

	// Schlucken – und schmecken, was sie gegessen hat
	for (int32 Tap = 0; Tap < Input.MouthTaps; ++Tap)
	{
		if (!Can(EGenesisFetalAction::Swallow))
		{
			continue;
		}
		UsedActions.Add(EGenesisFetalAction::Swallow);
		Events.Add(EGenesisFetalEvent::Swallow);
		Feel(0.12f, 0.15f, false);
		if (Mother.FlavorStrength > 0.15f)
		{
			ShowCaption(FString::Printf(TEXT("Das Fruchtwasser schmeckt %s."), *GenesisMotherDay::GetFlavorName(Mother.Flavor)));
		}
		else if (!CaptionsShown.Contains(TEXT("Fruchtwasser")))
		{
			CaptionsShown.Add(TEXT("Fruchtwasser"));
			ShowCaption(TEXT("Fruchtwasser – warm, ein wenig salzig."));
		}
	}

	// Greifen: die Hand schließt sich (Greifreflex, 3. Trimenon)
	// Nur kurzes Greifen der Nabelschnur ist als normal beschrieben; anhaltendes Festhalten kommt in Fallberichten mit
	// gestörtem Blutfluss vor (Habek 2002, Jakobovits 2007). Die Hand lässt deshalb nach einigen Sekunden von selbst los –
	// greifen kann das Kind erst wieder, wenn der Spieler neu ansetzt.
	HoldSeconds = bHoldingCord ? HoldSeconds + DeltaSeconds : 0.0f;
	if (bHoldingCord && HoldSeconds > HoldLimit)
	{
		bGripTired = true;
		ShowCaption(TEXT("Die Hand lässt los."));
	}
	bGripTired &= Input.bGraspHeld;
	const bool bGrasp = Input.bGraspHeld && !bGripTired && Can(EGenesisFetalAction::Grasp);
	HandClosed = FMath::FInterpConstantTo(HandClosed, bGrasp ? 1.0f : 0.0f, DeltaSeconds, 3.0f);
	if (bGrasp) { UsedActions.Add(EGenesisFetalAction::Grasp); }
	bGraspIntent = bGrasp;

	// Augen auf und zu – erst, wenn die Lider sich gelöst haben
	for (int32 Toggle = 0; Toggle < Input.EyeToggles; ++Toggle)
	{
		if (Can(EGenesisFetalAction::OpenEyes))
		{
			bLidsClosedByPlayer = !bLidsClosedByPlayer;
			UsedActions.Add(EGenesisFetalAction::OpenEyes);
		}
		else if (!CaptionsShown.Contains(TEXT("Lider")))
		{
			CaptionsShown.Add(TEXT("Lider"));
			ShowCaption(TEXT("Die Lider sind noch verwachsen – sie lösen sich um die 26. Woche."));
		}
	}

	// Wasserlassen: Die Niere arbeitet, der Urin wird Fruchtwasser (Maged 2014). Etwa alle 20–40 Minuten (Näherung).
	if (Weeks >= 12.0f)
	{
		MinutesToUrinate -= DeltaSeconds * 8.0f / 60.0f;
		if (MinutesToUrinate <= 0.0f)
		{
			MinutesToUrinate = Random.FRandRange(20.0f, 40.0f);
			if (Fetal.ConsciousAccess > 0.5f && CaptionAge > 8.0f)
			{
				ShowCaption(TEXT("Warm. Das Kind lässt Wasser – es wird wieder zu Fruchtwasser."));
				Feel(0.05f, 1.2f, false);
			}
		}
	}

	// Ihr Herz: Durch die Bauchschlagader spürt das Kind den Puls der Mutter als feines Klopfen
	HeartbeatPhase += DeltaSeconds * Mother.HeartRateBpm / 60.0f;
	if (HeartbeatPhase >= 1.0f)
	{
		HeartbeatPhase -= 1.0f;
		if (Perception.Presence > 0.5f)
		{
			Feel(0.05f, 0.06f);
		}
	}

	// Ein Tritt, den sie spürt: Sie legt die Hand auf den Bauch (wenn sie wach ist)
	for (EGenesisFetalEvent Event : Events)
	{
		if (Event == EGenesisFetalEvent::Kick && Fetal.bMotherFeelsMovement && Mother.Activity != EGenesisMotherActivity::Sleeping
			&& MotherTouchIn < 0.0f && MotherTouchAge < 0.0f && Random.FRand() < MotherTouchChance)
		{
			MotherTouchIn = Random.FRandRange(1.5f, 4.0f);
		}
	}
	Input = FGenesisWombInput();
}

void AGenesisWombScene::UpdateMotherTouch(float DeltaSeconds, const FGenesisFetalView& Fetal)
{
	if (MotherTouchIn >= 0.0f)
	{
		MotherTouchIn -= DeltaSeconds;
		if (MotherTouchIn < 0.0f)
		{
			// Sie legt die Hand dorthin, wo sie den Tritt gespürt hat – vorn auf den Bauch
			MotherTouchAge = 0.0f;
			// Vorn auf den Bauch, dorthin, wohin das Kind schaut – der Bauch wölbt sich nach vorn, dort spürt sie die Tritte
			const FVector Toward = (BellyDirection.GetSafeNormal() + LookDirection.GetSafeNormal()).GetSafeNormal();
			MotherTouchDirection = Toward.RotateAngleAxis(Random.FRandRange(-15.0f, 15.0f), FVector::UpVector)
				.RotateAngleAxis(Random.FRandRange(-10.0f, 10.0f), FVector::RightVector).GetSafeNormal();
			Feel(0.3f, 0.6f);
			ShowCaption(TEXT("Ihre Hand auf dem Bauch."));
			USoundBase* Line = nullptr;
			if (KickLine && !bKickLineSaid)
			{
				Line = KickLine;
				bKickLineSaid = true;
			}
			else if (BellyLines.Num() > 0 && !MotherVoice->IsPlaying() && Random.FRand() < 0.5f)
			{
				Line = BellyLines[Random.RandRange(0, BellyLines.Num() - 1)];
			}
			if (Line)
			{
				MotherVoice->SetSound(Line);
				MotherVoice->Play();
				SinceVoice = 0.0f;
			}
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Sie spürt den Tritt und legt die Hand auf den Bauch (SSW %.1f)%s."), Weeks,
				Line ? *FString::Printf(TEXT(", spricht %s"), *Line->GetName()) : TEXT(""));
		}
	}
	if (MotherTouchAge >= 0.0f)
	{
		MotherTouchAge += DeltaSeconds;
		if (MotherTouchAge > MotherTouchSeconds)
		{
			MotherTouchAge = -1.0f;
		}
	}
}

void AGenesisWombScene::UpdateSound(float DeltaSeconds)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	const FGenesisHearingPerception Hearing = Audio ? Audio->GetHearingPerception() : FGenesisHearingPerception();

	// Der Ort: ihr Herz, ihr Blut, ihr Darm – so, wie das Kind es hört
	for (TActorIterator<AGenesisWorldSoundActor> It(GetWorld()); It; ++It)
	{
		if (It->Params.Place == EGenesisPlace::Womb && It->WorldSound)
		{
			It->WorldSound->bHeardByUnborn = true;
			FGenesisWorldSoundParams Params = It->WorldSound->GetWorldSoundParams();
			Params.MaternalHeartRateBpm = Mother.HeartRateBpm;
			Params.Digestion = Mother.Digestion;
			Params.Activity = FMath::Clamp(0.2f + 0.6f * Mother.Rocking + 0.3f * Mother.Speaking, 0.0f, 1.0f);
			It->WorldSound->SetWorldSoundParams(Params);
		}
	}

	// Ihre Stimme: durch ihren eigenen Körper, dann durch das Gehör des Kindes
	MotherVoice->SetLowPassFilterEnabled(true);
	MotherVoice->SetLowPassFilterFrequency(FMath::Max(40.0f, Hearing.LowPassCutoffHz));
	MotherVoice->SetHighPassFilterEnabled(Hearing.HighPassCutoffHz > 20.0f);
	MotherVoice->SetHighPassFilterFrequency(FMath::Max(20.0f, Hearing.HighPassCutoffHz));
	MotherVoice->SetVolumeMultiplier(1.2f * GenesisAudioCoreLogic::UnbornPresentationGain(Hearing.BodyAudibility));

	const bool bAwake = Mother.Activity != EGenesisMotherActivity::Sleeping;
	if (bAwake && Weeks >= 16.0f && Mother.Speaking > 0.1f && SinceVoice > 20.0f && !MotherVoice->IsPlaying())
	{
		USoundBase* Line = nullptr;
		if (Weeks >= 36.0f && LateLine && !bLateLineSaid)
		{
			Line = LateLine;
			bLateLineSaid = true;
		}
		else if (BellyLines.Num() > 0 && (Mother.bTalkingToBelly || Random.FRand() < Mother.Speaking * DeltaSeconds * 0.1f || SinceVoice < 21.0f))
		{
			Line = BellyLines[Random.RandRange(0, BellyLines.Num() - 1)];
		}
		if (Line)
		{
			MotherVoice->SetSound(Line);
			MotherVoice->Play();
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: ihre Stimme %s (SSW %.1f, Tiefpass %.0f Hz, Hochpass %.0f Hz, Lautstärke %.2f)"), *Line->GetName(), Weeks,
				Hearing.LowPassCutoffHz, Hearing.HighPassCutoffHz, MotherVoice->VolumeMultiplier);
			SinceVoice = 0.0f;
		}
	}
}
