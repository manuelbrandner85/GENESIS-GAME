// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisFetalTypes.h"
#include "GenesisMotherDay.h"
#include "GenesisWombScene.generated.h"

class UAudioComponent;
class UCineCameraComponent;
class USoundBase;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Was das Kind in diesem Augenblick wahrnimmt (GENESIS-044 Teil 1b, Docs/34) – aus seinem Körper (Sinne) und dem
 * Tag der Mutter (Licht). Rein rechnerisch, damit testbar.
 */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisWombPerception
{
	GENERATED_BODY()

	/** Bewusstes Erleben (0..1): vor der Verbindung Thalamus–Rinde (SSW 23–26) gedämpft und bruchstückhaft. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Womb") float Presence = 0.0f;
	/** Helligkeit, die ankommt (0..1): Licht im Mutterleib × Lichtwahrnehmung × Lider. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Womb") float Brightness = 0.0f;
	/** Lider offen (0..1). Mit geschlossenen Lidern nur ein unscharfes, rotes Leuchten. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Womb") float EyesOpen = 0.0f;
	/** Unschärfe (0..1): geschlossene Lider ganz, offene Augen immer noch stark – ein Fetus sieht nur Helligkeit. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Womb") float Blur = 1.0f;
	/** Innenradius der Gebärmutterhöhle (cm) in dieser Woche. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Womb") float CavityRadiusCm = 5.0f;
};

namespace GenesisWombPerception
{
	/**
	 * Wahrnehmung aus den Sinnen des Kindes und dem Licht im Mutterleib (lx).
	 * Helligkeit: logarithmisch zwischen 0,05 lx (kaum) und 50 lx (hell für ein Auge im Dunkeln).
	 */
	GENESISSLICE_API FGenesisWombPerception Compute(const FGenesisFetalView& Fetal, float WombLux);

	/**
	 * Innenradius der Gebärmutterhöhle (cm). Näherung aus dem Symphysen-Fundus-Abstand (≈ SSW in cm ab SSW 20) und der
	 * Fruchtblase im Ultraschall (SSW 8: ~2,5 cm Durchmesser der Chorionhöhle); die Höhle ist gut halb so hoch wie lang.
	 */
	GENESISSLICE_API float CavityRadiusCm(float GestationalWeeks);
}

/**
 * Der Mutterleib aus Sicht des Kindes (GENESIS-044 Teil 1b): Die Kamera ist das Kind. Was es sieht und hört, folgt
 * seinen Sinnen (GenesisFetalLogic) und dem Tag der Mutter (GenesisMotherDay) – vor SSW 19 Stille und Dunkel, dann ihre
 * Stimme als Melodie, ab SSW 26–28 rotes Licht durch den Bauch. Seine eigenen Bewegungen spürt der Spieler als
 * Bewegung der Kamera (Schreck, Tritte, Schluckauf), das Gehen der Mutter als Wiegen.
 */
UCLASS()
class GENESISSLICE_API AGenesisWombScene : public AActor
{
	GENERATED_BODY()

public:
	AGenesisWombScene();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USceneComponent> Root;
	/** Die Höhle und alles darin; wird mit der Woche skaliert (gebaut für 10 cm Innenradius). */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<USceneComponent> Cavity;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Wall;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Placenta;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> PlacentaVessels;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Cord;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> CordVessels;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UCineCameraComponent> Camera;
	/** Die Stimme der Mutter, durch ihren Körper und das Gehör des Kindes gefiltert. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UAudioComponent> MotherVoice;

	/** Ihre Sätze an den Bauch: ab SSW 16 (sie spricht mit dem Kind, auch bevor sie es spürt). */
	UPROPERTY(EditAnywhere, Category = "Voice") TArray<TObjectPtr<USoundBase>> BellyLines;
	/** Nach dem ersten Tritt, den sie spürt (ab SSW 20): „Hallo, du da drin. Na – bist du wach?“ */
	UPROPERTY(EditAnywhere, Category = "Voice") TObjectPtr<USoundBase> KickLine;
	/** Gegen Ende (ab SSW 36): „Nicht mehr lange, dann sehen wir uns.“ */
	UPROPERTY(EditAnywhere, Category = "Voice") TObjectPtr<USoundBase> LateLine;

