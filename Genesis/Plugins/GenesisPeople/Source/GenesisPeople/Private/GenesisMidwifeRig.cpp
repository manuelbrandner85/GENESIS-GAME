// GENESIS: Der Kreislauf des Lebens

#include "GenesisMidwifeRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GenesisMotherLogic.h"
#include "GenesisMotherRig.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GenesisLog.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const FName LeftEyeBone(TEXT("FACIAL_L_Eye"));
	const FName RightEyeBone(TEXT("FACIAL_R_Eye"));

	/** Hände locker an den Seiten (Komponentenraum des Körpers, cm) – nicht die A-Pose der Referenz. */
	const FVector RestRightHand(-21.0, 4.0, 86.0);
	const FVector RestLeftHand(21.0, 4.0, 86.0);

	FVector Flat(const FVector& Vector, const FVector& Fallback)
	{
		const FVector Horizontal(Vector.X, Vector.Y, 0.0);
		return Horizontal.IsNearlyZero() ? Fallback : Horizontal.GetSafeNormal();
	}
}

namespace GenesisMidwifeLogic
{
	EGenesisMidwifeTask TaskFor(bool bBorn, bool bOnChest, float SecondsOnChest, float HandingSeconds, float& OutProgress)
	{
		OutProgress = 0.0f;
		if (!bBorn)
		{
			return EGenesisMidwifeTask::Attending;
		}
		if (!bOnChest)
		{
			return EGenesisMidwifeTask::Holding;
		}
		OutProgress = FMath::Clamp(SecondsOnChest / FMath::Max(0.1f, HandingSeconds), 0.0f, 1.0f);
		return OutProgress < 1.0f ? EGenesisMidwifeTask::Handing : EGenesisMidwifeTask::Watching;
	}

	FGenesisMidwifeStance Compute(const FInputs& In, float FaceDistanceMm, float EyeHeightMm)
	{
		FGenesisMidwifeStance Out;
		const FVector ChildForward = Flat(In.ChildForward, FVector::ForwardVector);

		// Sie fängt das Kind am Fußende des Bettes auf und bleibt dort stehen, der Mutter zugewandt. Sie hebt
		// es zu sich hoch (die Kamera des Kindes folgt ihren Händen, GetHeldView) – nicht umgekehrt.
		const FVector HoldFeet = FVector(In.FootOfBed.X, In.FootOfBed.Y, In.FloorZ);
		// Die Hände unter Kopf und Rücken des Kindes: das Köpfchen in der rechten, der Rücken auf der linken
		// (ChildForward zeigt vom Kind zu ihrem Gesicht – „hinter" dem Kind ist von ihr weg)
		const FVector HoldRight = In.ChildEye - FVector(0.0, 0.0, 70.0) - ChildForward * 40.0f;
		const FVector HoldLeft = In.ChildEye - FVector(0.0, 0.0, 230.0) - ChildForward * 10.0f;
		(void)FaceDistanceMm;
		(void)EyeHeightMm;

		switch (In.Task)
		{
		case EGenesisMidwifeTask::Attending:
			Out.Feet = FVector(In.FootOfBed.X, In.FootOfBed.Y, In.FloorZ);
			Out.Facing = Flat(In.MotherEye - Out.Feet, FVector::ForwardVector);
			// Unter der Geburt gilt ihr Blick dem Damm, nicht dem Gesicht der Mutter
			Out.LookAt = In.MotherEye - FVector(0.0, 0.0, 500.0) + (Out.Feet - In.MotherEye) * 0.35;
			Out.LeanDegPerVertebra = 4.0f;
			break;

		case EGenesisMidwifeTask::Holding:
			Out.Feet = HoldFeet;
			Out.Facing = Flat(In.MotherEye - HoldFeet, -ChildForward);
			Out.LookAt = In.ChildEye;
			Out.RightHand = HoldRight;
			Out.LeftHand = HoldLeft;
			Out.HandsOnChild = 1.0f;
			Out.LeanDegPerVertebra = 7.0f;
			break;

		case EGenesisMidwifeTask::Handing:
		{
			// Sie geht mit dem Kind an die Seite des Bettes, die Hände bleiben am Kind
			const float Walk = FMath::SmoothStep(0.0f, 1.0f, In.HandingProgress);
			const FVector Side = FVector(In.Bedside.X, In.Bedside.Y, In.FloorZ);
			Out.Feet = FMath::Lerp(HoldFeet, Side, Walk);
			Out.Facing = Flat(In.ChildEye - Out.Feet, -ChildForward);
			Out.LookAt = In.ChildEye;
			Out.RightHand = HoldRight;
			Out.LeftHand = HoldLeft;
			// Am Ende lässt sie los – die Mutter hält das Kind
			Out.HandsOnChild = 1.0f - FMath::SmoothStep(0.75f, 1.0f, In.HandingProgress);
			Out.LeanDegPerVertebra = FMath::Lerp(7.0f, 5.0f, Walk);
			break;
		}

		case EGenesisMidwifeTask::Watching:
		default:
			Out.Feet = FVector(In.Bedside.X, In.Bedside.Y, In.FloorZ);
			Out.Facing = Flat(In.MotherEye - Out.Feet, FVector::ForwardVector);
			Out.LookAt = In.ChildEye;
			Out.LeanDegPerVertebra = 2.5f;
			break;
		}
		return Out;
	}
}

