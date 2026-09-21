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

	/**
	 * Nahaufnahme: Sobald eine Zelle an der Zona hängt, geht die Kamera zwischen die Coronazellen an die Eintrittsstelle.
	 *
	 * Seit GENESIS-037 aus: Dort stand die Kamera eine Minute lang zwischen unscharfen Coronazellen, die
	 * bohrende Zelle darunter verborgen – gemessen bei 15, 40 und 65 s eines Durchlaufs, dreimal dasselbe
	 * Bild. Stattdessen zeigt die Kamera von außen, was wirklich geschieht: Dutzende Schwänze schlagen
	 * rund um den Cumulus, die Köpfe stecken darin, und sie fährt langsam an die führende Zelle heran.
	 */
	UPROPERTY(EditAnywhere, Category = "Oocyte")
	bool bCloseUpOnBinding = false;

	/** Heranfahrt an die führende Zelle, während gebohrt wird: Endabstand zur Eizellmitte (µm) und Dauer (s Echtzeit). */
	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "150"))
	float PushInDistanceUm = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "1"))
	float PushInSeconds = 22.0f;

	/**
	 * Optik in der ersten Woche. Mit der filmischen Optik (×18, f/11) blieben aus 260 µm nur ~18 µm scharf –
	 * der Keim ist 140 µm tief und war eine Wand aus unscharfen Zellen (gesehen bei 90 hpi). Ein Zeitraffer-
	 * Brutschrank zeigt den Keim scharf; mit ×3 bei f/22 sind es gut 150 µm.
	 */
	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "1", ClampMax = "40"))
	float EmbryoMacroScale = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "1.2", ClampMax = "32"))
	float EmbryoAperture = 22.0f;

	/** Abstand zur Mitte des Keims in der ersten Woche (µm): Zona und Keim füllen gut die Hälfte des Bildes. */
	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "120"))
	float EmbryoViewDistanceUm = 260.0f;

	/** Nach der Verschmelzung zurück auf die ganze Eizelle – die Cortikalreaktion läuft über ihre ganze Oberfläche (s). */
	UPROPERTY(EditAnywhere, Category = "Oocyte", meta = (ClampMin = "0.5"))
	float PullBackSeconds = 6.0f;

	/**
	 * Schwenk des Spielers (Grad, X = um die Hochachse, Y = auf und ab). Gesetzt von der Steuerung:
	 * Der Spieler führt das Mikroskop. Es bleibt stehen, wo er es hinschwenkt – ein Mikroskop federt nicht zurück.
	 */
	FVector2D PlayerOrbitDegrees = FVector2D::ZeroVector;

	/** Im Rennen: Kamera hinter der eigenen Zelle (Grad um sie herum, Höhe, Abstand in µm). */
	/**
	 * Schräg hinter und über der Zelle: Direkt dahinter (165°, 14°, 90 µm) zeigte die Geißel als
	 * unscharfer Fleck vor der Linse, und der Kopf ging im Dunkel unter. Von hier sind Kopf, Mittelstück
	 * und Geißel gleichzeitig zu sehen – und dahinter die Richtung, in die man lenkt.
	 */
	UPROPERTY(EditAnywhere, Category = "Race")
	float RaceAzimuthDegrees = 145.0f;

	UPROPERTY(EditAnywhere, Category = "Race")
	float RaceElevationDegrees = 28.0f;

	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "20"))
	float RaceDistanceUm = 75.0f;

	/**
	 * Optik beim Folgen im Rennen: mehr Schärfentiefe als beim Zuschauen. Mit Makro ×18 bei f/11 war die
	 * eigene Zelle aus 130 µm ein unscharfer Strich von 9 µm Länge – gemessen am Bild, Geißel unsichtbar.
	 * Wer lenkt, braucht Kopf, Geißel und Richtung gleichzeitig scharf; ein Mikroskopiker würde dafür
	 * ebenfalls abblenden.
	 */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "1.2", ClampMax = "32"))
	float RaceAperture = 22.0f;

	/**
	 * Schärfentiefe wächst umgekehrt mit dem Makro-Faktor. Bei ×10 und f/22 bleiben aus 75 µm nur etwa
	 * 3 µm scharf – die Zelle ist 60 µm lang. Mit ×1,2 sind es rund 20 µm: Kopf und Geißel scharf,
	 * Wand und Cumulus dahinter weich. (Ein echtes Mikroskop hat bei dieser Vergrößerung auch nur
	 * wenige Mikrometer – deshalb legt man Spermien zum Beobachten in eine flache Kammer.)
	 */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "1", ClampMax = "40"))
	float RaceMacroScale = 1.2f;

	/** Nachführung im Rennen (s): enger als beim Zuschauen, sonst läuft die eigene Zelle aus dem Bild. */
	UPROPERTY(EditAnywhere, Category = "Race", meta = (ClampMin = "0.05"))
	float RacePositionSmoothingSeconds = 0.35f;

	/** Blickpunkt hinter der Kopfspitze im Rennen (µm): die Mitte der Zelle, nicht der Kopf allein. */
	UPROPERTY(EditAnywhere, Category = "Race")
	float RaceLookBehindHeadUm = 22.0f;

	/**
	 * Halbe Ich-Perspektive (GENESIS-038, Wunsch des Game Directors): Die Kamera sitzt knapp hinter und über
	 * dem eigenen Kopf und blickt in Schwimmrichtung. Kopf und Mittelstück liegen unten im Bild, die Geißel
	 * schlägt unter der Linse nach hinten weg, voraus liegt der Weg. Umschalten auf die Verfolgeransicht mit V
	 * (Gamepad: rechter Stick drücken).
	 */
	UPROPERTY(EditAnywhere, Category = "Race")
	bool bRaceEgoView = true;

	/**
	 * Abstand hinter der Kopfspitze (µm). Bei 14 µm lag der Kopf unter dem Bildrand und die eigene Geißel direkt
	 * vor der Linse; bei 26 µm sitzt der Kopf im unteren Drittel, das Mittelstück läuft nach unten aus dem Bild.
	 */
	UPROPERTY(EditAnywhere, Category = "Race|Ego", meta = (ClampMin = "4"))
	float RaceEgoDistanceUm = 26.0f;

	/** Leicht seitlich versetzt (180° = genau dahinter): „über die Schulter", damit der Kopf den Weg nicht verdeckt. */
	UPROPERTY(EditAnywhere, Category = "Race|Ego")
	float RaceEgoAzimuthDegrees = 170.0f;

	/** Blick von oben auf die Schlagebene: nur so ist die Geißelwelle als Welle zu sehen. */
	UPROPERTY(EditAnywhere, Category = "Race|Ego")
	float RaceEgoElevationDegrees = 18.0f;

	/** Blickpunkt vor der Kopfspitze (µm) – dorthin, wohin gelenkt wird. */
	UPROPERTY(EditAnywhere, Category = "Race|Ego")
	float RaceEgoLookAheadUm = 30.0f;

	/** Schärfe liegt knapp vor dem Kopf: der eigene Kopf und der Weg voraus bleiben lesbar (µm ab Kopfspitze). */
	UPROPERTY(EditAnywhere, Category = "Race|Ego")
	float RaceEgoFocusAheadUm = 12.0f;

	/** Weitwinkliger als beim Zuschauen (Kleinbild-Brennweite, mm): Aus der Nähe braucht man Überblick. */
	UPROPERTY(EditAnywhere, Category = "Race|Ego", meta = (ClampMin = "10", ClampMax = "100"))
	float RaceEgoFocalLengthMm = 28.0f;

	/**
	 * Sensor und Brennweite gemeinsam verkleinert: gleicher Bildwinkel, mehr Schärfentiefe. Aus 16 µm
	 * Abstand wäre mit Kleinbild-Schärfentiefe nur ein Bruchteil eines Mikrometers scharf.
	 */
	UPROPERTY(EditAnywhere, Category = "Race|Ego", meta = (ClampMin = "0.1", ClampMax = "40"))
	float RaceEgoMacroScale = 0.4f;

	/** Abstand des Lichts hinter der Linse in der Ich-Perspektive (µm); es sitzt halb so hoch darüber. */
	UPROPERTY(EditAnywhere, Category = "Race|Ego", meta = (ClampMin = "0"))
	float RaceEgoLightBackUm = 36.0f;

	/** Entwickler: sofort die Einstellung auf den Keim wie nach der Verschmelzung (genesis.Conception.WatchEmbryo). */
	void DebugWatchEmbryo() { bWatchOocyte = true; bWatchFertilization = true; SecondsSinceFusion = 1000.0f; bInitialized = false; }

	void ToggleRaceView() { bRaceEgoView = !bRaceEgoView; bInitialized = false; }
	bool IsRaceEgoView() const { return bRaceEgoView; }
	/** Zeigt die Kamera gerade die Ich-Perspektive (im Rennen, hinter der eigenen Zelle)? */
	bool IsShowingEgoView() const;

	/** Ab diesem Abstand der eigenen Zelle zur Zona (µm) geht die Kamera auf die ganze Eizelle – am Rand des Cumulus. */
	UPROPERTY(EditAnywhere, Category = "Race")
	float RaceEggViewDistanceUm = 60.0f;

	/** Grenze für den Schwenk auf und ab (Grad) – darüber stünde die Kamera in der Schleimhaut. */
	UPROPERTY(EditAnywhere, Category = "Follow")
	float MaxPlayerPitchDegrees = 35.0f;

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
	/** Echtzeit seit die erste Zelle gebunden hat bzw. seit der Verschmelzung (negativ = noch nicht). */
	float SecondsSinceBinding = -1.0f;
	float SecondsSinceFusion = -1.0f;
	/** Richtung (von der Eizellmitte) zur führenden Zelle, geglättet – sie wechselt, wenn eine andere Zelle vorn liegt. */
	FVector SmoothedLeaderDirection = FVector::ZeroVector;
	bool bInitialized = false;
	bool bDebugPageRegistered = false;
	float CurrentFocusDistance = 0.0f;
};
