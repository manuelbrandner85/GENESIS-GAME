// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisMicroscopeCameraRig.generated.h"

class AGenesisSpermSwarm;
class UCineCameraComponent;
class USpotLightComponent;
class UPointLightComponent;

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

	/** Überträgt Makro-Faktor, Bildwinkel und Blende auf die Kamera. */
	void ApplyOptics();

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

	/** Nahaufnahme: Sobald eine Zelle an der Zona hängt, geht die Kamera zwischen die Coronazellen an die Eintrittsstelle. */
	UPROPERTY(EditAnywhere, Category = "Oocyte")
	bool bCloseUpOnBinding = true;

	/**
	 * Automatische Lichtregelung wie am Endoskop: kurzer Arbeitsabstand → weniger Licht.
	 *
	 * Seit GENESIS-026 an: Zusammen mit dem Streulicht der Umgebung hält sie alle Arbeitsabstände
	 * belichtet – vorher war nur der Bereich um 430 µm brauchbar, die Nahaufnahme brannte aus und
	 * die weite Einstellung fiel ins Schwarze. (Ohne Streulicht war die Regelung unbrauchbar:
	 * Dann bekam die Umgebung in Nahaufnahmen gar kein Licht mehr ab.)
	 */
	UPROPERTY(EditAnywhere, Category = "Light")
	bool bAutoLightControl = true;

	/** Lichtstärke beim Bezugsabstand (cd). */
	UPROPERTY(EditAnywhere, Category = "Light")
	float LightCandelasAtReference = 150.0f;

	/** Bezugsabstand der Lichtregelung (µm). */
	UPROPERTY(EditAnywhere, Category = "Light")
	float LightReferenceDistanceUm = 430.0f;

	/**
	 * Streulicht der Umgebung (cd). Es hängt nicht am Arbeitsabstand, denn die Wand des Eileiters
	 * bleibt gleich weit weg – es hält nur den Raum neben dem Lichtkegel lesbar.
	 */
	UPROPERTY(EditAnywhere, Category = "Light")
	float FillCandelas = 18.0f;

	/** Unterhalb des Bezugsabstands: Abstandsquadrat, wie es die Physik vorgibt. */
	UPROPERTY(EditAnywhere, Category = "Light", meta = (ClampMin = "0.5", ClampMax = "2.5"))
	float LightFalloffExponentNear = 2.0f;

	/** Oberhalb des Bezugsabstands: flacher, sonst überstrahlen die nahen Falten die weite Einstellung. */
	UPROPERTY(EditAnywhere, Category = "Light", meta = (ClampMin = "0.5", ClampMax = "2.5"))
	float LightFalloffExponentFar = 1.0f;

	/**
	 * Makro-Vergrößerung der Optik: Sensor und Brennweite werden gemeinsam um diesen Faktor vergrößert.
	 *
	 * Der Bildausschnitt bleibt dabei exakt gleich (das Verhältnis entscheidet über den Blickwinkel),
	 * aber der Zerstreuungskreis wächst mit – erst dadurch entsteht die extrem flache Schärfentiefe,
	 * die jede Mikroskopaufnahme hat. Ohne das ist im Mikrometerraum alles scharf, und das Bild wirkt
	 * wie ein Modell aus Kunststoff. Echte Makro-Optik (hier ~15:1) lässt sich mit einer dünnen Linse
	 * in Weltmaßstab sonst nicht nachbilden.
	 */
	UPROPERTY(EditAnywhere, Category = "Optics", meta = (ClampMin = "1", ClampMax = "40"))
	float MacroScale = 18.0f;

	/** Bildwinkel wie bei dieser Kleinbild-Brennweite (mm); die tatsächliche Brennweite ist das Makro-Vielfache davon. */
	UPROPERTY(EditAnywhere, Category = "Optics", meta = (ClampMin = "8", ClampMax = "200"))
	float NominalFocalLengthMm = 50.0f;

	/** Blende. Wirkt auf Schärfentiefe und – wie bei einer echten Kamera – auf die Helligkeit. */
	UPROPERTY(EditAnywhere, Category = "Optics", meta = (ClampMin = "1.2", ClampMax = "22"))
	float Aperture = 11.0f;

	/**
	 * Belichtung der Kamera (EV, größer = heller). Die Kamera bestimmt sie selbst, nicht ein Postprocess-Volume.
	 * Der hohe Wert gleicht den Maßstabssprung aus: Im Mikrometerraum trifft die Optik nur wenige Lux.
	 */
	UPROPERTY(EditAnywhere, Category = "Exposure")
	float ExposureBias = 13.0f;

	/** Beim Start zur Spielerkamera machen. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	bool bBecomeViewTarget = true;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpotLightComponent> EndoscopeLight;

	/** Streulicht der Umgebung – ohne es ist neben dem Lichtkegel alles schwarz. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> FillLight;

private:
	bool ComputeDesired(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const;
	bool ComputeOocyteView(FVector& OutLocation, FQuat& OutRotation, float& OutFocusDistance) const;

	void UpdateBeatNormal(float DeltaSeconds);
	/** Regelt das Endoskoplicht auf den Arbeitsabstand. */
	void UpdateLight();

	FVector SmoothedBeatNormal = FVector::UpVector;
	float OocyteOrbitPhase = 0.0f;
	bool bInitialized = false;
	bool bDebugPageRegistered = false;
	float CurrentFocusDistance = 0.0f;
};
