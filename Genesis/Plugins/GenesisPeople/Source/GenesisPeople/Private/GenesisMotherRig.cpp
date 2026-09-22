// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherRig.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GenesisMotherAnimInstance.h"
#include "GenesisMotherLogic.h"

namespace
{
	const FName HeadBone(TEXT("head"));
	const FName ChestBone(TEXT("spine_05"));
	const FName LeftEyeBone(TEXT("FACIAL_L_Eye"));
	const FName RightEyeBone(TEXT("FACIAL_R_Eye"));

	/** Hände in Ruhe (Komponentenraum, cm): auf dem Bauch, über der Decke. */
	const FVector RestRightHand(-12.0, 17.0, 93.0);
	const FVector RestLeftHand(12.0, 17.0, 93.0);

	/**
	 * Wo die Hände am Kind liegen, relativ zu seinen Augen (mm).
	 * Auf der Brust liegt das Kind bäuchlings, der Rücken zeigt von ihr weg: die rechte Hand flach auf dem
	 * oberen Rücken, die linke weiter unten am Po. Vor dem Gesicht hält sie es aufrecht: rechts hinter
	 * Kopf und Nacken, links unter dem Po.
	 */
	constexpr float ChestRightDown = 100.0f;
	constexpr float ChestRightOut = 55.0f;
	constexpr float ChestLeftDown = 330.0f;
	constexpr float ChestLeftOut = 40.0f;
	constexpr float ChestLeftInward = 20.0f;
	constexpr float FaceRightBehind = 75.0f;
	constexpr float FaceRightDown = 50.0f;
	constexpr float FaceLeftBehind = 35.0f;
	constexpr float FaceLeftDown = 270.0f;

	/**
	 * Blickkontakt: Das Kind sieht ihr in die Augen, wenn ihre Augen weniger als 10° neben seiner
	 * Blickmitte liegen – und er endet erst jenseits von 15°. Ohne diesen Abstand flackerte er mit
	 * Herzschlag und Atem mehrmals je Sekunde an und aus (in der gebauten Fassung gemessen).
	 */
	constexpr float EyeContactConeDeg = 10.0f;
	constexpr float EyeContactReleaseDeg = 15.0f;
	constexpr float EyeContactMaxDistanceMm = 450.0f;
}

AGenesisMotherRig::AGenesisMotherRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Blick des Kindes auf der Brust (wie GenesisBirthCameraRig: seitlich in den Raum, Scheitel zum Kinn der Mutter)
	PreviewChildView = FRotationMatrix::MakeFromXZ(FVector(0.30, 0.95, -0.08), FVector(-0.35, 0.0, 0.94)).Rotator();
	State = GenesisMotherLogic::Begin(0x51A7E5u);
}

void AGenesisMotherRig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Configure();
}

void AGenesisMotherRig::BeginPlay()
{
	Super::BeginPlay();
	State = GenesisMotherLogic::Begin(static_cast<uint32>(GetUniqueID()) * 2654435761u + 1u);
	Configure();
}

