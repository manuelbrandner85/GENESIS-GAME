// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherAnimInstance.h"
#include "Animation/AnimNodeBase.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

/*
 * Achsen im Komponentenraum des MetaHuman-Körpers: +X = ihre linke Seite, +Y = vorn, +Z = oben.
 *
 * Eine Drehung um +X um den Winkel θ bringt (0,0,1) nach (0,−sin θ, cos θ). Daraus folgen alle
 * Vorzeichen unten:
 *   - ein nach unten zeigendes Glied (Arm, Oberschenkel) geht mit +θ nach vorn,
 *   - ein nach oben zeigendes (Hals, Kopf, Wirbelsäule) geht mit −θ nach vorn (Beugung).
 */
namespace
{
	const FName Pelvis(TEXT("pelvis"));
	const FName Spine[] = { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05") };
	const FName Neck01(TEXT("neck_01"));
	const FName Neck02(TEXT("neck_02"));
	const FName Head(TEXT("head"));

	FQuat AboutX(float Degrees) { return FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Degrees)); }
	FQuat AboutY(float Degrees) { return FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Degrees)); }

	FCompactPoseBoneIndex FindBone(const FBoneContainer& Bones, const FName& Name)
	{
		const int32 MeshIndex = Bones.GetPoseBoneIndexForBoneName(Name);
		return MeshIndex == INDEX_NONE ? FCompactPoseBoneIndex(INDEX_NONE) : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
	}

	/** Dreht einen Knochen im Komponentenraum um seinen eigenen Ursprung. Die Kinder gehen mit. */
	void RotateBone(FCSPose<FCompactPose>& Pose, const FCompactPoseBoneIndex& Index, const FQuat& Rotation)
	{
		if (!Index.IsValid())
		{
			return;
		}
		FTransform Transform = Pose.GetComponentSpaceTransform(Index);
		Transform.SetRotation((Rotation * Transform.GetRotation()).GetNormalized());
		TArray<FBoneTransform> Changed;
		Changed.Emplace(Index, Transform);
		Pose.SafeSetCSBoneTransforms(Changed);
	}

	/** Teilt eine Drehung: der Anteil Fraction derselben Achse. */
	FQuat Portion(const FVector& Axis, float AngleRadians, float Fraction)
	{
		return FQuat(Axis, AngleRadians * Fraction);
	}

	void SolveArm(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const TCHAR* Side, const FVector& Target,
		const FVector& Pole, const FRotator& HandOffset, float Blend)
	{
		const FCompactPoseBoneIndex Upper = FindBone(Bones, FName(FString::Printf(TEXT("upperarm_%s"), Side)));
		const FCompactPoseBoneIndex Lower = FindBone(Bones, FName(FString::Printf(TEXT("lowerarm_%s"), Side)));
		const FCompactPoseBoneIndex Hand = FindBone(Bones, FName(FString::Printf(TEXT("hand_%s"), Side)));
		if (!Upper.IsValid() || !Lower.IsValid() || !Hand.IsValid() || Blend <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FTransform UpperT = Pose.GetComponentSpaceTransform(Upper);
		FTransform LowerT = Pose.GetComponentSpaceTransform(Lower);
		FTransform HandT = Pose.GetComponentSpaceTransform(Hand);
		const FQuat OldLower = LowerT.GetRotation();
		const FQuat OldHand = HandT.GetRotation();

		const FVector Effector = FMath::Lerp(HandT.GetLocation(), Target, Blend);
		AnimationCore::SolveTwoBoneIK(UpperT, LowerT, HandT, UpperT.GetLocation() + Pole, Effector, false, 1.0, 1.0);

		// Die Hand folgt dem Unterarm und legt sich dann mit der Handfläche an das Kind
		const FQuat LowerDelta = LowerT.GetRotation() * OldLower.Inverse();
		const FQuat Offset = FQuat::Slerp(FQuat::Identity, HandOffset.Quaternion(), Blend);
		HandT.SetRotation((Offset * LowerDelta * OldHand).GetNormalized());

		TArray<FBoneTransform> Changed;
		Changed.Emplace(Upper, UpperT);
		Changed.Emplace(Lower, LowerT);
		Changed.Emplace(Hand, HandT);
		Pose.SafeSetCSBoneTransforms(Changed);
	}
}

void FGenesisMotherAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UGenesisMotherAnimInstance* Instance = Cast<UGenesisMotherAnimInstance>(InAnimInstance))
	{
		Posture = Instance->Posture;
		Inputs = Instance->PoseInputs;
	}
}