AGenesisMidwifeRig::AGenesisMidwifeRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// Stehend: keine Beugung in Hüfte und Knie wie bei der Mutter im Bett
	Posture.HipFlexDeg = 0.0f;
	Posture.KneeFlexDeg = 6.0f;
	Posture.SpineSettleDeg = 4.0f;
	Posture.HeadShare = 0.8f;
	Posture.HeadTiltDeg = 5.0f;
	Posture.ElbowPoleRight = FVector(-30.0, -10.0, -30.0);
	Posture.ElbowPoleLeft = FVector(30.0, -10.0, -30.0);
}

void AGenesisMidwifeRig::BeginPlay()
{
	Super::BeginPlay();
	State = GenesisMotherLogic::Begin(static_cast<uint32>(GetUniqueID()) * 2246822519u + 7u);
	// Sie blinzelt ruhiger und lächelt dem Kind zu, nicht dauerhaft
	Tuning.BlinksPerMinute = 14.0f;
	Tuning.RestingSmile = 0.15f;
	Configure();
}

void AGenesisMidwifeRig::Configure()
{
	Body = nullptr;
	Face = nullptr;
	if (!MidwifeActor)
	{
		return;
	}
	TArray<USkeletalMeshComponent*> Meshes;
	MidwifeActor->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetName().StartsWith(TEXT("Body")))
		{
			Body = Mesh;
		}
		else if (Mesh->GetName().StartsWith(TEXT("Face")))
		{
			Face = Mesh;
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		}
	}
	if (!Body)
	{
		return;
	}
	if (Body->GetAnimClass() != UGenesisMotherAnimInstance::StaticClass())
	{
		Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Body->SetAnimInstanceClass(UGenesisMotherAnimInstance::StaticClass());
	}
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->bEnableUpdateRateOptimizations = false;

	// Kasack: das Standard-Kleidungsstück des MetaHuman, umgefärbt. Der Stoff selbst (Webung, Falten, Knötchen)
	// bleibt – nur die Farbe wird Blaugrün, wie Kasacks im deutschen Kreißsaal.
	TArray<USkeletalMeshComponent*> Parts;
	MidwifeActor->GetComponents<USkeletalMeshComponent>(Parts);
	for (USkeletalMeshComponent* Part : Parts)
	{
		if (Part == Body || Part == Face || !Part->GetName().StartsWith(TEXT("SkeletalMesh")))
		{
			continue;
		}
		for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
		{
			if (UMaterialInstanceDynamic* Fabric = Part->CreateAndSetMaterialInstanceDynamic(Slot))
			{
				for (const TCHAR* Parameter : { TEXT("diffuse_color_1"), TEXT("diffuse_color_2"), TEXT("B_diffuse_color_1") })
				{
					Fabric->SetVectorParameterValue(Parameter, ScrubsColor);
				}
			}
		}
	}
	Dress();
}

