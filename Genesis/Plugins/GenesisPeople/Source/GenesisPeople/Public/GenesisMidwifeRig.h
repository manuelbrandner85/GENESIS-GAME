// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisMotherAnimInstance.h"
#include "GenesisMotherTypes.h"
#include "GenesisLipSync.h"
#include "GenesisMidwifeRig.generated.h"

class USkeletalMeshComponent;
class UMaterialInterface;
class AGenesisMotherRig;

/** Was die Hebamme gerade tut – aus dem Verlauf der Geburt, nicht aus einem Drehbuch. */
UENUM(BlueprintType)
enum class EGenesisMidwifeTask : uint8
{
	/** Wehen: Sie steht am Fußende des Bettes. */
	Attending,
	/** Das Kind ist da: Sie hat es aufgefangen und hält es vor sich – das Erste, was es sieht. */
	Holding,
	/** Sie legt es der Mutter auf die Brust und geht dabei an die Seite des Bettes. */
	Handing,
	/** Es liegt auf der Mutter: Sie steht neben dem Bett und sieht zu. */
	Watching
};

/** Standort, Blick und Hände der Hebamme in Weltkoordinaten (Szene: 1 mm = 1 Einheit). */
struct GENESISPEOPLE_API FGenesisMidwifeStance
{
	/** Wo ihre Füße stehen (Boden). */
	FVector Feet = FVector::ZeroVector;
	/** Wohin ihr Körper zeigt (waagerecht, Einheitsvektor). */
	FVector Facing = FVector::ForwardVector;
	/** Wohin sie schaut. */
	FVector LookAt = FVector::ZeroVector;
	/** Wo ihre Hände sind; ohne Kind hängen sie locker (HandsOnChild = 0). */
	FVector RightHand = FVector::ZeroVector;
	FVector LeftHand = FVector::ZeroVector;
	float HandsOnChild = 0.0f;
	/** Vorbeugen über der Wirbelsäule (Grad je Wirbel). */
	float LeanDegPerVertebra = 0.0f;
};

namespace GenesisMidwifeLogic
{
	struct FInputs
	{
		EGenesisMidwifeTask Task = EGenesisMidwifeTask::Attending;
		/** 0..1 Fortschritt des Hinüberreichens. */
		float HandingProgress = 0.0f;
		/** Augen und Blick des Kindes (Welt). */
		FVector ChildEye = FVector::ZeroVector;
		FVector ChildForward = FVector::ForwardVector;
		/** Am Fußende des Bettes (Boden) und daneben, auf Höhe der Brust der Mutter (Boden). */
		FVector FootOfBed = FVector::ZeroVector;
		FVector Bedside = FVector::ZeroVector;
		FVector MotherEye = FVector::ZeroVector;
		float FloorZ = 0.0f;
	};

	/**
	 * Wo sie steht, wohin sie schaut, wo ihre Hände sind.
	 * Hält sie das Kind, steht sie so, dass ihr Gesicht gut 40 cm vor seinen Augen ist – genau der Abstand,
	 * auf den ein Neugeborenes scharf sieht. Sie beugt sich dazu vor, die Hände unter Kopf und Rücken.
	 */
	GENESISPEOPLE_API FGenesisMidwifeStance Compute(const FInputs& In, float FaceDistanceMm = 420.0f, float EyeHeightMm = 1540.0f);

	/** Die Aufgabe aus dem Zustand: noch nicht geboren, geboren, auf der Haut seit wie vielen Sekunden. */
	GENESISPEOPLE_API EGenesisMidwifeTask TaskFor(bool bBorn, bool bOnChest, float SecondsOnChest, float HandingSeconds, float& OutProgress);
}

/**
 * Die Hebamme. Macht aus einer MetaHuman-Figur (BP_Midwife) die Frau, die das Kind auffängt, es
 * hochhebt und der Mutter auf die Brust legt. Kasack und Hose in Blaugrün, wie im deutschen Kreißsaal.
 */
