// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisEmbryogenesisTypes.h"
#include "GenesisEmbryoScene.generated.h"

class UCineCameraComponent;
class USpotLightComponent;
class UStaticMeshComponent;

/** Was die Fruchthöhle aus der Simulation zeigt – reine Rechnung, damit sie testbar bleibt. */
struct GENESISEMBRYO_API FGenesisEmbryoSceneView
{
	/** Maßstab des Körpers: die simulierte Länge gegen die Länge des Modells (Tag 28, 4,6 mm). */
	float Scale = 1.0f;
	/** 0..1 – wie weit sich das Herz gerade zusammenzieht (0 = entspannt). */
	float HeartContraction = 0.0f;
	bool bHeartBeating = false;
	float HeartRateBpm = 0.0f;
	/** Tag nach der Befruchtung. */
	float Day = 0.0f;
};

namespace GenesisEmbryoSceneView
{
	/** Länge des Modells (mm): Tools/Blender/Embryogenesis/build_embryo_day28.py baut den Embryo am Ende der vierten Woche. */
	constexpr float ModelLengthMm = 4.6f;

	/**
	 * Ein Herzschlag als Verlauf über einen Zyklus (Phase 0..1): Die Kammer zieht sich rasch zusammen, erschlafft langsamer,
	 * dann Ruhe bis zum nächsten Schlag. Beim embryonalen Herzschlauch sichtbar als Welle der ganzen Schleife.
	 */
	GENESISEMBRYO_API float HeartContraction(double CyclePhase);

	/** BeatCycles: aufsummierte Herzzyklen (Echtzeit × Frequenz), die Nachkommastelle ist die Phase. */
	GENESISEMBRYO_API FGenesisEmbryoSceneView Compute(const FGenesisEmbryogenesisState& State, double HoursSinceFusion, double BeatCycles);
}

/**
 * Die Fruchthöhle am Ende der vierten Woche (GENESIS-041 Teil 5): der Embryo im Amnion, daneben der Dottersack.
 *
 * Der Embryo besteht aus einer durchscheinenden Hülle und den Organen darin (Tools/Blender/Embryogenesis). Dieser Actor
 * trägt sie, bringt sie auf die simulierte Länge und lässt das Herz im simulierten Takt schlagen – in Echtzeit, auch
 * wenn der Zeitraffer läuft. Die Umgebung (Amnion, Dottersack, Stiele, Wand der Chorionhöhle) steht fest in der Karte.
 *
 * Die Kamera ist ein Embryoskop: Optik mit Lichtleiter, sonst ist es dunkel. Sie pendelt langsam um den Embryo.
 * In der Schwangerschaft (nach Tag 29) blendet sie ab – für die Wochen danach gibt es hier nichts Wahres zu zeigen.
 */
UCLASS()
class GENESISEMBRYO_API AGenesisEmbryoScene : public AActor
{
	GENERATED_BODY()

public:
	AGenesisEmbryoScene();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USceneComponent> Root;
	/** Träger des ganzen Körpers: wird auf die simulierte Länge skaliert. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USceneComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Shell;
	/** Drehpunkt des Herzens in seiner Mitte, damit es sich um sich selbst zusammenzieht. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USceneComponent> HeartPivot;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Heart;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Vessels;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> NeuralTube;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Somites;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Liver;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UCineCameraComponent> Camera;
	/** Lichtleiter des Embryoskops neben der Optik. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USpotLightComponent> ScopeLight;

	/** Wie weit sich der Herzschlauch zusammenzieht (Anteil des Durchmessers). */
	UPROPERTY(EditAnywhere, Category = "Heart") float HeartAmplitude = 0.14f;

	/** Bildbreite am Embryo (µm) und Blickpunkt relativ zum Actor (zwischen Embryo und Dottersack). */
	UPROPERTY(EditAnywhere, Category = "Camera") float FramingWidthUm = 10500.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") FVector FocusOffset = FVector(-700.0, 600.0, -600.0);
	UPROPERTY(EditAnywhere, Category = "Camera") float FocalLengthMm = 28.0f;
	/** Blickrichtung (Grad) und das langsame Pendeln um sie herum. */
	UPROPERTY(EditAnywhere, Category = "Camera") float AzimuthDegrees = 128.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float ElevationDegrees = 14.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float SwayDegrees = 12.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float SwayPeriodSeconds = 40.0f;
	/** Die Kamera hat Masse: Sie folgt der Wunschlage gedämpft (Zeitkonstante in Sekunden). */
	UPROPERTY(EditAnywhere, Category = "Camera") float FollowSeconds = 1.6f;

	/** Beleuchtungsstärke am Embryo (lx) – der Lichtleiter regelt mit dem Abstand nach. */
	UPROPERTY(EditAnywhere, Category = "Camera") float TargetIlluminanceLux = 4.5f;
	/** Belichtungsausgleich (EV) bei fester Belichtung. */
	UPROPERTY(EditAnywhere, Category = "Camera") float ExposureBias = 0.0f;

	/** Ab diesem Tag (Schwangerschaft) blendet die Kamera in FadeSeconds ab. */
	UPROPERTY(EditAnywhere, Category = "Camera") float FadeFromDay = 29.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float FadeSeconds = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Camera") bool bTakeView = true;

	const FGenesisEmbryoSceneView& GetView() const { return View; }

private:
	void UpdateCamera(float DeltaSeconds, bool bSnap);
	void UpdateFade(float DeltaSeconds);

	FGenesisEmbryoSceneView View;
	double BeatCycles = 0.0;
	float SceneSeconds = 0.0f;
	float FadeAlpha = 0.0f;
	FVector CameraLocation = FVector::ZeroVector;
	FVector CameraFocus = FVector::ZeroVector;
	bool bCameraInitialized = false;
};
