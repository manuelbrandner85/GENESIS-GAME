// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisWorldSoundTypes.generated.h"

/**
 * Der Klang eines Ortes.
 *
 * Bisher klang in GENESIS nur der Körper: Herzschlag, Blutstrom, Atem. Die Welt um ihn herum war still.
 * Das ist der Unterschied zwischen einem Messgerät und einem Ort – ein Kreißsaal ist nicht still,
 * er summt, klappert, raschelt, und irgendwo im Gang spricht jemand.
 *
 * Auch hier wird nichts abgespielt: Ein Grundton aus gefiltertem Rauschen, dazu einzelne Ereignisse,
 * deren Zeitpunkte aus einem deterministischen Strom kommen. Zweimal derselbe Seed heißt zweimal
 * derselbe Raum.
 */
UENUM(BlueprintType)
enum class EGenesisPlace : uint8
{
	/** Stille. */
	None,
	/** Die Eileiter-Ampulle: Flüssigkeit, Flimmerhärchen, ein ferner Puls durch das Gewebe. */
	OviductAmpulla,
	/** Der Mutterleib von innen: Darmgeräusche, das Rauschen des Mutterkuchens, die gedämpfte Welt. */
	Womb,
	/** Der Kreißsaal: Lüftung, Monitor, Instrumente, Tücher, Stimmen im Gang. */
	DeliveryRoom
};

/** Was ein Ort gerade klingen lässt. Die Werte kommen aus der Simulation. */
USTRUCT(BlueprintType)
struct GENESISWORLDSOUND_API FGenesisWorldSoundParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|WorldSound")
	EGenesisPlace Place = EGenesisPlace::Womb;

	/** Gesamtlautstärke des Ortes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|WorldSound", meta = (ClampMin = "0", ClampMax = "2"))
	float Loudness = 0.8f;

	/**
	 * Herzschlag der Mutter (Schläge/min). Er taktet im Mutterleib das Rauschen des Mutterkuchens
	 * und im Kreißsaal den Monitor – dasselbe Herz, zweimal gehört.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|WorldSound")
	float MaternalHeartRateBpm = 78.0f;

	/** 0..1 – wie viel im Raum geschieht. Unter der Austreibung ist mehr los als in der Eröffnungsphase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|WorldSound", meta = (ClampMin = "0", ClampMax = "1"))
	float Activity = 0.4f;

	/** 0..1 – wie stark die Verdauung der Mutter gerade arbeitet (Darmgeräusche im Mutterleib). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|WorldSound", meta = (ClampMin = "0", ClampMax = "1"))
	float Digestion = 0.5f;
};

/** Ereignisarten eines Ortes – für Anzeige, Tests und Messung. */
UENUM(BlueprintType)
enum class EGenesisWorldEvent : uint8
{
	/** Darmgeräusch: gluckernde Flüssigkeit hinter der Gebärmutterwand. */
	Gurgle,
	/** Das Rauschen des Mutterkuchens, im Takt des mütterlichen Herzens. */
	PlacentalWhoosh,
	/** Eine Blase oder ein Tropfen in der Flüssigkeit. */
	Drop,
	/** Metall auf Metall: Instrumente auf einer Ablage. */
	Clink,
	/** Tücher, Papier, Handschuhe. */
	Cloth,
	/** Der Monitor, im Takt des mütterlichen Herzens. */
	Monitor,
	/** Stimmen im Gang – zu weit weg für Worte. */
	DistantVoices,
	/** Ein Schritt. */
	Footstep
};