UCLASS()
class GENESISPEOPLE_API AGenesisMidwifeRig : public AActor
{
	GENERATED_BODY()

public:
	AGenesisMidwifeRig();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Die MetaHuman-Figur (BP aus dem Creator), die hier zur Hebamme wird. */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	TObjectPtr<AActor> MidwifeActor;

	/** Die Mutter – für den Platz neben dem Bett und den Blick zu ihr. */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	TObjectPtr<AGenesisMotherRig> Mother;

	/** Farbe des Kasacks (linear): Blaugrün, wie Kasacks im deutschen Kreißsaal. */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	FLinearColor ScrubsColor = FLinearColor(0.030f, 0.165f, 0.185f);

	/** Am Fußende des Bettes, wo sie unter der Geburt steht (Boden, Welt). Kreißsaal: Bett endet bei x = 512, Boden bei z = −800. */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	FVector FootOfBed = FVector(1060.0, 0.0, -800.0);

	/** Seitlich neben dem Bett, wo sie nach dem Auflegen steht, relativ zu den Augen der Mutter (mm). */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	float BedsideOffsetMm = 650.0f;

	/** Dauer des Hinüberreichens (s) – die Kamera des Kindes braucht dafür 7 s. */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	float HandingSeconds = 7.0f;

	UPROPERTY(EditAnywhere, Category = "Midwife")
	FGenesisMotherPosture Posture;

	/** Von der Regie in jedem Bild gesetzt. */
	void SetChild(bool bBorn, bool bOnChest, const FVector& EyeLocation, const FQuat& ViewRotation);

	/** Sie spricht: Die Lippensynchron-Spur der Zeile läuft ab jetzt mit der Audio-Uhr der Welt – zeitgleich mit ihrer Stimme. */
	void Speak(const class UGenesisLipSyncTrack* Track);

	/** Spricht sie gerade? (Für Prüfung und Anzeige.) */
	bool IsSpeaking() const;

	/** Mitte zwischen ihren Augen (Welt) – für die Stimme und den Blick des Kindes. */
	FVector GetEyeLocation() const;

	EGenesisMidwifeTask GetTask() const { return Task; }

	/** Wo und wie das Kind in ihren Händen liegt (Welt): Augen und Blick zu ihrem Gesicht. */
	FTransform GetHeldView() const { return HeldView; }

	/** 0 = das Kind liegt noch dort, wo es herauskam; 1 = sie hat es zu sich hochgehoben. */
	float GetHoldBlend() const { return HoldBlend; }

	/** Abstand des gehaltenen Kindes vor ihren Augen und unter ihnen (mm). */
	UPROPERTY(EditAnywhere, Category = "Midwife")
	float HoldDistanceMm = 380.0f;

	UPROPERTY(EditAnywhere, Category = "Midwife")
	float HoldBelowEyesMm = 170.0f;

private:
	void Configure();

	UPROPERTY(Transient) FGenesisLipSyncPlayer LipSync;
	UGenesisMotherAnimInstance* GetAnim() const;

	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Face;

	/**
	 * Kasackhose und Nitrilhandschuhe: zwei Hüllen auf dem Körper (gleiches Mesh, folgen seiner Pose). Das Material
	 * wählt Beine bzw. Hände aus und legt den Stoff darüber (Tools/Unreal/Birth/midwife_scrubs.py).
	 */
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Trousers;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Gloves;
	void Dress();

	FGenesisMotherState State;
	FGenesisMotherTuning Tuning;
	EGenesisMidwifeTask Task = EGenesisMidwifeTask::Attending;
	FVector ChildEye = FVector::ZeroVector;
	FQuat ChildView = FQuat::Identity;
	bool bBorn = false;
	bool bOnChest = false;
	float OnChestSeconds = 0.0f;
	float BornSeconds = 0.0f;
	FTransform HeldView = FTransform::Identity;
	float HoldBlend = 0.0f;
	FVector SmoothedFeet = FVector::ZeroVector;
	FVector SmoothedFacing = FVector::ForwardVector;
	bool bPlaced = false;
};
