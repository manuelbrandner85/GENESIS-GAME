// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "GenesisMotherAnimInstance.generated.h"

/**
 * Haltung der Mutter im Entbindungsbett. Winkel in Grad, Längen in cm (MetaHuman-Maßstab).
 *
 * Das Rückenteil selbst ist nicht Teil der Pose: Der ganze Körper liegt um 45° zurück, weil der
 * Actor so im Bett steht. Hier steht nur, was sie darin tut.
 */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherPosture
{
	GENERATED_BODY()

	/** Die Hüfte ist gebeugt: Oberkörper am Rückenteil, Oberschenkel auf der Sitzfläche. */
	UPROPERTY(EditAnywhere, Category = "Posture") float HipFlexDeg = 45.0f;

	/** Knie leicht angewinkelt – so liegt man nach einer Geburt, nicht gestreckt. */
	UPROPERTY(EditAnywhere, Category = "Posture") float KneeFlexDeg = 35.0f;

	/** Der Rücken rundet sich ins Kissen: Beugung je Wirbelsäulenglied. */
	UPROPERTY(EditAnywhere, Category = "Posture") float SpineSettleDeg = 3.0f;

	/** Wie viel der Kopfdrehung zum Kind der Hals übernimmt – den Rest machen die Augen. */
	UPROPERTY(EditAnywhere, Category = "Gaze", meta = (ClampMin = "0", ClampMax = "1")) float HeadShare = 0.75f;

	/** Grenzen der Kopfdrehung (Grad). */
	UPROPERTY(EditAnywhere, Category = "Gaze") float MaxHeadTurnDeg = 70.0f;

	/** Kopfneigung zur Seite beim Ansehen – eine der unwillkürlichsten Gesten gegenüber einem Säugling. */
	UPROPERTY(EditAnywhere, Category = "Gaze") float HeadTiltDeg = 7.0f;

	/** Augenmitte relativ zum Kopfknochen in der Referenzpose (cm). */
	UPROPERTY(EditAnywhere, Category = "Gaze") FVector EyeOffsetFromHead = FVector(0.0, 9.0, 11.5);

	/** Wie weit die Augen per Steuerkurve höchstens drehen (Grad) – für den Wert 1 der Kurve. */
	UPROPERTY(EditAnywhere, Category = "Gaze") float EyeCurveRangeDeg = 30.0f;

	/** Einatmen: Streckung der oberen Brustwirbelsäule und Heben der Schlüsselbeine (Grad). */
	UPROPERTY(EditAnywhere, Category = "Breath") float BreathSpineDeg = 1.2f;
	UPROPERTY(EditAnywhere, Category = "Breath") float BreathClavicleDeg = 1.5f;

	/** Ellenbogen weisen nach außen-hinten-unten (Richtung relativ zur Schulter, cm). */
	UPROPERTY(EditAnywhere, Category = "Arms") FVector ElbowPoleRight = FVector(-30.0, -12.0, -25.0);
	UPROPERTY(EditAnywhere, Category = "Arms") FVector ElbowPoleLeft = FVector(30.0, -12.0, -25.0);

	/** Zusätzliche Drehung der Hände (Komponentenraum), damit die Handflächen am Kind liegen. */
	UPROPERTY(EditAnywhere, Category = "Arms") FRotator RightHandOffset = FRotator(0.0, 0.0, 0.0);
	UPROPERTY(EditAnywhere, Category = "Arms") FRotator LeftHandOffset = FRotator(0.0, 0.0, 0.0);
};

/** Was die Pose in diesem Bild braucht – alles im Komponentenraum des Körpers (cm). */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherPoseInputs
{
	GENERATED_BODY()

	/** Wohin sie schaut. */
	UPROPERTY(EditAnywhere, Category = "Pose") FVector LookTarget = FVector(0.0, 40.0, 120.0);

	/** Hände: rechts am oberen Rücken des Kindes, links unter dem Po. */
	UPROPERTY(EditAnywhere, Category = "Pose") FVector RightHandTarget = FVector(-8.0, 22.0, 105.0);
	UPROPERTY(EditAnywhere, Category = "Pose") FVector LeftHandTarget = FVector(4.0, 20.0, 90.0);

	/** 0 = Arme folgen der Referenzpose, 1 = Hände am Ziel. */
	UPROPERTY(EditAnywhere, Category = "Pose") float ArmBlend = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Pose") float BreathLift = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Pose") float BlinkClosure = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Pose") float Smile = 0.1f;
};

class UGenesisMotherAnimInstance;

/** Rechnet die Pose auf dem Animations-Thread. Ohne Animationsgraph: Referenzpose, dann Haltung, Atem, Blick, Arme. */
struct GENESISPEOPLE_API FGenesisMotherAnimInstanceProxy : public FAnimInstanceProxy
{
	FGenesisMotherAnimInstanceProxy() = default;
	explicit FGenesisMotherAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	FGenesisMotherPosture Posture;
	FGenesisMotherPoseInputs Inputs;
};

/**
 * Animationsinstanz für den Körper der Mutter (MetaHuman-Körperskelett).
 *
 * Das Gesicht hängt am Körper: Die Gesichts-Animation des MetaHuman übernimmt Pose und Kurven vom
 * Körper, deshalb schreibt diese Instanz auch Lidschlag, Blick und Lächeln als Steuerkurven.
 */
UCLASS(Transient, NotBlueprintable)
class GENESISPEOPLE_API UGenesisMotherAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Genesis|Mother") FGenesisMotherPosture Posture;
	UPROPERTY(EditAnywhere, Category = "Genesis|Mother") FGenesisMotherPoseInputs PoseInputs;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
};
