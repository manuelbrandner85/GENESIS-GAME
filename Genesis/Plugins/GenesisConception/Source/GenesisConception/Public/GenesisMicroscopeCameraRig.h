// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisMicroscopeCameraRig.generated.h"

class AGenesisSpermSwarm;
class UCineCameraComponent;
class USpotLightComponent;

/**
 * Kamera der Mikrowelt: eine Kamera mit Masse (weich gedämpfte Nachführung), echter Brennweite/Blende und Schärfenachführung
 * wie ein Fokus-Assistent. Einzige Lichtquelle ist das Endoskoplicht direkt an der Optik – im Körper gibt es kein anderes Licht.
 */
UCLASS()
class GENESISCONCEPTION_API AGenesisMicroscopeCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AGenesisMicroscopeCameraRig();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Follow")
	TObjectPtr<AGenesisSpermSwarm> Swarm;

	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "0"))
	int32 FollowCellIndex = 0;

	/** Beim Start automatisch eine gut sichtbare Zelle wählen: frei im Kanal schwimmend statt an der Wand klebend. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	bool bAutoPickCell = true;

	/** Abstand zum Kopf (µm). */
	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "10"))
	float OrbitDistanceUm = 110.0f;

	/** 0° = direkt vor dem Kopf, 180° = hinter der Zelle. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	float OrbitAzimuthDegrees = 125.0f;

	UPROPERTY(EditAnywhere, Category = "Follow")
	float OrbitElevationDegrees = 18.0f;

	/** Blickpunkt hinter der Kopfspitze (µm) – Kopf und Mittelstück im Bild. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	float LookBehindHeadUm = 10.0f;

	/** Zeitkonstante der Nachführung in Echtzeit (s) – die Kamera hat Masse. */
	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "0.01"))
	float PositionSmoothingSeconds = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "0.01"))
	float RotationSmoothingSeconds = 0.6f;

	/** Schärfenachführung wie ein Fokus-Assistent (s). */
	UPROPERTY(EditAnywhere, Category = "Follow", meta = (ClampMin = "0.01"))
	float FocusSmoothingSeconds = 0.35f;

	/** Beim Start zur Spielerkamera machen. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	bool bBecomeViewTarget = true;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpotLightComponent> EndoscopeLight;

private:
	bool ComputeDesired(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const;
	void UpdateBeatNormal(float DeltaSeconds);

	FVector SmoothedBeatNormal = FVector::UpVector;
	bool bInitialized = false;
	bool bDebugPageRegistered = false;
	float CurrentFocusDistance = 0.0f;
};