void AGenesisMotherRig::Configure()
{
	Body = nullptr;
	Face = nullptr;
	if (!MotherActor)
	{
		return;
	}

	TArray<USkeletalMeshComponent*> Meshes;
	MotherActor->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetName().StartsWith(TEXT("Body")))
		{
			Body = Mesh;
		}
	}
	if (!Body)
	{
		return;
	}

	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		const FString Name = Mesh->GetName();
		if (Name.StartsWith(TEXT("Face")))
		{
			Face = Mesh;
			// Das Gesicht übernimmt die Pose vom Körper – auch in der Editor-Vorschau, sonst stünde der
			// Kopf in der Grundhaltung neben einem Hals, der längst zum Kind geneigt ist
			Mesh->SetUpdateAnimationInEditor(true);
			Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		}
		else if (Mesh != Body)
		{
			Mesh->SetVisibility(!bHideOutfit, true);
			Mesh->SetHiddenInGame(bHideOutfit, true);
		}
	}

	if (Body->GetAnimClass() != UGenesisMotherAnimInstance::StaticClass())
	{
		Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Body->SetAnimInstanceClass(UGenesisMotherAnimInstance::StaticClass());
	}
	Body->SetUpdateAnimationInEditor(true);
	// Die Kamera sieht oft nur Gesicht und Hände – der Körper muss trotzdem jedes Bild posiert werden
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->bEnableUpdateRateOptimizations = false;

	for (const TCHAR* Bone : { TEXT("thigh_l"), TEXT("thigh_r") })
	{
		if (bHideLegs)
		{
			Body->HideBoneByName(Bone, PBO_None);
		}
		else
		{
			Body->UnHideBoneByName(Bone);
		}
	}

	if (const USkinnedAsset* Asset = Body->GetSkinnedAsset())
	{
		const FReferenceSkeleton& Reference = Asset->GetRefSkeleton();
		const int32 HeadIndex = Reference.FindBoneIndex(HeadBone);
		if (HeadIndex != INDEX_NONE)
		{
			HeadRefRotation = FAnimationRuntime::GetComponentSpaceTransformRefPose(Reference, HeadIndex).GetRotation();
		}
		const int32 ChestIndex = Reference.FindBoneIndex(ChestBone);
		if (ChestIndex != INDEX_NONE)
		{
			ChestRefTransform = FAnimationRuntime::GetComponentSpaceTransformRefPose(Reference, ChestIndex);
		}
	}

	// Wo ihre Augen relativ zum Kopfknochen sitzen, steht im Gesichtsskelett – nicht schätzen, nachsehen
	if (const USkinnedAsset* FaceAsset = Face ? Face->GetSkinnedAsset() : nullptr)
	{
		const FReferenceSkeleton& Reference = FaceAsset->GetRefSkeleton();
		const int32 HeadIndex = Reference.FindBoneIndex(HeadBone);
		const int32 LeftEye = Reference.FindBoneIndex(LeftEyeBone);
		const int32 RightEye = Reference.FindBoneIndex(RightEyeBone);
		if (HeadIndex != INDEX_NONE && LeftEye != INDEX_NONE && RightEye != INDEX_NONE)
		{
			const FVector EyeMid = 0.5 * (FAnimationRuntime::GetComponentSpaceTransformRefPose(Reference, LeftEye).GetLocation()
				+ FAnimationRuntime::GetComponentSpaceTransformRefPose(Reference, RightEye).GetLocation());
			Posture.EyeOffsetFromHead = EyeMid - FAnimationRuntime::GetComponentSpaceTransformRefPose(Reference, HeadIndex).GetLocation();
		}
	}
}

UGenesisMotherAnimInstance* AGenesisMotherRig::GetMotherAnim() const
{
	return Body ? Cast<UGenesisMotherAnimInstance>(Body->GetAnimInstance()) : nullptr;
}

void AGenesisMotherRig::SetChild(bool bOnChest, bool bSeeksFace, const FVector& EyeLocation, const FQuat& ViewRotation)
{
	bHasChild = true;
	Inputs.bChildOnChest = bOnChest;
	Inputs.bChildSeeksFace = bSeeksFace;
	ChildEye = EyeLocation;
	ChildView = ViewRotation;
}

float AGenesisMotherRig::GetEnFaceBlend() const
{
	return GenesisMotherLogic::GetEnFaceBlend(State);
}

float AGenesisMotherRig::GetBreathLift() const
{
	return GenesisMotherLogic::GetBreathLift(State, Tuning);
}

FVector AGenesisMotherRig::GetChestChildLocation() const
{
	if (!Body)
	{
		return PreviewChildEye;
	}
	// Am obersten Brustwirbel festgemacht: Rundet sich ihr Rücken ins Kissen, geht die Brust mit –
	// über die ganze Wirbelsäule sind das mehrere Zentimeter
	const int32 ChestIndex = Body->GetBoneIndex(ChestBone);
	if (ChestIndex == INDEX_NONE)
	{
		return Body->GetComponentTransform().TransformPosition(ChildChestEyeCm);
	}
	const FVector InChestFrame = ChestRefTransform.InverseTransformPosition(ChildChestEyeCm);
	return Body->GetBoneTransform(ChestIndex).TransformPosition(InChestFrame);
}

