// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GenesisFetalTypes.generated.h"

/**
 * Die Fetalzeit nach Referenzdaten (GENESIS-044, Docs/34). Alle Wochenangaben hier sind **SSW** (Gestationsalter ab der
 * letzten Regel), wie in den Quellen; die Weltuhr rechnet ab der Befruchtung (SSW = Wochen nach Befruchtung + 2).
 */

/** Ein Punkt der Wachstumskurve (50. Perzentile). */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisFetalGrowthPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Fetal") float GestationalWeeks = 0.0f;
	/** Scheitel-Steiß-Länge (cm), 0 = nicht angegeben. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Fetal") float CrownRumpCm = 0.0f;
	/** Scheitel-Fersen-Länge (cm), 0 = nicht angegeben. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Fetal") float CrownHeelCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Fetal") float WeightGrams = 0.0f;
	/** Mittlere Herzfrequenz (/min), 0 = nicht angegeben. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genesis|Fetal") float HeartRateBpm = 0.0f;
};

/** Wann Bewegungen und Sinne einsetzen (SSW). */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisFetalMilestones
{
	GENERATED_BODY()

	// Bewegungen (de Vries, Visser & Prechtl 1982)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float FirstMovement = 7.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float Startle = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float GeneralMovement = 8.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float Hiccup = 9.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float IsolatedLimbs = 9.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float HeadRotation = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float HandFaceContact = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float BreathingMovements = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float Yawn = 11.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float SuckAndSwallow = 12.0f;
	/** Greifen (Greifreflex): Die Hand schließt sich um das, was sie berührt – beobachtet im 3. Trimenon, auch um die Nabelschnur (Jakobovits 2007). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float Grasp = 28.0f;
	/** Ab hier wird es eng: Das Kind füllt die Höhle zunehmend aus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float SpaceNarrows = 26.0f;
	/** Die Mutter spürt die Bewegungen zum ersten Mal (Erstgebärende). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement") float QuickeningFirstPregnancy = 20.0f;

	// Hören (Hepper & Shahidullah 1994): zuerst 500 Hz, dann tiefer, zuletzt höher
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float HearingOnset = 19.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float HearingLowFrequencies = 23.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float Hearing1000Hz = 33.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float Hearing3000Hz = 35.0f;
	/** Hörschwelle (dB) um SSW 28 und zum Termin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float HearingThresholdAt28Db = 40.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hearing") float HearingThresholdAtTermDb = 13.5f;

	// Sehen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sight") float EyesBeginToOpen = 26.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sight") float EyesOpen = 28.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sight") float PupilResponse = 31.0f;

	// Tasten, Schmecken
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touch") float TouchAroundMouth = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Touch") float TouchWholeBody = 14.0f;

	/** Thalamus-Fasern wachsen in die Hirnrinde (23–24): erst ab hier ist bewusstes Wahrnehmen strukturell möglich. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brain") float ConsciousAccessBegins = 23.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Brain") float ConsciousAccessEstablished = 26.0f;

	// Schlaf und Wachen (Nijhuis 1982)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States") float RestActivityCycles = 25.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States") float ClearSleepStates = 32.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States") float FourBehaviouralStates = 36.0f;
};

/** Die ganze Referenz: Wachstum, Meilensteine, Kindslage. */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisFetalReference
{
	GENERATED_BODY()

	FGenesisFetalReference();

	/** Nach SSW aufsteigend. Zwischen den Punkten wird linear gerechnet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Growth") TArray<FGenesisFetalGrowthPoint> Growth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Milestones") FGenesisFetalMilestones Milestones;

	/** Anteil in Beckenendlage (SSW → Anteil). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation") TArray<FVector2D> BreechShare;
};

/** Was die Wochen für Körper und Sinne bedeuten – aus der Referenz abgeleitet, für alle Systeme gleich. */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisFetalView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float GestationalWeeks = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float CrownRumpCm = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float CrownHeelCm = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float WeightGrams = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float HeartRateBpm = 0.0f;

	/** Hörbares Band (Hz). Vor dem Einsetzen des Hörens leer (Low = High = 0). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float HearingLowHz = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float HearingHighHz = 0.0f;
	/** Wie stark Klang ankommt (0..1): aus der Hörschwelle – 0 vor dem Einsetzen, 1 zum Termin. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float HearingSensitivity = 0.0f;

	/** Lider offen (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float EyesOpen = 0.0f;
	/** Licht wird als Helligkeit wahrgenommen (0..1): durch die Lider schwach, offen und mit Pupillenreaktion voll. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float LightPerception = 0.0f;
	/** Tasten (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float Touch = 0.0f;
	/** Bewusster Zugang zu den Sinnen (0..1): Thalamus–Rinde. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float ConsciousAccess = 0.0f;
	/** Die Mutter spürt die Bewegungen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") bool bMotherFeelsMovement = false;
	/** Anteil in Beckenendlage in dieser Woche. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Fetal") float BreechShare = 0.0f;
};

/** Was das Kind gerade tut. */
UENUM(BlueprintType)
enum class EGenesisFetalState : uint8
{
	/** Ruhe (vor SSW 32) bzw. ruhiger Schlaf (1F). */
	Quiet,
	/** Aktivität (vor SSW 32) bzw. aktiver Schlaf (2F). */
	Active,
	/** Ruhig wach (3F, erst ab SSW 36). */
	QuietAwake,
	/** Aktiv wach (4F, erst ab SSW 36). */
	ActiveAwake
};

UENUM(BlueprintType)
enum class EGenesisFetalEvent : uint8
{
	Startle,
	Kick,
	Stretch,
	Hiccup,
	Yawn,
	ThumbSuck,
	Swallow,
	HeadTurn,
	BreathingBout
};

/**
 * Was der Spieler als Kind tun kann (GENESIS-044 Teil 2a, Docs/37). Jede Handlung erst ab der Woche, in der der Körper
 * sie kann (GenesisFetalLogic::CanPerform).
 */
UENUM(BlueprintType)
enum class EGenesisFetalAction : uint8
{
	/** Sich im Ganzen bewegen, drehen. */
	MoveBody,
	/** Arm und Hand bewegen. */
	MoveHand,
	TurnHead,
	/** Strampeln, treten. */
	Kick,
	Stretch,
	Yawn,
	/** Hand zum Mund, am Daumen saugen. */
	HandToMouth,
	/** Fruchtwasser schlucken – und schmecken. */
	Swallow,
	/** Die Hand schließen, etwa um die Nabelschnur. */
	Grasp,
	/** Die Augen öffnen und schließen. */
	OpenEyes
};

/** Laufender Verhaltenszustand des Kindes (wird mitgespeichert). */
USTRUCT(BlueprintType)
struct GENESISBODY_API FGenesisFetalBehaviour
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Fetal") EGenesisFetalState State = EGenesisFetalState::Quiet;
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Fetal") float StateSecondsLeft = 0.0f;
	/** Schluckauf: Rest der Serie (s) und Sekunden bis zum nächsten Hicks. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Fetal") float HiccupSecondsLeft = 0.0f;
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Fetal") float NextHiccupIn = 0.0f;
	/** Sekunden in den Zuständen (für Anteile und Tests). */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Genesis|Fetal") TArray<float> SecondsInState;
};

/** Projekteinstellungen der Fetalzeit (Project Settings → Genesis → Fetal). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Fetal"))
class GENESISBODY_API UGenesisFetalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	UPROPERTY(Config, EditAnywhere, Category = "Reference")
	FGenesisFetalReference Reference;
};
