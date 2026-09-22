// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisEmbryoTypes.h"
#include "GenesisImplantationSite.generated.h"

class AGenesisEmbryo;
class UCineCameraComponent;
class USpotLightComponent;
class UMaterialParameterCollection;
class UStaticMeshComponent;

/**
 * Was die Einnistungsstelle an die Szene weitergibt – reine Rechnung, damit sie testbar bleibt.
 * Längen in µm (= Unreal-Einheiten), Höhen relativ zur Oberfläche der Schleimhaut.
 */
struct GENESISEMBRYO_API FGenesisImplantationView
{
	/** Sichtbarer Radius des Keims (µm). */
	float RadiusUm = 100.0f;
	/** Höhe der Keimmitte über der Oberfläche (negativ = darunter). */
	float CenterHeightUm = 100.0f;
	/** Schnittradius zwischen Keim und Oberfläche (0 = berührt nicht oder ganz darunter). */
	float WaterlineRadiusUm = 0.0f;
	/** 0..1 – wie weit sich der Embryonalpol zur Schleimhaut gedreht hat. */
	float Orientation = 0.0f;
	/** 0..1 – Wulst des Epithels, das den einsinkenden Keim umschließt. */
	float Collar = 0.0f;
	/** Radius des offenen Defekts mit Fibrinpfropf über dem versunkenen Keim (µm). */
	float PlugRadiusUm = 0.0f;
	/** Wölbung der Oberfläche über dem wachsenden Keim (µm). */
	float DomeHeightUm = 0.0f;
	/** 0..1 – mütterliches Blut, das durch das dünne Gewebe über dem Keim dunkelrot durchscheint. */
	float BloodShowing = 0.0f;
	/** 0..1 – Hyperämie der Deziduareaktion rund um die Stelle. */
	float Hyperemia = 0.0f;
	/** Ist vom Keim selbst noch etwas über der Oberfläche zu sehen? */
	bool bEmbryoVisible = true;
	/** Bildbreite am Keim, die die Kamera zeigen soll (µm). */
	float FramingWidthUm = 800.0f;
};

namespace GenesisImplantationView
{
	/**
	 * Die Einnistung als Bild. OuterRadiusUm ist der Radius des Keims, wie ihn die Zellen darstellen (ohne Maßstab).
	 * Vor dem Anlegen sinkt der geschlüpfte Keim durch die Uterusflüssigkeit auf die Schleimhaut.
	 */
	GENESISEMBRYO_API FGenesisImplantationView Compute(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning);
}

/**
 * Die Einnistungsstelle in der Gebärmutter (GENESIS-040): Schleimhaut, Keim, Hysteroskop-Kamera und -Licht.
 *
 * Die Schleimhaut ist ein Nanite-Mesh (Tools/Blender/Implantation/build_endometrium.py). Was sich mit der Einnistung ändert –
 * Wulst, Fibrinpfropf, Wölbung, durchscheinendes Blut, Hyperämie – schreibt dieser Actor in eine Material-Parameter-Sammlung,
 * das Material der Schleimhaut formt daraus Oberfläche und Farbe. Der Keim-Actor wird auf die Stelle gesetzt,
 * mit dem Embryonalpol nach unten gedreht und auf seine Größe gebracht.
 *
 * Der Actor steht genau auf der Oberfläche der Schleimhaut über der Einnistungsstelle.
 */
UCLASS()
class GENESISEMBRYO_API AGenesisImplantationSite : public AActor
{
	GENERATED_BODY()

public:
	AGenesisImplantationSite();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Implantation")
	TObjectPtr<AGenesisEmbryo> Embryo;

	/** MPC_GEN_Implantation – liest das Material der Schleimhaut. */
	UPROPERTY(EditAnywhere, Category = "Implantation")
	TObjectPtr<UMaterialParameterCollection> Parameters;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/**
	 * Der Keim, wie ihn das Hysteroskop von außen sieht: eine glasige Hülle aus flachen, vieleckigen Trophoblastzellen
	 * (Kugel mit 55 µm Radius, Zellmuster im Material). Die Zellen der Simulation als Einzelkugeln wirkten wie Popcorn;
	 * die Einzelzellen zeigt das Mikroskop der ersten Woche.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Conceptus;

	/**
	 * Der Embryoblast, durch die dünne Hülle als trüber Zellknoten zu sehen. Beim Anlegen dreht er sich zur Schleimhaut:
	 * Mit diesem Pol nistet sich der Keim ein.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> InnerCellMass;

	/**
	 * Die leere Zona pellucida, aus der der Keim geschlüpft ist: Sie bleibt eingefallen in der Gebärmutter liegen
	 * und löst sich in den folgenden Tagen auf.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EmptyZona;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

	/** Licht des Hysteroskops: eine Kaltlichtquelle neben der Optik, sonst ist es in der Gebärmutter dunkel. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpotLightComponent> ScopeLight;

	/** Blickwinkel über der Schleimhaut (Grad) und langsame Drift um die Stelle (Grad je Sekunde). */
	UPROPERTY(EditAnywhere, Category = "Camera") float ElevationDegrees = 34.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float AzimuthDegrees = -80.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float OrbitDegreesPerSecond = 0.6f;

	/** Die Kamera hat Masse: Sie folgt der Wunschlage gedämpft (Zeitkonstante in Sekunden). */
	UPROPERTY(EditAnywhere, Category = "Camera") float FollowSeconds = 1.6f;

	/**
	 * Beleuchtungsstärke am Keim (lx). Wie ein Hysteroskop regelt das Licht mit dem Abstand nach, damit das Bild gleich hell
	 * bleibt, wenn die Kamera beim Wachsen des Keims zurückweicht. Maßstab der Mikrowelt: 1 µm = 1 cm.
	 */
	UPROPERTY(EditAnywhere, Category = "Camera") float TargetIlluminanceLux = 90.0f;

	/**
	 * Belichtung der Kamera (EV, größer = heller), wie beim Mikroskop: feste Belichtung aus Blende, Verschlusszeit und ISO
	 * plus dieser Ausgleich für den Maßstabssprung (im Mikrometerraum trifft die Optik nur wenige Lux).
	 */
	UPROPERTY(EditAnywhere, Category = "Camera") float ExposureBias = 8.5f;

	/** Übernimmt beim Start die Sicht des Spielers. */
	UPROPERTY(EditAnywhere, Category = "Camera") bool bTakeView = true;

	/** Zuletzt berechnetes Bild (für Tests und Entwickler). */
	const FGenesisImplantationView& GetView() const { return View; }

private:
	void PlaceEmbryo(const FGenesisEmbryoState& State, float DeltaSeconds);
	void PlaceZona(const FGenesisEmbryoState& State);
	void PushParameters();
	void UpdateCamera(float DeltaSeconds, bool bSnap);

	FGenesisImplantationView View;
	FVector CameraLocation = FVector::ZeroVector;
	FVector CameraFocus = FVector::ZeroVector;
	float OrbitDegrees = 0.0f;
	bool bCameraInitialized = false;
};