FVector AGenesisMotherRig::GetEyeLocation() const
{
	if (!Body)
	{
		return GetActorLocation();
	}
	// Am genauesten: die Augäpfel selbst (Knochen des MetaHuman-Gesichts)
	if (Face)
	{
		const int32 LeftEye = Face->GetBoneIndex(LeftEyeBone);
		const int32 RightEye = Face->GetBoneIndex(RightEyeBone);
		if (LeftEye != INDEX_NONE && RightEye != INDEX_NONE)
		{
			return 0.5 * (Face->GetBoneTransform(LeftEye).GetLocation() + Face->GetBoneTransform(RightEye).GetLocation());
		}
	}
	const int32 HeadIndex = Body->GetBoneIndex(HeadBone);
	if (HeadIndex == INDEX_NONE)
	{
		return Body->GetComponentLocation();
	}
	const FTransform Component = Body->GetComponentTransform();
	const FTransform HeadWorld = Body->GetBoneTransform(HeadIndex);
	const FQuat HeadComponent = Component.GetRotation().Inverse() * HeadWorld.GetRotation();
	const FVector HeadLocation = Component.InverseTransformPosition(HeadWorld.GetLocation());
	const FQuat Delta = HeadComponent * HeadRefRotation.Inverse();
	return Component.TransformPosition(HeadLocation + Delta.RotateVector(Posture.EyeOffsetFromHead));
}

void AGenesisMotherRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Der Blueprint der Figur baut seine Komponenten bei jeder Änderung im Editor neu – dann neu einrichten
	if (!Body || !GetMotherAnim())
	{
		Configure();
	}
	UpdatePose(DeltaSeconds);
}

