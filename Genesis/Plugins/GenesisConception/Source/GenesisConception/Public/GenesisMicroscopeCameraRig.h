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

	/** Sobald die erste Zelle an der Zona hängt, schneidet die Kamera auf die Eizelle – der entscheidende Moment. */
	UPROPERTY(EditAnywhere, Category = "Oocyte")
	bool bWatchFertilization = true;

	/** Blick auf die Eizelle statt auf eine Zelle (Konsole: genesis.Conception.WatchOocyte 1). */
	UPROPERTY(EditAnywhere, Category = "Oocyte")
	bool bWatchOocyte = false;

	/** Abstand zur Eizelle (µm). 420 µm zeigen mit 50 mm Brennweite den ganzen Eizell-Cumulus-Komplex. */
	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "80"))
	float OocyteDistanceUm = 420.0f;

	/** Langsame Umkreisung der Eizelle (rad/s Echtzeit). */
	UPROPERTY(EditAnywhere, Category = "Oocyte")
	float OocyteOrbitSpeed = 0.06f;

	/**
	 * Belichtung der Kamera (EV, größer = dunkler). Die Kamera bestimmt sie selbst, nicht ein Postprocess-Volume:
	 * Blende und Brennweite dürfen das Bild sonst über die physikalische Kamerabelichtung mit aufhellen.
	 */
	UPROPERTY(EditAnywhere, Category = "Exposure")
	float ExposureBias = 14.0f;

	/** Beim Start zur Spielerkamera machen. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	bool bBecomeViewTarget = true;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpotLightComponent> EndoscopeLight;

private:
	bool ComputeDesired(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const;
	bool ComputeOocyteView(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const;
	void UpdateBeatNormal(float DeltaSeconds);

	FVector SmoothedBeatNormal = FVector::UpVector;
	float OocyteOrbitPhase = 0.0f;
	bool bInitialized = false;
	bool bDebugPageRegistered = false;
	float CurrentFocusDistance = 0.0f;
};
