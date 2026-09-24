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
class UPoseableMeshComponent;

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
 * Was der Spieler als Kind in diesem Augenblick tut (GENESIS-044 Teil 2a, Docs/37) – vom PlayerController je Bild
 * gefüllt. Gedrückte Tasten zählen als Anzahl, gehaltene als Dauer.
 */
USTRUCT()
struct GENESISSLICE_API FGenesisWombInput
{
	GENERATED_BODY()

	/** Linker Stick / WASD: sich bewegen – mit gehaltener Hand-Taste die Hand. */
	FVector2D Move = FVector2D::ZeroVector;
	/** Rechter Stick / Maus: den Kopf drehen. */
	FVector2D Look = FVector2D::ZeroVector;
	bool bHandHeld = false;
	bool bGraspHeld = false;
	/** Gehalten: Hand zum Mund (Dauer s); kurz getippt: schlucken. */
	float MouthHeldSeconds = 0.0f;
	int32 MouthTaps = 0;
	/** Gehalten: gähnen (Dauer s); kurz getippt: strecken. */
	float StretchHeldSeconds = 0.0f;
	int32 StretchTaps = 0;
	int32 Kicks = 0;
	int32 EyeToggles = 0;
};

/** Das Kind in einem Alter (Blender build_fetus.py): Netz, Körpermitte und Nabel relativ zu den Augen (cm, +X Blick). */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisFetusStage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") float Weeks = 20.0f;
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") FVector Center = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") FVector Navel = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") float CrownRumpCm = 16.0f;
	/** Äußerste Punkte des Körpers (cm, relativ zu den Augen) – damit das Kind ganz in der Fruchtblase liegt. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") TArray<FVector> Hull;
	/** Halber Augenabstand (cm): Die Augen liegen im Netz bei (0, ±EyeHalfSpacingCm, 0) – für die Pigmentflecke. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") float EyeHalfSpacingCm = 0.0f;
	/** 0 = Augen blicken nach vorn (+X), 1 = seitlich (±Y) wie beim Embryo in SSW 8 – Achse der Pigmentflecke. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") float EyeSideways = 0.0f;
	/** Herz und Leber unter der dünnen Haut (Embryo): Mitte im Netz (cm) und Radius in W; W = 0 = nicht sichtbar. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") FVector4 HeartCm = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, Category = "Genesis|Womb") FVector4 LiverCm = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
};

/** Eine Handlung, die der Spieler in dieser Woche kann – für die Hinweise im Bild. */
struct GENESISSLICE_API FGenesisWombHint
{
	FName InputAction;
	FString Verb;
	bool bUsed = false;
};

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
	/** Die Nabelschnur: Skelettnetz mit Knochenkette, weich simuliert (SimulateCord). */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UPoseableMeshComponent> Cord;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UCineCameraComponent> Camera;
	/** Kaltlicht an der Optik – nur in den frühen Momenten von außen, wenn durch den Bauch noch kein Licht kommt. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<class UPointLightComponent> ScopeLight;
	/** Beleuchtungsstärke des Kaltlichts am Kind (lx, im echten Maßstab gerechnet). */
	UPROPERTY(EditAnywhere, Category = "Fetus") float ScopeLux = 25.0f;
	/** Das Kind selbst, von außen (Teil 2b) – nur während der Kamerafahrt in seine Augen sichtbar. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaticMeshComponent> Fetus;
	/** Die gebauten Alter des Kindes (SSW 12–40). */
	UPROPERTY(EditAnywhere, Category = "Fetus") TArray<FGenesisFetusStage> FetusStages;
	/** Dauer der Fahrt von außen in die Augen (s). */
	UPROPERTY(EditAnywhere, Category = "Fetus") float ExteriorSeconds = 9.0f;
	/** Die eigene Hand des Kindes (Teil 2a): an der Kamera, mit Skelett – öffnen, schließen, zum Mund. */
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UPoseableMeshComponent> OwnHand;
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
	UPROPERTY(EditAnywhere, Category = "Camera") FVector LookDirection = FVector(1.0, 0.25, 0.1);
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

	/** Wie lange ihre Hand auf dem Bauch bleibt, nachdem sie einen Tritt gespürt hat (s). */
	UPROPERTY(EditAnywhere, Category = "Mother") float MotherTouchSeconds = 9.0f;
	/** Wie oft sie auf einen gespürten Tritt mit der Hand antwortet (0..1). */
	UPROPERTY(EditAnywhere, Category = "Mother") float MotherTouchChance = 0.75f;

	const FGenesisWombPerception& GetPerception() const { return Perception; }
	const FGenesisMotherMoment& GetMotherMoment() const { return Mother; }
	float GetGestationalWeeks() const { return Weeks; }

	/** Eingabe des Spielers für dieses Bild (GENESIS-044 Teil 2a). */
	void SetPlayerInput(const FGenesisWombInput& InInput) { Input = InInput; }
	/** Was der Spieler in dieser Woche tun kann (und ob er es schon getan hat). */
	TArray<FGenesisWombHint> GetHints() const;
	/** Was das Kind gerade spürt, schmeckt, erlebt – eine Zeile, ein- und ausgeblendet (Alpha 0..1). */
	FString GetCaption(float& OutAlpha) const;
	/** Entwickler (genesis.Womb.Do): eine Handlung auslösen, als hätte der Spieler die Taste gedrückt. */
	void DebugAction(const FString& Action, const FVector2D& Value);
	/**
	 * Das Kind von außen zeigen, dann in seine Augen fahren (jeder neue Moment, genesis.Womb.Exterior).
	 * StaySeconds > 0: draußen bleiben, so lange (frühe Wochen ohne bewusstes Erleben, Docs/37 Teil 2c).
	 */
	void StartExterior(float StaySeconds = 0.0f);
	bool IsExterior() const { return ExteriorAge >= 0.0f; }