bool FGenesisMotherAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	const FBoneContainer& Bones = Output.Pose.GetBoneContainer();

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output.Pose);

	const FCompactPoseBoneIndex HeadIndex = FindBone(Bones, Head);
	const FQuat HeadRefRotation = HeadIndex.IsValid() ? Pose.GetComponentSpaceTransform(HeadIndex).GetRotation() : FQuat::Identity;

	// 1. Beine: Oberschenkel auf der Sitzfläche, Knie leicht angewinkelt
	for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
	{
		RotateBone(Pose, FindBone(Bones, FName(FString::Printf(TEXT("thigh_%s"), Side))), AboutX(Posture.HipFlexDeg));
		RotateBone(Pose, FindBone(Bones, FName(FString::Printf(TEXT("calf_%s"), Side))), AboutX(-Posture.KneeFlexDeg));
	}

	// 2. Rücken ins Kissen gerundet; der Atem streckt die obere Brustwirbelsäule ein wenig
	const float Breath = FMath::Clamp(Inputs.BreathLift, 0.0f, 1.0f);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Spine); ++Index)
	{
		const float Settle = -Posture.SpineSettleDeg;
		const float Lift = Index >= 2 ? Posture.BreathSpineDeg * Breath / 3.0f : 0.0f;
		RotateBone(Pose, FindBone(Bones, Spine[Index]), AboutX(Settle + Lift));
	}
	// Die Schlüsselbeine heben sich beim Einatmen (links: +X zeigt nach oben bei negativer Drehung um Y)
	RotateBone(Pose, FindBone(Bones, TEXT("clavicle_l")), AboutY(-Posture.BreathClavicleDeg * Breath));
	RotateBone(Pose, FindBone(Bones, TEXT("clavicle_r")), AboutY(Posture.BreathClavicleDeg * Breath));

	// 3. Blick: Der Kopf wendet sich dem Kind zu, den Rest übernehmen die Augen
	float EyeYaw = 0.0f;
	float EyePitch = 0.0f;
	if (HeadIndex.IsValid())
	{
		auto FaceFrame = [&](FVector& OutEye, FVector& OutForward, FVector& OutUp)
		{
			const FTransform HeadNow = Pose.GetComponentSpaceTransform(HeadIndex);
			const FQuat Delta = HeadNow.GetRotation() * HeadRefRotation.Inverse();
			OutEye = HeadNow.GetLocation() + Delta.RotateVector(Posture.EyeOffsetFromHead);
			OutForward = Delta.RotateVector(FVector::YAxisVector);
			OutUp = Delta.RotateVector(FVector::ZAxisVector);
		};

		FVector Eye, Forward, Up;
		FaceFrame(Eye, Forward, Up);
		const FVector Desired = (Inputs.LookTarget - Eye).GetSafeNormal();
		if (!Desired.IsNearlyZero())
		{
			FVector Axis;
			float Angle = 0.0f;
			FQuat::FindBetweenNormals(Forward, Desired).ToAxisAndAngle(Axis, Angle);
			Angle = FMath::Min(Angle, FMath::DegreesToRadians(Posture.MaxHeadTurnDeg)) * FMath::Clamp(Posture.HeadShare, 0.0f, 1.0f);
			// Der Hals trägt die Drehung zu gleichen Teilen mit dem Kopf – nicht der Kopf allein
			RotateBone(Pose, FindBone(Bones, Neck01), Portion(Axis, Angle, 0.3f));
			RotateBone(Pose, FindBone(Bones, Neck02), Portion(Axis, Angle, 0.3f));
			RotateBone(Pose, HeadIndex, Portion(Axis, Angle, 0.4f));

			FaceFrame(Eye, Forward, Up);
			RotateBone(Pose, HeadIndex, FQuat(Forward, FMath::DegreesToRadians(Posture.HeadTiltDeg)));

			FaceFrame(Eye, Forward, Up);
			const FVector Left = FVector::CrossProduct(Forward, Up).GetSafeNormal();
			const FVector Residual = (Inputs.LookTarget - Eye).GetSafeNormal();
			EyeYaw = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Residual, Left), FVector::DotProduct(Residual, Forward)));
			EyePitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FVector::DotProduct(Residual, Up), -1.0f, 1.0f)));
		}
	}

	// 4. Arme: rechte Hand am oberen Rücken des Kindes, linke unter dem Po
	const float ArmBlend = FMath::Clamp(Inputs.ArmBlend, 0.0f, 1.0f);
	SolveArm(Pose, Bones, TEXT("r"), Inputs.RightHandTarget, Posture.ElbowPoleRight, Posture.RightHandOffset, ArmBlend);
	SolveArm(Pose, Bones, TEXT("l"), Inputs.LeftHandTarget, Posture.ElbowPoleLeft, Posture.LeftHandOffset, ArmBlend);

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output.Pose);

	// 5. Gesicht als Steuerkurven: Das MetaHuman-Gesicht übernimmt sie vom Körper
	const float Range = FMath::Max(5.0f, Posture.EyeCurveRangeDeg);
	const float LookLeft = FMath::Clamp(EyeYaw / Range, 0.0f, 1.0f);
	const float LookRight = FMath::Clamp(-EyeYaw / Range, 0.0f, 1.0f);
	const float LookUp = FMath::Clamp(EyePitch / Range, 0.0f, 1.0f);
	const float LookDown = FMath::Clamp(-EyePitch / Range, 0.0f, 1.0f);
	const float Blink = FMath::Clamp(Inputs.BlinkClosure, 0.0f, 1.0f);
	const float Smile = FMath::Clamp(Inputs.Smile, 0.0f, 1.0f);
	for (const TCHAR* Side : { TEXT("L"), TEXT("R") })
	{
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeBlink%s"), Side)), Blink);
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeLookLeft%s"), Side)), LookLeft);
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeLookRight%s"), Side)), LookRight);
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeLookUp%s"), Side)), LookUp);
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeLookDown%s"), Side)), LookDown);
		// Ein warmes Lächeln hebt Mundwinkel und Wangen – die Wangen machen es echt
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_mouthCornerPull%s"), Side)), Smile);
		Output.Curve.Set(FName(FString::Printf(TEXT("CTRL_expressions_eyeCheekRaise%s"), Side)), Smile * 0.6f);
	}
	return true;
}

FAnimInstanceProxy* UGenesisMotherAnimInstance::CreateAnimInstanceProxy()
{
	return new FGenesisMotherAnimInstanceProxy(this);
}

void UGenesisMotherAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
	delete InProxy;
}