void AGenesisMotherRig::UpdatePose(float DeltaSeconds)
{
	if (!Body)
	{
		return;
	}

	const bool bGame = GetWorld() && GetWorld()->IsGameWorld();
	if (!bGame)
	{
		Inputs.bChildOnChest = true;
		Inputs.bChildSeeksFace = bPreviewEnFace;
		ChildEye = bPreviewEnFace && bEnFaceInitialized ? EnFaceChild.GetLocation() : GetChestChildLocation();
		ChildView = bPreviewEnFace && bEnFaceInitialized ? EnFaceChild.GetRotation() : PreviewChildView.Quaternion();
		bHasChild = true;
	}

	const FTransform Component = Body->GetComponentTransform();
	const FVector Up = Component.TransformVectorNoScale(FVector::ZAxisVector);
	const FVector Forward = Component.TransformVectorNoScale(FVector::YAxisVector);
	const FVector Left = Component.TransformVectorNoScale(FVector::XAxisVector);

	// Wo das Kind vor ihrem Gesicht sein wird: auf Sehschärfe-Abstand vor ihren Augen. Sie liegt 45°
	// zurück – „vor dem Gesicht“ heißt deshalb ihrer Blickrichtung nach halb zur Brust hin, in der Welt
	// etwa waagerecht vor ihr. Ihre Blickachse nach vorn zeigte schräg zur Decke; dorthin hebt niemand ein Kind.
	const FVector Eye = GetEyeLocation();
	const FVector Toward = (Forward * 0.72 - Up * 0.69).GetSafeNormal();
	const FVector FaceTarget = Eye + Toward * Tuning.EnFaceDistanceMm;
	// Das Kind wird aufrecht gehalten: sein „oben" ist das Oben der Welt, nicht ihres
	const FQuat FaceRotation = FRotationMatrix::MakeFromXZ(-Toward, FVector::UpVector).ToQuat();
	if (!bEnFaceInitialized)
	{
		EnFaceChild = FTransform(FaceRotation, FaceTarget);
		bEnFaceInitialized = true;
	}
	else
	{
		// Geglättet: Ihr Kopf bewegt sich mit dem Blick, das Ziel soll dabei nicht zittern
		EnFaceChild.SetLocation(FMath::VInterpTo(EnFaceChild.GetLocation(), FaceTarget, DeltaSeconds, 2.0f));
		EnFaceChild.SetRotation(FQuat::Slerp(EnFaceChild.GetRotation(), FaceRotation, FMath::Clamp(DeltaSeconds * 2.0f, 0.0f, 1.0f)));
	}

	// Blickkontakt: Ihre Augen liegen in der Blickmitte des Kindes, auf Armlänge
	const FVector ToEyes = Eye - ChildEye;
	const float Distance = ToEyes.Size();
	const float Cosine = FVector::DotProduct(ChildView.GetForwardVector(), ToEyes.GetSafeNormal());
	const float Cone = Inputs.bChildLooksAtEyes ? EyeContactReleaseDeg : EyeContactConeDeg;
	Inputs.bChildLooksAtEyes = bHasChild && Distance < EyeContactMaxDistanceMm && Cosine > FMath::Cos(FMath::DegreesToRadians(Cone));
	bEyeContact = Inputs.bChildLooksAtEyes;

	GenesisMotherLogic::Advance(State, Tuning, Inputs, DeltaSeconds);
	bEyeContact = bEyeContact && GenesisMotherLogic::IsFaceToFace(State);

	// Hände: ruhen, bis das Kind auf ihr liegt, und greifen zu, wenn es ankommt (die Hebamme hebt es in 7 s herüber)
	OnChestSeconds = Inputs.bChildOnChest ? OnChestSeconds + DeltaSeconds : 0.0f;
	HoldBlend = bGame ? FMath::SmoothStep(4.0f, 7.0f, OnChestSeconds) : 1.0f;

	const float EnFace = GetEnFaceBlend();
	const FVector ViewForward = ChildView.GetForwardVector();
	const FVector ViewUp = ChildView.GetUpVector();
	const FVector ChestRight = ChildEye - Up * ChestRightDown + Forward * ChestRightOut;
	const FVector ChestLeft = ChildEye - Up * ChestLeftDown + Forward * ChestLeftOut + Left * ChestLeftInward;
	const FVector FaceRight = ChildEye - ViewForward * FaceRightBehind - ViewUp * FaceRightDown;
	const FVector FaceLeft = ChildEye - ViewForward * FaceLeftBehind - ViewUp * FaceLeftDown;
	const FVector HoldRight = Component.InverseTransformPosition(FMath::Lerp(ChestRight, FaceRight, EnFace));
	const FVector HoldLeft = Component.InverseTransformPosition(FMath::Lerp(ChestLeft, FaceLeft, EnFace));

	if (UGenesisMotherAnimInstance* Anim = GetMotherAnim())
	{
		Anim->Posture = Posture;
		FGenesisMotherPoseInputs& Pose = Anim->PoseInputs;
		Pose.LookTarget = bHasChild ? Component.InverseTransformPosition(ChildEye) : FVector(0.0, 60.0, 100.0);
		Pose.RightHandTarget = FMath::Lerp(RestRightHand, HoldRight, HoldBlend);
		Pose.LeftHandTarget = FMath::Lerp(RestLeftHand, HoldLeft, HoldBlend);
		Pose.ArmBlend = 1.0f;
		Pose.BreathLift = GenesisMotherLogic::GetBreathLift(State, Tuning);
		Pose.BlinkClosure = GenesisMotherLogic::GetBlinkClosure(State, Tuning);
		Pose.Smile = State.Smile;
		// Hält sie das Kind, umfassen ihre Finger seinen Rücken; sonst liegen sie entspannt
		Pose.HandCurl = 0.8f * HoldBlend;
		LipSync.Evaluate(GetWorld()->GetAudioTimeSeconds(), Pose.SpeechCurves);
	}
}

void AGenesisMotherRig::Speak(const UGenesisLipSyncTrack* Track)
{
	if (const UWorld* World = GetWorld())
	{
		LipSync.Start(Track, World->GetAudioTimeSeconds());
	}
}

bool AGenesisMotherRig::IsSpeaking() const
{
	return GetWorld() && LipSync.IsSpeaking(GetWorld()->GetAudioTimeSeconds());
}
