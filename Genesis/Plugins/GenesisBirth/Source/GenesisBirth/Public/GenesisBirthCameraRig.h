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
	float LightExposureBias = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Birth")
	bool bBecomeViewTarget = true;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCineCameraComponent> Camera;

private:
	void ApplyPerception(const FGenesisBirthState& State, const FGenesisBirthPerception& Perception, float DeltaSeconds);

	/** Herzschlag als Bewegung: Das Kind spürt seinen eigenen Puls, und er wird unter der Wehe langsamer. */
	float HeartPhase = 0.0f;
	float SmoothedPressure = 0.0f;
	float SmoothedLight = 0.0f;
};
