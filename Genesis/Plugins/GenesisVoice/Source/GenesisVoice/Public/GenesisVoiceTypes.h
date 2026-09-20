// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisGeneticsTypes.h"
#include "GenesisVoiceTypes.generated.h"

/**
 * Die Stimme eines Menschen.
 *
 * Eine Stimme ist kein Klangeffekt, sondern ein Körper: Die Grundfrequenz kommt von der Länge und
 * Spannung der Stimmlippen, die Klangfarbe von der Länge des Ansatzrohrs zwischen Kehlkopf und Lippen.
 * Beides wächst mit dem Menschen – deshalb klingt ein Kind nicht wie ein kleiner Erwachsener,
 * sondern anders: höher **und** heller.
 *
 * Was hier steht, ist messbar. Die Zahlen stammen aus der Phonetik, nicht aus dem Gefühl.
 */

/** Was jemand gerade von sich gibt. Worte sind nicht dabei – die kommen mit dem Dialogsystem. */
// ScriptName: In Python hieße der Aufzählungstyp sonst genauso wie die Struktur FGenesisUtterance
UENUM(BlueprintType, meta = (ScriptName = "GenesisUtteranceType"))
enum class EGenesisUtterance : uint8
{
	/** Der Schrei des Neugeborenen: laut, hoch, rau, mit hörbarem Einatmen dazwischen. */
	Cry,
	/** Quengeln – der Vorlauf zum Schrei. */
	Fuss,
	/** Gurren. Ab etwa zwei Monaten, wenn das Kind zufrieden ist. */
	Coo,
	/** Lallen: Silbenketten wie „ba-ba-ba". Ab etwa sechs Monaten. */
	Babble,
	/** Lachen. Ab etwa vier Monaten. */
	Laugh,
	/** Seufzen. */
	Sigh,
	/** Summen – die Melodie ohne Worte, mit der Mütter beruhigen. */
	Hum,
	/** „Schhh" – Beruhigen. Fast nur Rauschen, sehr wenig Stimme. */
	Soothe,
	/** Sprechen ohne Worte: Silben mit Sprechmelodie. Platzhalter, bis es Sprache gibt. */
	Speak,
	/** Rufen über Entfernung: lauter, höher, längere Silben. */
	Call
};

/** Vokale als Formantziele. Mehr braucht es für nicht-sprachliche Laute nicht. */
UENUM(BlueprintType)
enum class EGenesisVowel : uint8
{
	A,
	E,
	I,
	O,
	U,
	Schwa
};

/**
 * Die Stimme einer Person zu einem Zeitpunkt ihres Lebens.
 * Wird aus Körper und Genom berechnet – nichts davon wird von Hand eingestellt.
 */
USTRUCT(BlueprintType)
struct GENESISVOICE_API FGenesisVoiceProfile
{
	GENERATED_BODY()

	/**
	 * Mittlere Sprechgrundfrequenz (Hz).
	 * Neugeborenes 450, Kind 270, erwachsene Frau 200, erwachsener Mann 115.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float F0Hz = 200.0f;

	/** Melodieumfang beim Sprechen (Halbtöne). Kinder sprechen melodischer als Erwachsene. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float F0RangeSemitones = 4.0f;

	/**
	 * Skalierung der Formanten gegenüber einem erwachsenen Mann (Ansatzrohr 17,5 cm).
	 * Neugeborenes etwa 2,5 (7 cm), erwachsene Frau etwa 1,2 (14,5 cm).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float FormantScale = 1.0f;

	/** 0..1 – Behauchtheit. Luft, die ungenutzt durch die Stimmritze strömt. Steigt im Alter und bei Erschöpfung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float Breathiness = 0.15f;

	/** 0..1 – Rauigkeit: unregelmäßige Schwingung (Jitter und Shimmer). Steigt bei Heiserkeit und im Alter. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float Roughness = 0.05f;

	/** 0..1 – Näseln. Steigt bei Schnupfen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float Nasality = 0.1f;

	/** 0..1 – Kraft der Stimme. Ein erschöpfter oder kranker Mensch spricht leiser. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float Strength = 0.8f;

	/** Sprechtempo als Faktor (1 = ruhig erwachsen). Kinder und Aufgeregte sprechen schneller. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float TempoScale = 1.0f;

	/** Biologisches Geschlecht – für die Anzeige; der Stimmbruch steckt schon in F0 und Formanten. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") EGenesisBiologicalSex Sex = EGenesisBiologicalSex::Female;

	/** Alter, zu dem dieses Profil gehört (Jahre). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Voice") float AgeYears = 0.0f;
};

/** Was aus dem Körper in die Stimme geht. Alles davon kommt aus der Simulation. */
USTRUCT(BlueprintType)
struct GENESISVOICE_API FGenesisVoiceInputs
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") EGenesisBiologicalSex Sex = EGenesisBiologicalSex::Female;

	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float AgeYears = 30.0f;