private:
	void UpdateTime();
	void UpdateSound(float DeltaSeconds);
	/** Die Handlungen des Spielers: Körper, Hand, Mund, Augen – und was davon in den Sinnen ankommt. */
	void UpdatePlayer(float DeltaSeconds, const FGenesisFetalView& Fetal, TArray<EGenesisFetalEvent>& Events);
	/** Sie spürt einen Tritt und legt die Hand auf den Bauch: Schatten im Licht, Druck, ihre Stimme. */
	void UpdateMotherTouch(float DeltaSeconds, const FGenesisFetalView& Fetal);
	void UpdateCamera(float DeltaSeconds, const TArray<EGenesisFetalEvent>& Events);
	void ShowCaption(const FString& Text);
	/** Ein Stoß am Controller – nur wenn das Kind schon tastet und Vibration eingeschaltet ist. */
	void Feel(float Intensity, float Seconds, bool bLarge = true);
	bool Can(EGenesisFetalAction Action) const;
	/** Die eigene Hand: Lage vor dem Gesicht oder am Mund, Finger nach Greifen und Ruhehaltung, Nabelschnur in der Hand. */
	void UpdateOwnHand(float DeltaSeconds);
	/** Handlänge (cm) in dieser Woche – Näherung: 0,83 × Fußlänge (Neugeborene: Hand ~6,2, Fuß ~7,5 cm). */
	float HandLengthCm() const;
	/** Das passende Alter des Kindes zur Woche; setzt FetusScale für das Wachstum dazwischen. */
	const FGenesisFetusStage* CurrentFetusStage();
	void UpdateExterior(float DeltaSeconds, const FVector& FirstPersonLocation, const FRotator& FirstPersonRotation);
	/** Die Augen so legen, dass alle Hüllpunkte des Kindes in der (etwas verkleinerten) Höhle liegen. */
	FVector FitFetus(const FGenesisFetusStage& Stage, const FQuat& Orientation, float Radius) const;
	/** Lage des Kindes: Körperlängsachse längs in der Höhle (Kopf oben bzw. unten), Gesicht zum Bauch. */
	FQuat FetusOrientation(const FGenesisFetusStage& Stage, const FVector& HeadUp) const;
	/** Knochen im Komponentenraum bei gegebener Beugung je Fingergelenk (Reihenfolge wie CurlBones). */
	TArray<FTransform> HandComponentPose(const TArray<float>& Curl) const;
	/** Berührt dieses Fingerglied (als Kapsel) die Nabelschnur? */
	bool PhalanxTouchesCord(const TArray<FTransform>& Component, int32 CurlIndex, float Radius) const;
	/** Ein Fingerglied als Segment in Weltkoordinaten. */
	void PhalanxSegment(const TArray<FTransform>& Component, int32 CurlIndex, FVector& OutStart, FVector& OutEnd) const;
	/** Kürzester Abstand eines Segments zur Mittellinie der Schnur (Welt). */
	float DistanceToCord(const FVector& A, const FVector& B, FVector& OutOnSegment, FVector& OutOnCord) const;
	float CordRadiusWorld() const;
	/** Die weiche Nabelschnur: Ruhelage lesen, dann je Bild mit den Gliedern der Hand als Hindernis lösen. */
	void InitCord();
	void SimulateCord(float DeltaSeconds, const TArray<FVector>& FingerA, const TArray<FVector>& FingerB, float FingerRadius);

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
	/** > 0: Die Außenansicht bleibt so lange draußen und fährt nicht in die Augen. */
	float ExteriorStaySeconds = 0.0f;

	// Der Spieler (Teil 2a)
	FGenesisWombInput Input;
	FGenesisWombInput DebugInput;
	FString DebugHold;
	float DebugHoldSeconds = 0.0f;
	float DebugHeld = 0.0f;
	FGenesisFetalView FetalNow;
	/** Lage des Körpers in der Höhle (Anteil vom Radius) und Drehung des Kopfes (Grad). */
	FVector2D BodyOffset = FVector2D::ZeroVector;
	FVector2D HeadLook = FVector2D::ZeroVector;
	/** Hand im Blickfeld (-1..1), zum Mund (0..1), geschlossen (0..1). */
	FVector2D HandPosition = FVector2D(0.3f, -0.9f);
	float HandToMouth = 0.0f;
	float HandClosed = 0.0f;
	/** Beugen der Finger: Knochen, Achse im Knochenraum, Winkel bei voller Faust (Grad), Ruhelage. */
	TArray<int32> CurlBones;
	TArray<FVector> CurlAxes;
	TArray<float> CurlDegrees;
	TArray<bool> CurlIsThumb;
	TArray<FTransform> HandRefPose;
	TArray<int32> CurlFinger;
	TArray<int32> CurlSegment;
	/** Beugung je Gelenk (0 gestreckt … 1 Faust) – jedes Gelenk bleibt stehen, sobald sein Glied die Schnur berührt. */
	TArray<float> JointCurl;
	/** Wie viele Finger anliegen (geglättet) – für das Halten mit Hysterese. */
	float HeldFingers = 0.0f;
	TArray<int32> HandParents;
	TArray<FTransform> HandRefComponent;
	int32 FingerTip[5] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	float FingerTipLength[5] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
	/** Die Hand als Körper im Wasser (Kamera-Raum, cm): Lage und Geschwindigkeit. */
	FVector HandPos = FVector::ZeroVector;
	FVector HandVelocity = FVector::ZeroVector;
	bool bHandPlaced = false;
	bool bCordBumped = false;
	FVector GraspAnchor = FVector::ZeroVector;
	/** Greifen: will das Kind greifen, wie weit hat es sich der Schnur zugewandt (0..1), wo liegt sie (Kamera-Raum, cm). */
	bool bGraspIntent = false;
	float ReachAlpha = 0.0f;
	FVector CordContact = FVector::ZeroVector;
	FVector CordSide = FVector::ForwardVector;
	/** Die Nabelschnur gibt nach, wenn das Kind an ihr zieht (Welt-Raum). */
	/** Nabelschnur-Simulation (Raum des Netzes): Teilchen, vorige Lage, Ruhelage, Abstände. */
	TArray<FVector> CordX;
	TArray<FVector> CordPrev;
	TArray<FVector> CordRest;
	TArray<float> CordRestLength;
	TArray<FTransform> CordRefLocal;
	TArray<FTransform> CordRefComponent;
	TArray<int32> CordParents;
	/** Radius der Schnur im Raum des Netzes (gebaut: 0,65 cm bei 10 cm Höhle). */
	float CordRadius = 0.65f;
	int32 GraspParticle = INDEX_NONE;
	FVector GraspOffset = FVector::ZeroVector;
	FVector GraspPointWorld = FVector::ZeroVector;
	/** Dellen unter den Fingern (Welt: Lage, Tiefe) – ans Material. */
	TArray<FVector4> CordDents;
	/** Sichtbare Dellen mit Gedächtnis (klingen über Sekunden ab). */
	TArray<FVector4> ShownDents;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FetusMaterial;
	float FetusScale = 1.0f;
	/** Lage der Augen in der Höhle (cm), so gelegt, dass der ganze Körper hineinpasst; neu bei Wechsel von Woche oder Lage. */
	FVector FittedEyes = FVector::ZeroVector;
	float FittedWeeks = -1.0f;
	float FittedHeadDown = -1.0f;
	float ExteriorAge = -1.0f;
	float ExteriorSpin = 0.0f;
	FVector NavelWorld = FVector::ZeroVector;
	bool bHasNavel = false;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> HandMaterials;
	/** Hand zum Mund: Fortschritt der Bewegung (0..1), daraus die gebremste Kurve HandToMouth. */
	float MouthProgress = 0.0f;
	/** Greifen: wie lange schon gehalten, wann die Hand von selbst loslässt, müde bis zum neuen Ansetzen. */
	float HoldSeconds = 0.0f;
	float HoldLimit = 5.0f;
	bool bGripTired = false;
	/** Eigenbewegung nach Verhaltenszustand (0 still … 1 aktiv). */
	float Activity = 0.5f;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CordMaterial;
	bool bHoldingCord = false;
	float CordPulsePhase = 0.0f;
	bool bThumbFelt = false;
	bool bCordFelt = false;
	/** Strecken und Gähnen laufen als kurze Bewegung ab (s seit Beginn, < 0 = nicht). */
	float StretchAge = -1.0f;
	float YawnAge = -1.0f;
	bool bLidsClosedByPlayer = false;
	TSet<EGenesisFetalAction> UsedActions;
	/** Ihre Hand auf dem Bauch: Countdown bis sie kommt, Alter, Richtung (in der Höhle). */
	float MotherTouchIn = -1.0f;
	float MotherTouchAge = -1.0f;
	FVector MotherTouchDirection = FVector(1.0, 0.0, 0.0);
	/** Wasserlassen: simulierte Minuten bis zum nächsten Mal. */
	float MinutesToUrinate = 25.0f;
	float HeartbeatPhase = 0.0f;
	FString Caption;
	float CaptionAge = 100.0f;
	TSet<FString> CaptionsShown;
};
