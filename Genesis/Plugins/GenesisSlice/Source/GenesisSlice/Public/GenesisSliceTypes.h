// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisMotherDay.h"
#include "GenesisSliceTypes.generated.h"

/**
 * Der Vertical Slice als ein Durchlauf.
 *
 * Bis hierher gab es die Teile: die Befruchtung, die erste Woche, die Geburt, die erste Stunde.
 * Jeder Teil lief für sich, gestartet über Konsolenbefehle. Was fehlte, war das, worum es geht –
 * **ein Leben am Stück**: von der Verschmelzung zweier Zellen bis zu dem Moment, in dem ein Mensch
 * satt und warm einschläft.
 *
 * Die Regie erfindet dabei nichts. Sie schaut zu, was die Systeme melden, und entscheidet nur,
 * wann gewartet, wann gesprungen und wann der Ort gewechselt wird.
 */
UENUM(BlueprintType)
enum class EGenesisSlicePhase : uint8
{
	/** Kein Durchlauf. */
	Idle,
	/** Der Schwarm im Eileiter: Die Befruchtung steht noch aus. */
	Conception,
	/** Die erste Woche: Furchung, Morula, Blastozyste, Schlüpfen, Einnistung. */
	Embryo,
	/** Schwangerschaft: neun Monate, die im Zeitraffer vergehen. */
	Gestation,
	/** Die Geburt. */
	Birth,
	/** Die erste Stunde. */
	FirstHour,
	/** Angekommen: Das Kind ist warm, satt und schläft. */
	Complete,
	/** Abgebrochen – meistens, weil das Leben nicht zustande kam. */
	Ended
};

/** Warum ein Durchlauf geendet hat. */
UENUM(BlueprintType)
enum class EGenesisSliceEnding : uint8
{
	/** Läuft noch. */
	None,
	/** Das Kind schläft: der vorgesehene Schluss. */
	Asleep,
	/** Die erste Stunde ist vorbei, das Kind aber nicht zur Ruhe gekommen. Auch das ist ein Ausgang. */
	Unsettled,
	/** Der Keim hat sich nicht weiterentwickelt. Das ist kein Fehler, das ist Biologie. */
	EmbryoArrested,
	/** Das Kind hat die Geburt nicht überlebt. */
	NotAlive,
	/** Von Hand abgebrochen. */
	Aborted
};

/** Was die Systeme gerade melden. Daraus – und nur daraus – entscheidet die Regie. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceSignals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bConceived = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bImplanted = false;

	/**
	 * Ende der vierten Woche: Der Embryo hat seinen Bauplan (Neuralrohr zu, Herz schlägt, 30 Somiten).
	 * Erst dann übernimmt die Körpersimulation die Schwangerschaft (GENESIS-041).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bBodyPlanDone = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bEmbryoArrested = false;

	/** Schwangerschaftswoche des Kindes. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float GestationalWeeks = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bLaborRunning = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bBorn = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bAlive = true;

	/** Minuten seit der Geburt. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float MinutesSinceBirth = 0.0f;

	/** true, sobald das Kind in der ersten Stunde eingeschlafen ist. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") bool bAsleep = false;
};

/** Zustand eines Durchlaufs. Persistiert – ein Slice überlebt das Speichern. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") EGenesisSlicePhase Phase = EGenesisSlicePhase::Idle;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") EGenesisSliceEnding Ending = EGenesisSliceEnding::None;

	UPROPERTY() FGuid EntityId;
	UPROPERTY() uint64 RunSeed = 0;

	/** Zeitpunkt, an dem der Durchlauf begonnen hat. */
	UPROPERTY() FGenesisTimestamp StartTime;

	/** Wie lange der Durchlauf in echter Zeit schon läuft (s) – für Anzeige und Messung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float RealSeconds = 0.0f;

	/** Echtzeit in der aktuellen Phase (s). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Slice") float PhaseRealSeconds = 0.0f;

	bool IsRunning() const { return Phase != EGenesisSlicePhase::Idle && Phase != EGenesisSlicePhase::Complete && Phase != EGenesisSlicePhase::Ended; }
};

/** Was die Mutter in einem Moment tut – der Ablauf sucht dafür die Uhrzeit in ihrem wirklichen Tag. */
UENUM(BlueprintType)
enum class EGenesisGestationSituation : uint8
{
	/** Genau zur angegebenen Uhrzeit, was immer sie dann tut. */
	AtHour,
	/** Wenn sie draußen spazieren geht (Licht durch den Bauch). */
	Walk,
	/** Wenn sie abends mit dem Bauch spricht. */
	BellyTalk
};