void AGenesisMidwifeRig::Dress()
{
	// Die Shorts des Standard-Kleidungsstücks bleiben – umgefärbt sind sie der obere Teil der Hose. Der MetaHuman-Körper
	// hat unter ihnen keine Haut (dort ausgeschnitten, damit nichts durchsticht); die Hosenbeine setzen am Saum an.
	auto Layer = [this](const TCHAR* Name, const TCHAR* MaterialPath) -> USkeletalMeshComponent*
	{
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
		if (!Material || !Body->GetSkeletalMeshAsset())
		{
			UE_LOG(LogGenesis, Warning, TEXT("Hebamme: %s fehlt (%s)"), Name, MaterialPath);
			return nullptr;
		}
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(MidwifeActor, Name);
		Mesh->SetSkeletalMesh(Body->GetSkeletalMeshAsset());
		Mesh->SetupAttachment(Body);
		Mesh->RegisterComponent();
		Mesh->SetLeaderPoseComponent(Body);
		// Der Stoff steht bis zu 8 cm vom Körper ab – die Sichtbarkeitsgrenzen müssen das mit abdecken
		Mesh->SetBoundsScale(1.2f);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, Mesh);
		for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
		{
			Mesh->SetMaterial(Slot, Instance);
		}
		return Mesh;
	};
	if (!Trousers)
	{
		Trousers = Layer(TEXT("ScrubTrousers"), TEXT("/Game/Genesis/People/Materials/M_GEN_ScrubTrousers.M_GEN_ScrubTrousers"));
		if (Trousers)
		{
			if (UMaterialInstanceDynamic* Fabric = Cast<UMaterialInstanceDynamic>(Trousers->GetMaterial(0)))
			{
				// Das Oberteil der Hose (MetaHuman-Stoff mit Webtextur) wirkt bei gleicher Farbe etwas heller – angeglichen am Saum
				Fabric->SetVectorParameterValue(TEXT("Color"), ScrubsColor * 1.12f);
			}
		}
	}
	if (!Gloves)
	{
		Gloves = Layer(TEXT("NitrileGloves"), TEXT("/Game/Genesis/People/Materials/M_GEN_NitrileGloves.M_GEN_NitrileGloves"));
	}
}

UGenesisMotherAnimInstance* AGenesisMidwifeRig::GetAnim() const
{
	return Body ? Cast<UGenesisMotherAnimInstance>(Body->GetAnimInstance()) : nullptr;
}

void AGenesisMidwifeRig::SetChild(bool bInBorn, bool bInOnChest, const FVector& EyeLocation, const FQuat& ViewRotation)
{
	bBorn = bInBorn;
	bOnChest = bInOnChest;
	ChildEye = EyeLocation;
	ChildView = ViewRotation;
}

FVector AGenesisMidwifeRig::GetEyeLocation() const
{
	if (Face)
	{
		const int32 Left = Face->GetBoneIndex(LeftEyeBone);
		const int32 Right = Face->GetBoneIndex(RightEyeBone);
		if (Left != INDEX_NONE && Right != INDEX_NONE)
		{
			return 0.5 * (Face->GetBoneTransform(Left).GetLocation() + Face->GetBoneTransform(Right).GetLocation());
		}
	}
	return MidwifeActor ? MidwifeActor->GetActorLocation() + FVector(0.0, 0.0, 1540.0) : GetActorLocation();
}

void AGenesisMidwifeRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Body || !GetAnim())
	{
		Configure();
	}
	if (!Body || !MidwifeActor)
	{
		return;
	}

	OnChestSeconds = bOnChest ? OnChestSeconds + DeltaSeconds : 0.0f;
	BornSeconds = bBorn ? BornSeconds + DeltaSeconds : 0.0f;
	float Progress = 0.0f;
	Task = GenesisMidwifeLogic::TaskFor(bBorn, bOnChest, OnChestSeconds, HandingSeconds, Progress);

	// Wo sie das Kind hält: gut 38 cm vor ihren Augen, etwas unter dem Kinn – das Kind liegt in ihren Händen
	// und schaut zu ihr hoch. Genau auf diesen Abstand sieht ein Neugeborenes am schärfsten.
	const FVector MyEye = GetEyeLocation();
	const FVector HeldEye = MyEye + SmoothedFacing * HoldDistanceMm - FVector(0.0, 0.0, HoldBelowEyesMm);
	HeldView = FTransform(FRotationMatrix::MakeFromXZ(MyEye - HeldEye, FVector::UpVector).ToQuat(), HeldEye);
	// Sie nimmt es in der ersten Sekunde auf und hebt es in zwei Sekunden zu sich hoch
	HoldBlend = Task == EGenesisMidwifeTask::Attending ? 0.0f : FMath::SmoothStep(0.8f, 3.0f, BornSeconds);

	GenesisMidwifeLogic::FInputs In;
	In.Task = Task;
	In.HandingProgress = Progress;
	const bool bHeldByHer = Task == EGenesisMidwifeTask::Holding;
	In.ChildEye = bHeldByHer ? HeldEye : ChildEye;
	In.ChildForward = bHeldByHer ? HeldView.GetRotation().GetForwardVector() : ChildView.GetForwardVector();
	In.FootOfBed = FootOfBed;
	In.FloorZ = FootOfBed.Z;
	In.MotherEye = Mother ? Mother->GetEyeLocation() : FootOfBed + FVector(1000.0, 0.0, 1000.0);
	const FVector MotherRight = Mother ? Mother->GetActorRightVector() : FVector::RightVector;
	In.Bedside = In.MotherEye + MotherRight * BedsideOffsetMm;
	const FGenesisMidwifeStance Stance = GenesisMidwifeLogic::Compute(In);

	// Sie bewegt sich wie ein Mensch, nicht sprunghaft: Schritte und Drehung geglättet (außer beim ersten Bild)
	if (!bPlaced)
	{
		SmoothedFeet = Stance.Feet;
		SmoothedFacing = Stance.Facing;
		bPlaced = true;
	}
	const float Alpha = 1.0f - FMath::Exp(-DeltaSeconds / 0.6f);
	SmoothedFeet = FMath::Lerp(SmoothedFeet, Stance.Feet, Alpha);
	SmoothedFacing = FMath::Lerp(SmoothedFacing, Stance.Facing, Alpha).GetSafeNormal2D();
	// MetaHuman: vorn ist +Y des Körpers
	MidwifeActor->SetActorLocationAndRotation(SmoothedFeet, FRotationMatrix::MakeFromYZ(SmoothedFacing, FVector::UpVector).Rotator());

	GenesisMotherLogic::Advance(State, Tuning, FGenesisMotherInputs(), DeltaSeconds);
	if (UGenesisMotherAnimInstance* Anim = GetAnim())
	{
		FGenesisMotherPosture Pose = Posture;
		Pose.SpineSettleDeg = Stance.LeanDegPerVertebra;
		Anim->Posture = Pose;
		const FTransform Component = Body->GetComponentTransform();
		FGenesisMotherPoseInputs& Inputs = Anim->PoseInputs;
		Inputs.LookTarget = Component.InverseTransformPosition(Stance.LookAt);
		Inputs.RightHandTarget = FMath::Lerp(RestRightHand, Component.InverseTransformPosition(Stance.RightHand), Stance.HandsOnChild);
		Inputs.LeftHandTarget = FMath::Lerp(RestLeftHand, Component.InverseTransformPosition(Stance.LeftHand), Stance.HandsOnChild);
		Inputs.ArmBlend = 1.0f;
		Inputs.BreathLift = GenesisMotherLogic::GetBreathLift(State, Tuning);
		Inputs.BlinkClosure = GenesisMotherLogic::GetBlinkClosure(State, Tuning);
		// Wenn sie das Kind hält, lächelt sie es an
		Inputs.Smile = Task == EGenesisMidwifeTask::Attending ? 0.05f : 0.4f;
		LipSync.Evaluate(GetWorld()->GetAudioTimeSeconds(), Inputs.SpeechCurves);
	}
}

void AGenesisMidwifeRig::Speak(const UGenesisLipSyncTrack* Track)
{
	if (const UWorld* World = GetWorld())
	{
		LipSync.Start(Track, World->GetAudioTimeSeconds());
	}
}

bool AGenesisMidwifeRig::IsSpeaking() const
{
	return GetWorld() && LipSync.IsSpeaking(GetWorld()->GetAudioTimeSeconds());
}

/**
 * genesis.Debug.ViewMidwife [Abstand mm] [Höhe mm] [Seitenwinkel °] – Prüfkamera auf die ganze Hebamme (Kleidung,
 * Hände, Haltung). Das Kind sieht sie nur aus seinen Armen; für die Abnahme der Kleidung braucht es den Blick von außen.
 */
static FAutoConsoleCommandWithWorldAndArgs GenesisViewMidwifeCommand(
	TEXT("genesis.Debug.ViewMidwife"),
	TEXT("genesis.Debug.ViewMidwife [Abstand mm] [Höhe mm] [Winkel °] – Prüfkamera auf die Hebamme"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		TActorIterator<AGenesisMidwifeRig> It(World);
		if (!Controller || !It || !It->MidwifeActor)
		{
			return;
		}
		const float Distance = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 2400.0f;
		const float Height = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 1000.0f;
		const float Angle = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.0f;
		const AActor* Midwife = It->MidwifeActor;
		const FVector Feet = Midwife->GetActorLocation();
		// MetaHuman: vorn ist +Y des Actors
		const FVector Front = Midwife->GetActorRightVector().RotateAngleAxis(Angle, FVector::UpVector);
		const FVector Eye = Feet + Front * Distance + FVector(0.0, 0.0, Height);
		const FVector Target = Feet + FVector(0.0, 0.0, 850.0);
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Eye, (Target - Eye).Rotation());
		if (Camera)
		{
			Camera->GetCameraComponent()->SetFieldOfView(45.0f);
			Controller->SetViewTarget(Camera);
		}
	}));