	/** Körpergröße (cm). Wirkt auf die Klangfarbe erst, wenn der Mensch ausgewachsen ist. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float HeightCm = 172.0f;

	/** 0..1 – Gesundheit von Kehlkopf und Lunge. Wenig davon heißt: leise und behaucht. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float RespiratoryHealth = 1.0f;

	/** 0..1 – akute Erkrankung (Erkältung, Heiserkeit). Macht rau und näselnd. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float Illness = 0.0f;

	/** 0..1 – Erschöpfung (Schlafmangel, Anstrengung). */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float Exhaustion = 0.0f;

	/** 0..1 – Erregung. Wer aufgeregt ist, spricht höher, schneller und melodischer. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float Arousal = 0.3f;

	/** Individuelle Streuung – aus dem Genom. Zwei gleich alte Menschen klingen nicht gleich. */
	UPROPERTY() uint64 IndividualSeed = 0;
};

/** Ein einzelner Laut, den jemand gerade macht. */
USTRUCT(BlueprintType)
struct GENESISVOICE_API FGenesisUtterance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") EGenesisUtterance Type = EGenesisUtterance::Speak;

	/** 0..1 – wie dringend, laut und hoch. Ein Schmerzschrei ist nicht dasselbe wie Quengeln. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float Intensity = 0.5f;

	/** Dauer in Sekunden. 0 = die natürliche Dauer des Lauts. */
	UPROPERTY(BlueprintReadWrite, Category = "Genesis|Voice") float DurationSeconds = 0.0f;

	/** Streuung, damit zweimal derselbe Laut nicht identisch klingt. */
	UPROPERTY() uint64 Seed = 0;
};

/** Stellschrauben der Stimmentwicklung. Voreinstellungen sind gemessene Mittelwerte aus der Phonetik. */
USTRUCT(BlueprintType)
struct GENESISVOICE_API FGenesisVoiceTuning
{
	GENERATED_BODY()

	/** Grundfrequenz des Schreis eines Neugeborenen (Hz). Gemessen werden 400–600. */
	UPROPERTY(EditAnywhere, Category = "Pitch") float NewbornF0Hz = 450.0f;

	/** Grundfrequenz im Kindergartenalter (Hz) – bei beiden Geschlechtern praktisch gleich. */
	UPROPERTY(EditAnywhere, Category = "Pitch") float ChildF0Hz = 270.0f;

	/** Grundfrequenz einer erwachsenen Frau (Hz). */
	UPROPERTY(EditAnywhere, Category = "Pitch") float AdultFemaleF0Hz = 200.0f;

	/** Grundfrequenz eines erwachsenen Mannes (Hz). */
	UPROPERTY(EditAnywhere, Category = "Pitch") float AdultMaleF0Hz = 115.0f;

	/** Beginn und Ende des Stimmwechsels (Jahre). Bei Jungen fällt die Stimme dabei um etwa eine Oktave. */
	UPROPERTY(EditAnywhere, Category = "Pitch") float VoiceChangeStartYears = 11.5f;
	UPROPERTY(EditAnywhere, Category = "Pitch") float VoiceChangeEndYears = 15.5f;

	/** Ab hier verändert sich die Stimme wieder (Presbyphonie): Männer steigen, Frauen fallen. */
	UPROPERTY(EditAnywhere, Category = "Pitch") float AgingVoiceStartYears = 60.0f;

	/** Verschiebung der Grundfrequenz im hohen Alter (Hz, ab 80 Jahren voll wirksam). */
	UPROPERTY(EditAnywhere, Category = "Pitch") float OldMaleF0RiseHz = 20.0f;
	UPROPERTY(EditAnywhere, Category = "Pitch") float OldFemaleF0DropHz = 25.0f;

	/** Länge des Ansatzrohrs (cm): Neugeborenes, erwachsene Frau, erwachsener Mann. */
	UPROPERTY(EditAnywhere, Category = "Timbre") float NewbornTractCm = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Timbre") float AdultFemaleTractCm = 14.5f;
	UPROPERTY(EditAnywhere, Category = "Timbre") float AdultMaleTractCm = 17.5f;

	/** Behauchtheit und Rauigkeit, die das Alter allein bis 85 Jahre hinzufügt. */
	UPROPERTY(EditAnywhere, Category = "Timbre") float AgeBreathiness = 0.35f;
	UPROPERTY(EditAnywhere, Category = "Timbre") float AgeRoughness = 0.30f;
};