/**
 * Ein Moment der Schwangerschaft (GENESIS-044 Teil 1b, Docs/34): eine Woche, eine Uhrzeit, eine Dauer. Zwischen den
 * Momenten läuft die Zeit im Zeitraffer; im Moment selbst fast in Echtzeit – subjektive Zeit (Masterprompt, Punkt 10).
 * Die Uhrzeit zählt, weil der Tag der Mutter sie bestimmt: abends spricht sie, nachts ist es still und dunkel.
 */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisGestationMoment
{
	GENERATED_BODY()

	/** SSW (ab der letzten Regel, wie in den Quellen). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") float GestationalWeeks = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") float HourOfDay = 12.0f;
	/**
	 * Statt einer festen Uhrzeit eine Situation in ihrem Tag: Der Ablauf sucht in dieser Woche den Tag und die Stunde,
	 * zu der sie wirklich spazieren geht oder mit dem Bauch spricht. Die Welt wartet nicht auf den Spieler – der Moment
	 * findet die Wirklichkeit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") EGenesisGestationSituation Situation = EGenesisGestationSituation::AtHour;
	/** Echtzeit im Moment (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") float Seconds = 30.0f;
	/** Kapitelzeile, sparsam eingeblendet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") FString Title;
	/** Darunter: was sich in dieser Woche für das Kind ändert (Docs/34). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") FString Subtitle;
	/**
	 * Nur von außen (Docs/37, Teil 2c): Vor SSW 19 nimmt das Kind weder Ton noch Licht bewusst wahr – die Kamera bleibt
	 * draußen und zeigt, was es in dieser Woche schon tut, statt in seine Augen zu fahren.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Slice") bool bOnlyFromOutside = false;
};

/** Wo der Ablauf der Schwangerschaft nach einer Echtzeit steht. */
struct GENESISSLICE_API FGenesisGestationPlanPoint
{
	/** Zielzeit: Stunden seit der Befruchtung. */
	double HoursAfterConception = 0.0;
	/** Index des laufenden Moments, -1 im Zeitraffer dazwischen. */
	int32 MomentIndex = -1;
	/** Fortschritt im Moment bzw. im Zeitraffer (0..1). */
	float Alpha = 0.0f;
	/** Der Ablauf ist durch: Die Zielzeit steht auf der Geburt. */
	bool bFinished = false;
};

/** Stellschrauben der Regie. */
USTRUCT(BlueprintType)
struct GENESISSLICE_API FGenesisSliceTuning
{
	GENERATED_BODY()

	FGenesisSliceTuning();

	/**
	 * Ab dieser Woche nach der Befruchtung beginnt die Geburt: 38 = SSW 40, der errechnete Termin. (Bis GENESIS-044
	 * stand hier 39 – das war SSW 41, eine Woche über dem Termin.)
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float BirthAtWeeks = 38.0f;

	/**
	 * Zeitraffer der ersten Woche wie im EmbryoScope (Stunden Keimzeit je Sekunde Echtzeit, GENESIS-038):
	 * vor der ersten Teilung zügig, während der Teilungen ruhig genug, dass man jede sieht, die Einnistung
	 * wieder schneller. Vorher liefen zehn Tage in zehn Sekunden, in Sprüngen von sechs Stunden.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0.1")) float ZygoteHoursPerSecond = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0.1")) float CleavageHoursPerSecond = 1.6f;
	/** Die zweite Woche: sieben Stufen, je ein Satz dazu – 5 h je Sekunde lassen jeden lesen (gut eine halbe Minute). */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0.1")) float ImplantationHoursPerSecond = 5.0f;

	/** Dritte und vierte Woche (Keimblätter bis Herzschlag): 12 h je Sekunde, gut eine halbe Minute für 15 Tage. */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0.1")) float BodyPlanHoursPerSecond = 12.0f;

	/**
	 * Simulationssekunden je Sekunde im Eileiter.
	 *
	 * **Zeitlupe ist hier keine Stilfrage, sondern Physik.** Eine Spermienzelle schlägt mit 16–24 Hz
	 * und rollt mit 4–8 Hz. Bei 60 Bildern je Sekunde bleiben davon in Echtzeit drei Bilder je
	 * Schlagzyklus – die Zelle zuckt, statt zu schwimmen (genau das war zu sehen, als diese Szene
	 * versuchsweise in Echtzeit lief). Jede echte Aufnahme von Spermien ist deshalb eine
	 * Hochgeschwindigkeitsaufnahme, verlangsamt abgespielt. Bei 0,3 liegt der sichtbare Schlag bei
	 * 5–7 Hz, also bei zehn Bildern je Zyklus.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float ConceptionTimeScale = 0.3f;

	/** Wie lange der letzte Moment eines Lebens stehen bleibt, bevor der Abspann beginnt (s). */
	UPROPERTY(EditAnywhere, Category = "Slice") float EndingHoldSeconds = 6.0f;

	/** Die Befruchtung als Wettrennen: Der Spieler führt eine Zelle (GENESIS-037). */
	UPROPERTY(EditAnywhere, Category = "Slice") bool bConceptionRace = true;

	/** Nach einer Niederlage: so lange bleibt das Bild stehen, bevor das Rennen neu beginnt (s). */
	UPROPERTY(EditAnywhere, Category = "Slice") float RaceLostHoldSeconds = 7.0f;

	/** Simulationsminuten je Sekunde in der Geburt – für die Stunden der Eröffnungsphase. */
	UPROPERTY(EditAnywhere, Category = "Slice") float BirthTimeScale = 90.0f;

	/**
	 * Übergangs- und Austreibungsphase laufen langsamer: Mit 90 Minuten je Sekunde war die Austreibung,
	 * der Höhepunkt der Geburt, in weniger als einer Sekunde vorbei – keine Wehe zu spüren, kein Satz der
	 * Hebamme passte hinein. Einheit: Simulationsminuten je Sekunde. Eine Presswehe kommt alle 2–3 Minuten:
	 * Mit 0,125 (7,5 s je Sekunde) folgt sie alle 20 s und dauert 8–10 s, die Austreibung (~18 min) dauert
	 * gut zwei Minuten – der Rhythmus, in dem im Kreißsaal gesprochen wird. Die Übergangsphase (~30 min)
	 * mit 0,7: gut 45 s, drei bis vier Wehen (GENESIS-038, Ton; gemessen im Spiel).
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float TransitionTimeScale = 0.7f;
	UPROPERTY(EditAnywhere, Category = "Slice") float PushingTimeScale = 0.125f;

	/** Simulationsminuten je Sekunde in der ersten Stunde. */
	UPROPERTY(EditAnywhere, Category = "Slice") float FirstHourTimeScale = 2.0f;

	/**
	 * Nach dieser Zeit legt die Hebamme das Kind auf die Haut der Mutter und spricht mit ihm (min).
	 * Das ist keine Entscheidung des Spielers, sondern das, was in einem Kreißsaal ohnehin geschieht –
	 * solange es keine Eingabe gibt, handelt die Welt. Mit `genesis.Newborn.SkinToSkin 0` lässt sich
	 * das Gegenteil erzwingen, und dann kühlt das Kind aus.
	 * Leitlinie (AWMF, WHO): Ein gesundes Kind kommt sofort auf die Mutter und wird dort abgetrocknet – eine halbe
	 * Minute in den Händen der Hebamme, nicht zwei (so lange nass in der Luft kühlt es spürbar aus).
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float SkinContactAfterMinutes = 0.5f;

	/**
	 * Um wie viele Minuten jede gerufene Minute die Versorgung vorzieht.
	 * Ein Kind, das schreit, wird schneller geholt – das ist der einzige Hebel, den es hat.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float CallShortensCare = 4.0f;

	/** Spätestens nach dieser Zeit endet die erste Stunde, auch wenn das Kind nicht schläft (min). */
	UPROPERTY(EditAnywhere, Category = "Slice") float FirstHourLimitMinutes = 75.0f;

	/**
	 * Weltsekunden je Echtzeitsekunde in den Szenen, die man wirklich erlebt (Geburt, erste Stunde).
	 * Dort soll nur die Phase selbst beschleunigen – sonst laufen zwei Zeitraffer übereinander.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") float SceneClockTimeScale = 1.0f;

	/** Weltsekunden je Echtzeitsekunde außerhalb dieser Szenen. */
	UPROPERTY(EditAnywhere, Category = "Slice") float DefaultClockTimeScale = 60.0f;

	/** Die Karten des Durchlaufs. */
	UPROPERTY(EditAnywhere, Category = "Slice") FName ConceptionMap = TEXT("L_GEN_OviductAmpulla");
	UPROPERTY(EditAnywhere, Category = "Slice") FName BirthMap = TEXT("L_GEN_Birth");
	/** Die zweite Woche (GENESIS-040): Nach dem Schlüpfen zeigt das Spiel die Schleimhaut der Gebärmutter. */
	UPROPERTY(EditAnywhere, Category = "Slice") FName ImplantationMap = TEXT("L_GEN_UterineCavity");

	/**
	 * Die Fruchthöhle (GENESIS-041 Teil 5): der Embryo selbst im Amnion, daneben der Dottersack. Erst ab diesem Tag
	 * nach der Befruchtung – der Körper ist für das Ende der vierten Woche gebaut (Carnegie 12–13); die Tage davor
	 * sehen anders aus und bekommen eigene Formen.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice") FName EmbryoMap = TEXT("L_GEN_Fruchthoehle");
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "15", ClampMax = "28")) float EmbryoSceneFromDay = 26.0f;

	/**
	 * Steht der Bauplan, hält der Zeitraffer an: So lange (s) schlägt das Herz in Echtzeit, bevor die Schwangerschaft
	 * weiterläuft. Der eine Moment, in dem die Zeit stillsteht.
	 */
	UPROPERTY(EditAnywhere, Category = "Slice", meta = (ClampMin = "0", ClampMax = "60")) float BodyPlanHoldSeconds = 8.0f;

	/** Die Schwangerschaft in Momenten (Standard in GenesisSliceLogic::DefaultGestationMoments, Docs/34). */
	UPROPERTY(EditAnywhere, Category = "Gestation") TArray<FGenesisGestationMoment> GestationMoments;
	/** Echtzeit des Zeitraffers zwischen zwei Momenten (s). */
	UPROPERTY(EditAnywhere, Category = "Gestation", meta = (ClampMin = "1", ClampMax = "60")) float GestationTravelSeconds = 16.0f;
	/** Der Mutterleib aus Sicht des Kindes (GENESIS-044 Teil 1b). */
	UPROPERTY(EditAnywhere, Category = "Gestation") FName GestationMap = TEXT("L_GEN_Mutterleib");
	/** Weltsekunden je Echtzeitsekunde im Moment: fast Echtzeit, ein Abend in einer halben Minute wäre zu schnell. */
	UPROPERTY(EditAnywhere, Category = "Gestation", meta = (ClampMin = "1", ClampMax = "600")) float GestationMomentTimeScale = 8.0f;
};
