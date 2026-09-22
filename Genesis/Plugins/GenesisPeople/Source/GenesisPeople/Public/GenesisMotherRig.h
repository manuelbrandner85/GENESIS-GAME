// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisMotherAnimInstance.h"
#include "GenesisMotherTypes.h"
#include "GenesisLipSync.h"
#include "GenesisMotherRig.generated.h"

class USkeletalMeshComponent;
class UGenesisMotherAnimInstance;

/**
 * Macht aus einer MetaHuman-Figur im Level die Mutter.
 *
 * Die Figur selbst (Blueprint aus dem MetaHuman Creator) bleibt unverändert; dieser Actor setzt beim
 * Start die eigene Animationsinstanz auf ihren Körper, blendet die Standardkleidung und die Beine
 * (die unter der Decke liegen) aus und rechnet in jedem Bild, wohin sie schaut und wo ihre Hände sind.
 *
 * Maßstab der Szene: 1 mm = 1 Einheit. Die Figur ist in cm gebaut und steht deshalb mit Faktor 10 im Level.
 */
UCLASS()
class GENESISPEOPLE_API AGenesisMotherRig : public AActor
{
	GENERATED_BODY()

public:
	AGenesisMotherRig();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return bPreviewInEditor; }

	/** Die MetaHuman-Figur (BP aus dem Creator), die hier zur Mutter wird. */
	UPROPERTY(EditAnywhere, Category = "Mother")
	TObjectPtr<AActor> MotherActor;

	UPROPERTY(EditAnywhere, Category = "Mother")
	FGenesisMotherTuning Tuning;

	UPROPERTY(EditAnywhere, Category = "Mother")
	FGenesisMotherPosture Posture;

	/** Die Beine liegen unter der Decke (eigenes Tuch aus der Stoffsimulation) und werden ausgeblendet. */
	UPROPERTY(EditAnywhere, Category = "Mother")
	bool bHideLegs = true;

	/** Die Standardkleidung des Creators (T-Shirt, Shorts) passt nicht in einen Kreißsaal. */
	UPROPERTY(EditAnywhere, Category = "Mother")
	bool bHideOutfit = true;

	/** Im Editor mitlaufen lassen, damit Haltung und Hände ohne Spielstart geprüft werden können. */
	UPROPERTY(EditAnywhere, Category = "Preview")
	bool bPreviewInEditor = true;

	/** Vorschau im Editor: Das Kind liegt auf der Brust, die Augen hier (Weltkoordinaten, mm). */
	UPROPERTY(EditAnywhere, Category = "Preview")
	FVector PreviewChildEye = FVector(-538.0, 60.0, 438.0);

	UPROPERTY(EditAnywhere, Category = "Preview")
	FRotator PreviewChildView = FRotator::ZeroRotator;

	/** Vorschau: Kind vor dem Gesicht statt auf der Brust. */
	UPROPERTY(EditAnywhere, Category = "Preview")
	bool bPreviewEnFace = false;

	/** Von der Regie in jedem Bild gesetzt. */
	void SetChild(bool bOnChest, bool bSeeksFace, const FVector& EyeLocation, const FQuat& ViewRotation);

	/** Sie spricht: Die Lippensynchron-Spur der Zeile läuft ab jetzt mit der Audio-Uhr der Welt – zeitgleich mit ihrer Stimme. */
	void Speak(const class UGenesisLipSyncTrack* Track);

	/** Spricht sie gerade? (Für Prüfung und Anzeige.) */
	bool IsSpeaking() const;

	/**
	 * Wo die Augen des Kindes liegen, wenn es bäuchlings auf ihrer Brust liegt – im Komponentenraum
	 * ihres Körpers (cm, MetaHuman-Referenzpose): etwas rechts der Mitte, oberhalb der Brust, die Wange
	 * auf dem Brustbein, das Auge gut 6 cm über der Haut. Von hier aus fällt der Blick an ihrem Arm
	 * entlang in den Raum, und nach oben über Hals und Kinn in ihr Gesicht.
	 */
	UPROPERTY(EditAnywhere, Category = "Mother")
	FVector ChildChestEyeCm = FVector(-3.0, 18.5, 127.0);

	/** Zustand und Ausgaben für Kamera und Regie. */
	const FGenesisMotherState& GetState() const { return State; }

	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	float GetEnFaceBlend() const;

	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	float GetBreathLift() const;

	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	bool HasEyeContact() const { return bEyeContact; }

	/** Wo die Augen des Kindes in der En-face-Haltung sind und wie es dabei schaut (Welt). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	FTransform GetEnFaceChildTransform() const { return EnFaceChild; }

	/** Augen des Kindes auf ihrer Brust (Welt), ohne Atem – den legt die Kamera selbst darauf. */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	FVector GetChestChildLocation() const;

	/** Mitte zwischen ihren Augen (Welt). */
	UFUNCTION(BlueprintCallable, Category = "Genesis|Mother")
	FVector GetEyeLocation() const;

private:
	UPROPERTY(Transient) FGenesisLipSyncPlayer LipSync;
	void Configure();
	void UpdatePose(float DeltaSeconds);
	UGenesisMotherAnimInstance* GetMotherAnim() const;

	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Face;

	FGenesisMotherState State;
	FGenesisMotherInputs Inputs;
	FVector ChildEye = FVector::ZeroVector;
	FQuat ChildView = FQuat::Identity;
	bool bHasChild = false;
	bool bEyeContact = false;
	/** 0 = Hände ruhen, 1 = Hände am Kind. */
	float HoldBlend = 0.0f;
	float OnChestSeconds = 0.0f;
	FTransform EnFaceChild = FTransform::Identity;
	FQuat HeadRefRotation = FQuat::Identity;
	FTransform ChestRefTransform = FTransform::Identity;
	bool bEnFaceInitialized = false;
};