	/** Richtung zum Bauch der Mutter (vorn): Durch die vordere Wand kommt das Licht. */
	UPROPERTY(EditAnywhere, Category = "Light") FVector BellyDirection = FVector(1.0, 0.0, 0.0);
	/** Leuchtdichte der Wand je lx im Mutterleib (Material-Emission), gegen das Bild abgeglichen. */
	UPROPERTY(EditAnywhere, Category = "Light") float GlowPerLux = 0.03f;
	/** Blickrichtung des Kindes (in der Höhle): nach vorn ins Licht, leicht hinauf und zur Seite. */
	UPROPERTY(EditAnywhere, Category = "Camera") FVector LookDirection = FVector(1.0, 0.35, 0.25);
	/**
	 * Unreal-Einheiten je Zentimeter. Lumen gibt Teilen unter ~10 Einheiten keine Oberflächenkarten – bei 1 cm = 1 Einheit
	 * bekäme die Nabelschnur kein Streulicht von der leuchtenden Wand und stünde schwarz im Bild. Die Optik (Fokus, Blende)
	 * wird mitskaliert, damit die Unschärfe dieselbe bleibt wie im echten Maßstab.
	 */
	UPROPERTY(EditAnywhere, Category = "Camera") float WorldScale = 10.0f;
	/** Belichtung (EV, manuell): Das Auge im Dunkeln ist empfindlich; 0 = wie abgeglichen. */
	UPROPERTY(EditAnywhere, Category = "Camera") float ExposureBias = 0.0f;
	/** Abgleich: Bei diesem Licht im Mutterleib (lx) und dieser gefühlten Helligkeit gilt ExposureBias (SSW 31, draußen). */
	UPROPERTY(EditAnywhere, Category = "Camera") float ReferenceLux = 11.0f;
	UPROPERTY(EditAnywhere, Category = "Camera") float ReferenceBrightness = 0.66f;

	/** Ohne Durchlauf (Prüfkarte, Bildschirmfoto): SSW und Uhrzeit von Hand (genesis.Womb.Preview). */
	UPROPERTY(EditAnywhere, Category = "Preview") float PreviewWeeks = 28.0f;
	UPROPERTY(EditAnywhere, Category = "Preview") float PreviewHour = 16.8f;
	/** Prüfansicht: Licht im Mutterleib (lx) fest vorgeben, < 0 = aus dem Tag der Mutter. */
	UPROPERTY(EditAnywhere, Category = "Preview") float PreviewWombLux = -1.0f;

	const FGenesisWombPerception& GetPerception() const { return Perception; }
	const FGenesisMotherMoment& GetMotherMoment() const { return Mother; }
	float GetGestationalWeeks() const { return Weeks; }

private:
	void UpdateTime();
	void UpdateSound(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds, const TArray<EGenesisFetalEvent>& Events);

	/** Materialien, die das Licht durch den Bauch zeigen (Wand leuchtend, Gewebe durchscheinend). */
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> LitMaterials;

	float Weeks = 28.0f;
	double HourOfDay = 12.0;
	int32 DayIndex = 0;
	int32 Seed = 1;
	FGenesisMotherMoment Mother;
	FGenesisWombPerception Perception;
	FGenesisFetalBehaviour Behaviour;
	FRandomStream Random;
	float SceneSeconds = 0.0f;
	bool bInRun = false;
	float JoltSeconds = 0.0f;
	float JoltStrength = 0.0f;
	FVector JoltDirection = FVector::ZeroVector;
	float SinceVoice = 1000.0f;
	bool bKickLineSaid = false;
	bool bLateLineSaid = false;
	int32 LastMomentIndex = -2;
};
