// GENESIS: Der Kreislauf des Lebens

#include "GenesisMidwifeRig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GenesisMotherLogic.h"
#include "GenesisPeopleRendering.h"
#include "GenesisMotherRig.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GenesisLog.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarMidwifeDebug(TEXT("genesis.Debug.MidwifeArms"), 0, TEXT("1 = Handziele der Hebamme protokollieren"));

namespace
{
	const FName MidwifeLeftEye(TEXT("FACIAL_L_Eye"));
	const FName MidwifeRightEye(TEXT("FACIAL_R_Eye"));

	/** Hände locker an den Seiten (Komponentenraum des Körpers, cm) – nicht die A-Pose der Referenz. */
	const FVector MidwifeRestRightHand(-21.0, 4.0, 86.0);
	const FVector MidwifeRestLeftHand(21.0, 4.0, 86.0);

	FVector MidwifeFlat(const FVector& Vector, const FVector& Fallback)
	{
		const FVector Horizontal(Vector.X, Vector.Y, 0.0);
		return Horizontal.IsNearlyZero() ? Fallback : Horizontal.GetSafeNormal();
	}
}

namespace GenesisMidwifeLogic
{
	EGenesisMidwifeTask TaskFor(bool bBorn, bool bOnChest, float SecondsOnChest, float HandingSeconds, float& OutProgress,
		float DryingSeconds, float CoveringSeconds)
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
		// Auf die Brust legen, abrubbeln, zudecken – in dieser Reihenfolge, wie in jedem Kreißsaal
		float T = SecondsOnChest;
		const struct { EGenesisMidwifeTask Task; float Seconds; } Steps[] = {
			{ EGenesisMidwifeTask::Handing, HandingSeconds },
			{ EGenesisMidwifeTask::Drying, DryingSeconds },
			{ EGenesisMidwifeTask::Covering, CoveringSeconds } };
		for (const auto& Step : Steps)
		{
			const float Seconds = FMath::Max(0.1f, Step.Seconds);
			if (T < Seconds)
			{
				OutProgress = FMath::Clamp(T / Seconds, 0.0f, 1.0f);
				return Step.Task;
			}
			T -= Seconds;
		}
		OutProgress = 1.0f;
		return EGenesisMidwifeTask::Watching;
	}

	FGenesisMidwifeStance Compute(const FInputs& In, float FaceDistanceMm, float EyeHeightMm)
	{
		FGenesisMidwifeStance Out;
		const FVector ChildForward = MidwifeFlat(In.ChildForward, FVector::ForwardVector);

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
			Out.Facing = MidwifeFlat(In.MotherEye - Out.Feet, FVector::ForwardVector);
			// Unter der Geburt gilt ihr Blick dem Damm, nicht dem Gesicht der Mutter
			Out.LookAt = In.MotherEye - FVector(0.0, 0.0, 500.0) + (Out.Feet - In.MotherEye) * 0.35;
			Out.LeanDegPerVertebra = 4.0f;
			break;

		case EGenesisMidwifeTask::Holding:
			Out.Feet = HoldFeet;
			Out.Facing = MidwifeFlat(In.MotherEye - HoldFeet, -ChildForward);
			Out.LookAt = In.ChildEye;
			Out.RightHand = HoldRight;
			Out.LeftHand = HoldLeft;
			Out.HandsOnChild = 1.0f;
			Out.LeanDegPerVertebra = 2.5f;
			break;

		case EGenesisMidwifeTask::Handing:
		{
			// Sie geht mit dem Kind an die Seite des Bettes, die Hände bleiben am Kind
			const float Walk = FMath::SmoothStep(0.0f, 1.0f, In.HandingProgress);
			const FVector Side = FVector(In.Bedside.X, In.Bedside.Y, In.FloorZ);
			Out.Feet = FMath::Lerp(HoldFeet, Side, Walk);
			Out.Facing = MidwifeFlat(In.ChildEye - Out.Feet, -ChildForward);
			Out.LookAt = In.ChildEye;
			Out.RightHand = HoldRight;
			Out.LeftHand = HoldLeft;
			// Am Ende lässt sie los – die Mutter hält das Kind
			Out.HandsOnChild = 1.0f - FMath::SmoothStep(0.75f, 1.0f, In.HandingProgress);
			Out.LeanDegPerVertebra = FMath::Lerp(2.5f, 5.0f, Walk);
			break;
		}

		case EGenesisMidwifeTask::Drying:
		case EGenesisMidwifeTask::Covering:
		{
			// Neben dem Bett, über das Kind gebeugt. Das Kind liegt bäuchlings, der Körper Richtung Bauch der Mutter
			// (+X), der Rücken oben. Rechte Hand mit dem Tuch auf dem Rücken, die linke am Hinterkopf.
			Out.Feet = FVector(In.Bedside.X, In.Bedside.Y, In.FloorZ);
			Out.Facing = MidwifeFlat(In.ChildEye - Out.Feet, FVector::ForwardVector);
			Out.LookAt = In.ChildEye;
			const FVector Back = In.ChildEye + FVector(110.0, 0.0, 55.0);
			const FVector Head = In.ChildEye + FVector(-25.0, 0.0, 70.0);
			if (In.Task == EGenesisMidwifeTask::Drying)
			{
				// Zügig, aber sanft: gut anderthalb Striche je Sekunde über den Rücken, die Hand am Kopf reibt mit
				const float Stroke = FMath::Sin(In.Time * 2.0f * PI * 1.6f);
				Out.RightHand = Back + FVector(40.0 * Stroke, 0.0, 0.0);
				Out.LeftHand = Head + FVector(0.0, 15.0 * FMath::Sin(In.Time * 2.0f * PI * 1.6f + 1.3f), 0.0);
				Out.LeanDegPerVertebra = 9.0f;
			}
			else
			{
				// Das Tuch kommt von oben: Hände über dem Kind, dann legen sie es über Rücken und Kopf und lassen los
				const float Lay = FMath::SmoothStep(0.0f, 0.7f, In.TaskProgress);
				Out.RightHand = FMath::Lerp(Back + FVector(0.0, 0.0, 180.0), Back, Lay);
				Out.LeftHand = FMath::Lerp(Head + FVector(0.0, 0.0, 180.0), Head, Lay);
				Out.LeanDegPerVertebra = 8.0f;
			}
			Out.HandsOnChild = In.Task == EGenesisMidwifeTask::Covering ? 1.0f - FMath::SmoothStep(0.8f, 1.0f, In.TaskProgress) : 1.0f;
			break;
		}

		case EGenesisMidwifeTask::Watching:
		default:
			Out.Feet = FVector(In.Bedside.X, In.Bedside.Y, In.FloorZ);
			Out.Facing = MidwifeFlat(In.MotherEye - Out.Feet, FVector::ForwardVector);
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
	// Die Ellenbogen hängen nach unten und etwas nach außen, nah am Körper – nicht seitlich hochgestellt
	Posture.ElbowPoleRight = FVector(-18.0, -12.0, -45.0);
	Posture.ElbowPoleLeft = FVector(18.0, -12.0, -45.0);
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
	GenesisPeopleRendering::ScaleHairVoxelsTo(MidwifeActor);
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
	// Kasackhose und Clogs sind eigene Kleidungsstücke (Tools/Blender/Birth/build_scrub_trousers.py: Schnitt aus ihrem
	// Körper gemessen, Stoff physikalisch fallen gelassen, an ihr Skelett gebunden). Die Shorts des Standard-Kleidungs-
	// stücks liegen darunter und werden ausgeblendet – die Hose bedeckt Hüfte und Schritt selbst.
	UMaterialInterface* Hidden = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Genesis/People/Materials/M_GEN_Hidden.M_GEN_Hidden"));
	TArray<USkeletalMeshComponent*> Parts;
	MidwifeActor->GetComponents<USkeletalMeshComponent>(Parts);
	for (USkeletalMeshComponent* Part : Parts)
	{
		if (!Hidden || Part == Body || Part == Face || Part == Trousers || Part == Clogs || Part == Gloves)
		{
			continue;
		}
		const TArray<FName> Slots = Part->GetMaterialSlotNames();
		for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
		{
			const UMaterialInterface* Material = Part->GetMaterial(Slot);
			const FString Name = (Slots.IsValidIndex(Slot) ? Slots[Slot].ToString() : FString()) + (Material ? Material->GetName() : FString());
			if (Name.Contains(TEXT("Short")))
			{
				Part->SetMaterial(Slot, Hidden);
			}
		}
	}

	auto Attach = [this](const TCHAR* Name, USkeletalMesh* Asset, UMaterialInterface* Material, float BoundsScale) -> USkeletalMeshComponent*
	{
		if (!Asset || !Body->GetSkeletalMeshAsset())
		{
			UE_LOG(LogGenesis, Warning, TEXT("Hebamme: %s fehlt"), Name);
			return nullptr;
		}
		USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(MidwifeActor, Name);
		Mesh->SetSkeletalMesh(Asset);
		Mesh->SetupAttachment(Body);
		Mesh->RegisterComponent();
		Mesh->SetLeaderPoseComponent(Body);
		Mesh->SetBoundsScale(BoundsScale);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		if (Material)
		{
			UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, Mesh);
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				Mesh->SetMaterial(Slot, Instance);
			}
		}
		return Mesh;
	};
	if (!Trousers)
	{
		Trousers = Attach(TEXT("ScrubTrousers"), LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Genesis/People/Midwife/SK_GEN_MidwifeTrousers.SK_GEN_MidwifeTrousers")),
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Genesis/People/Materials/M_GEN_ScrubFabric.M_GEN_ScrubFabric")), 1.0f);
		if (Trousers)
		{
			if (UMaterialInstanceDynamic* Fabric = Cast<UMaterialInstanceDynamic>(Trousers->GetMaterial(0)))
			{
				// Kasack und Hose sind ein Satz: dieselbe Farbe
				Fabric->SetVectorParameterValue(TEXT("Color"), ScrubsColor);
			}
		}
	}
	if (!Clogs)
	{
		Clogs = Attach(TEXT("Clogs"), LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Genesis/People/Midwife/SK_GEN_MidwifeClogs.SK_GEN_MidwifeClogs")),
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Genesis/People/Materials/M_GEN_ClogPolymer.M_GEN_ClogPolymer")), 1.0f);
	}
	// Die Handschuhe bleiben eine Hülle auf dem Körper (0,1 mm Nitril folgt der Haut exakt)
	if (!Gloves)
	{
		Gloves = Attach(TEXT("NitrileGloves"), Body->GetSkeletalMeshAsset(),
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Genesis/People/Materials/M_GEN_NitrileGloves.M_GEN_NitrileGloves")), 1.1f);
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
		const int32 Left = Face->GetBoneIndex(MidwifeLeftEye);
		const int32 Right = Face->GetBoneIndex(MidwifeRightEye);
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
	Task = GenesisMidwifeLogic::TaskFor(bBorn, bOnChest, OnChestSeconds, HandingSeconds, Progress, DryingSeconds, CoveringSeconds);
	TaskProgress = Progress;

	// Wo sie das Kind hält: gut 38 cm vor ihren Augen, etwas unter dem Kinn – das Kind liegt in ihren Händen
	// und schaut zu ihr hoch. Genau auf diesen Abstand sieht ein Neugeborenes am schärfsten.
	const FVector MyEye = GetEyeLocation();
	FVector Shoulders = MyEye - FVector(0.0, 0.0, 150.0);
	if (Body && Body->GetBoneIndex(TEXT("upperarm_l")) != INDEX_NONE && Body->GetBoneIndex(TEXT("upperarm_r")) != INDEX_NONE)
	{
		Shoulders = 0.5 * (Body->GetBoneLocation(TEXT("upperarm_l")) + Body->GetBoneLocation(TEXT("upperarm_r")));
	}
	const FVector HeldEye = Shoulders + SmoothedFacing * HoldForwardOfShouldersMm - FVector(0.0, 0.0, HoldBelowShouldersMm);
	HeldView = FTransform(FRotationMatrix::MakeFromXZ(MyEye - HeldEye, FVector::UpVector).ToQuat(), HeldEye);
	// Sie nimmt es in der ersten Sekunde auf und hebt es in zwei Sekunden zu sich hoch
	HoldBlend = Task == EGenesisMidwifeTask::Attending ? 0.0f : FMath::SmoothStep(0.8f, 3.0f, BornSeconds);

	GenesisMidwifeLogic::FInputs In;
	In.Task = Task;
	In.HandingProgress = Task == EGenesisMidwifeTask::Handing ? Progress : 1.0f;
	In.TaskProgress = Progress;
	In.Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
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
	// Niemand steht still wie eine Statue: Das Gewicht wandert langsam zwischen den Füßen (ein paar Millimeter
	// seitlich, ein Grad Drehung), unregelmäßig – Rauschen statt Sinus, damit sich nichts sichtbar wiederholt.
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const FVector Side = FVector::CrossProduct(FVector::UpVector, SmoothedFacing).GetSafeNormal();
	const float Shift = FMath::PerlinNoise1D(Now * 0.11f + 3.7f);
	const FVector Sway = Side * (14.0f * Shift) + SmoothedFacing * (5.0f * FMath::PerlinNoise1D(Now * 0.09f + 11.3f));
	const FRotator Turn(0.0f, 1.8f * FMath::PerlinNoise1D(Now * 0.07f + 5.1f), 0.0f);
	// MetaHuman: vorn ist +Y des Körpers
	// Sie steht auf 28 mm Clog-Sohle
	MidwifeActor->SetActorLocationAndRotation(SmoothedFeet + Sway + FVector(0.0, 0.0, SoleMm), (FRotationMatrix::MakeFromYZ(SmoothedFacing, FVector::UpVector).Rotator() + Turn));

	GenesisMotherLogic::Advance(State, Tuning, FGenesisMotherInputs(), DeltaSeconds);
	if (UGenesisMotherAnimInstance* Anim = GetAnim())
	{
		FGenesisMotherPosture Pose = Posture;
		Pose.SpineSettleDeg = Stance.LeanDegPerVertebra;
		Anim->Posture = Pose;
		const FTransform Component = Body->GetComponentTransform();
		FGenesisMotherPoseInputs& Inputs = Anim->PoseInputs;
		Inputs.LookTarget = Component.InverseTransformPosition(Stance.LookAt);
		// Hände ohne Kind: Beim Zusehen liegen die behandschuhten Hände locker ineinander vor dem Bauch – so stehen
		// Hebammen am Bett, die Hände sauber und bereit. Unter den Wehen hängen sie entspannt.
		const bool bClasped = Task == EGenesisMidwifeTask::Watching || Task == EGenesisMidwifeTask::Covering;
		const FVector FreeRight = bClasped ? FVector(-5.0, 20.0, 99.0) : MidwifeRestRightHand;
		const FVector FreeLeft = bClasped ? FVector(5.0, 21.0, 97.0) : MidwifeRestLeftHand;
		// Lebende Hände zittern nicht, aber sie stehen nie ganz still: wenige Millimeter, langsam, jede für sich
		const float Drift = Stance.HandsOnChild > 0.5f ? 0.35f : 0.9f;
		auto Noise = [Now](float Seed) { return FVector(FMath::PerlinNoise1D(Now * 0.31f + Seed), FMath::PerlinNoise1D(Now * 0.27f + Seed * 1.7f),
			FMath::PerlinNoise1D(Now * 0.23f + Seed * 2.3f)); };
		Inputs.RightHandTarget = FMath::Lerp(FreeRight, Component.InverseTransformPosition(Stance.RightHand), Stance.HandsOnChild) + Noise(1.3f) * Drift;
		Inputs.LeftHandTarget = FMath::Lerp(FreeLeft, Component.InverseTransformPosition(Stance.LeftHand), Stance.HandsOnChild) + Noise(7.9f) * Drift;
		if (CVarMidwifeDebug.GetValueOnGameThread() > 0 && Body)
		{
			const int32 Shoulder = Body->GetBoneIndex(TEXT("upperarm_r"));
			const FVector ShoulderCS = Shoulder != INDEX_NONE ? Body->GetBoneTransform(Shoulder, FTransform::Identity).GetLocation() : FVector::ZeroVector;
			UE_LOG(LogGenesis, Display, TEXT("Hebamme: Aufgabe %d, Hand rechts (cm) %s, Schulter %s, Abstand %.1f cm, Kind-Auge Welt %s, eigenes Auge %s"),
				(int32)Task, *Inputs.RightHandTarget.ToString(), *ShoulderCS.ToString(), FVector::Dist(Inputs.RightHandTarget, ShoulderCS),
				*In.ChildEye.ToString(), *GetEyeLocation().ToString());
		}
		// Finger: um das Kind geschmiegt, beim Rubbeln halb offen (das Tuch in der Hand), sonst entspannt
		Inputs.HandCurl = Task == EGenesisMidwifeTask::Holding || Task == EGenesisMidwifeTask::Handing ? 1.0f
			: (Task == EGenesisMidwifeTask::Drying ? 0.55f : (bClasped ? 0.45f : 0.15f));
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
	TEXT("genesis.Debug.ViewMidwife [Abstand mm] [Höhe mm] [Winkel °] [Zielhöhe mm] – Prüfkamera auf die Hebamme"),
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
		const float TargetHeight = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 850.0f;
		const FVector Target = Feet + FVector(0.0, 0.0, TargetHeight);
		ACameraActor* Camera = World->SpawnActor<ACameraActor>(Eye, (Target - Eye).Rotation());
		if (Camera)
		{
			Camera->GetCameraComponent()->SetFieldOfView(45.0f);
			Controller->SetViewTarget(Camera);
		}
	}));
