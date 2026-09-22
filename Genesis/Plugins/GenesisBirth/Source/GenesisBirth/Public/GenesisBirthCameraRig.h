// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisBirthTypes.h"
#include "GenesisBirthCameraRig.generated.h"

class UCineCameraComponent;

/**
 * Die Geburt aus den Augen des Kindes.
 *
 * Maßstab der Geburtsszene: 1 mm = 1 Unreal-Einheit. Der Geburtskanal ist etwa 100 mm lang –
 * in Zentimetern gerechnet wäre er zehn Einheiten groß und damit kleiner als die Nahgrenze jeder Kamera.
 *
 * Die Kamera erzählt nichts. Sie zeigt nur, was das Kind hat: Dunkelheit, Druck, Enge,
 * einen Herzschlag, der bei jeder Wehe langsamer wird – und am Ende Licht, das viel zu hell ist.
 */
UCLASS()
class GENESISBIRTH_API AGenesisBirthCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AGenesisBirthCameraRig();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Länge des Kanals in mm: Von hier aus schiebt sich das Kind zum Ausgang (Ursprung des Actors). */
	UPROPERTY(EditAnywhere, Category = "Birth", meta = (ClampMin = "10"))
	float CanalLengthMm = 105.0f;

	/** Wie weit das Kind nach der Geburt noch aus dem Kanal heraus getragen wird (mm). */
	UPROPERTY(EditAnywhere, Category = "Birth")
	float AfterBirthTravelMm = 160.0f;

	/** Sehschärfe eines Neugeborenen: Es sieht nur auf etwa 25 cm scharf, alles andere verschwimmt. */
	UPROPERTY(EditAnywhere, Category = "Birth")
	float NewbornFocusDistanceMm = 250.0f;

	/**
	 * Makro-Vergrößerung der Optik: Sensor und Brennweite wachsen gemeinsam, der Bildwinkel bleibt.
	 * Nur so entsteht die geringe Schärfentiefe, mit der ein Neugeborenes sieht – eine normale Linse
	 * hätte bei 25 cm Motivabstand alles scharf, und genau das sieht ein Säugling nicht.
	 */
	UPROPERTY(EditAnywhere, Category = "Optics", meta = (ClampMin = "1", ClampMax = "30"))
	float MacroScale = 6.0f;

	/** Belichtung im Kanal und draußen (EV). Der Sprung ins Licht ist der stärkste Reiz des ganzen Lebens. */
	UPROPERTY(EditAnywhere, Category = "Exposure")
	float DarkExposureBias = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Exposure")
	/**
	 * Draußen (nach der Geburt). Gemessen im Kreißsaal (GENESIS-035): Mit +3 EV lag der Median des Bildes
	 * bei 0,79 – weiße Wände, untergehende Bildschirme. +1,6 EV ergibt einen gedimmten Raum, in dem das
	 * Fenster hell und das CTG sichtbar leuchtet. Der Sprung ins Licht bleibt: im Kanal +4 EV auf fast nichts.
	 */
	float LightExposureBias = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Birth")
	bool bBecomeViewTarget = true;

	/**
	 * Blickrichtung relativ zur Ruhelage (Grad, X = seitlich, Y = hoch).
	 * Ein Neugeborenes kann den Kopf kaum halten – viel mehr als ein Wenden zur Stimme ist nicht drin.
	 * Gesetzt wird der Wert von der Steuerung; der Rig rechnet ihn auf die Kamera.
	 */
	FVector2D LookOffsetDegrees = FVector2D::ZeroVector;

	/**
	 * Liegt das Kind auf der Brust der Mutter? Gesetzt von außen (Regie/Spielmodus), sobald die
	 * Hebamme es auf die Haut legt. Der Rig hebt die Kamera dann in einem Bogen über den Bauch.
	 */
	bool bOnMothersChest = false;

	/**
	 * Augen des Kindes auf der Brust (mm, Kreißsaal-Koordinaten): bäuchlings auf der rechten Brust,
	 * die Wange auf der Haut, gut 4 cm über ihr. Abgeleitet aus dem Aufbau in
	 * Tools/Blender/Birth/build_delivery_room.py (Rückenteil 45°, Brust 360 mm das Rückenteil hinauf).
	 * Y ist gegenüber Blender gespiegelt (FBX): Das Fenster liegt in Unreal bei −Y, der Raum mit CTG bei +Y.
	 */
	UPROPERTY(EditAnywhere, Category = "Chest")
	FVector ChestEyeLocation = FVector(-538.0, 60.0, 438.0);

	/**
	 * Blick von der Brust: Der Kopf liegt zur Seite gedreht – so liegen Neugeborene beim Hautkontakt,
	 * damit Nase und Mund frei bleiben. Der Blick geht vom Fenster weg quer über die Brust in den Raum:
	 * CTG, Wärmebett, Tür. Zum Fenster hin sähe das Kind nur eine überstrahlte helle Fläche – das
	 * Tageslicht kommt so von der Seite, und der Raum bekommt Tiefe.
	 */
	UPROPERTY(EditAnywhere, Category = "Chest")
	FVector ChestViewForward = FVector(0.30, 0.95, -0.08);

	/**
	 * „Oben" für das liegende Kind: Der Scheitel zeigt zum Kinn der Mutter, also halb nach oben und
	 * halb das Rückenteil hinauf. Daraus ergibt sich ein um gut 20° geneigter Horizont.
	 */
	UPROPERTY(EditAnywhere, Category = "Chest")
	FVector ChestViewUp = FVector(-0.35, 0.0, 0.94);

	/** Wie lange das Hinüberheben dauert (s) und wie hoch der Bogen über den Bauch geht (mm). */
	UPROPERTY(EditAnywhere, Category = "Chest")
	float LiftSeconds = 7.0f;

	UPROPERTY(EditAnywhere, Category = "Chest")
	float LiftArcMm = 380.0f;

	/** Atem der Mutter: 14 Züge je Minute, die Brust hebt sich um wenige Millimeter. */
	UPROPERTY(EditAnywhere, Category = "Chest")
	float MotherBreathsPerMinute = 14.0f;

	UPROPERTY(EditAnywhere, Category = "Chest")
	float MotherBreathMm = 4.0f;

	/**
	 * Atemhub der Mutter (0..1), gesetzt von der Regie, sobald es eine Mutter mit eigenem Atem gibt.
	 * Dann liegt das Kind auf genau diesem Atem statt auf einem eigenen Takt – sonst schwebte es
	 * über einer Brust, die sich im anderen Rhythmus hebt. Negativ = eigener Takt.
	 */
	float MotherBreathLift = -1.0f;

	/**
	 * Die Mutter holt das Kind vor ihr Gesicht (0..1, von der Regie). Die Kamera geht dann zu
	 * EnFaceView: die Augen des Kindes auf Sehschärfe-Abstand vor ihren, der Blick auf ihre Augen.
	 */
	float EnFaceBlend = 0.0f;
	FTransform EnFaceView = FTransform::Identity;

	/**
	 * Die Hebamme hat das Kind aufgefangen und zu sich hochgehoben (0..1, von der Regie). Die Kamera liegt dann
	 * in ihren Händen (MidwifeHeldView, Welt): gut 38 cm vor ihrem Gesicht, der Blick zu ihr hoch. Von dort aus
	 * legt sie das Kind der Mutter auf die Brust.
	 */
	float MidwifeHoldBlend = 0.0f;
	FTransform MidwifeHeldView = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

private:
	void ApplyPerception(const FGenesisBirthState& State, const FGenesisBirthPerception& Perception, float DeltaSeconds);

	/** Herzschlag als Bewegung: Das Kind spürt seinen eigenen Puls, und er wird unter der Wehe langsamer. */
	float HeartPhase = 0.0f;
	float SmoothedPressure = 0.0f;
	float SmoothedLight = 0.0f;
	/** 0 = in den Händen der Hebamme am Fußende, 1 = auf der Brust. */
	float ChestBlend = 0.0f;
	float BreathPhase = 0.0f;
};
