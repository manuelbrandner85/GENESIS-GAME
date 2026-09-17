// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisKarma.generated.h"

/** Die sechs Karma-Dimensionen. Positive Werte = erster Pol, negative = Gegenpol. */
UENUM(BlueprintType)
enum class EGenesisKarmaDimension : uint8
{
	/** Mitgefühl ↔ Grausamkeit */
	Compassion,
	/** Ehrlichkeit ↔ Täuschung */
	Honesty,
	/** Mut ↔ Feigheit */
	Courage,
	/** Großzügigkeit ↔ Gier */
	Generosity,
	/** Weisheit ↔ Ignoranz */
	Wisdom,
	/** Liebe ↔ Gleichgültigkeit */
	Love
};

static constexpr int32 GenesisKarmaDimensionCount = 6;

/**
 * Sechsdimensionaler Karma-Vektor – als Impuls einer Handlung (Authoring) oder als Profilwert (−100 … +100).
 *
 * WICHTIG: Karma ist während des Lebens verborgen. Werte sind bewusst nicht BlueprintReadable und
 * dürfen nie in der Spiel-UI erscheinen. Einzige Anzeige: Developer HUD (nicht in Shipping).
 */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisKarmaVector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Compassion = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Honesty = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Courage = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Generosity = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Wisdom = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Karma")
	float Love = 0.0f;

	float Get(EGenesisKarmaDimension Dimension) const;
	void Set(EGenesisKarmaDimension Dimension, float Value);

	FGenesisKarmaVector operator*(float Scale) const;
	FGenesisKarmaVector& operator+=(const FGenesisKarmaVector& Other);
	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const;
};

/** Regeln der Charakterformung. */
USTRUCT(BlueprintType)
struct GENESISLIFESIMULATION_API FGenesisKarmaTuning
{
	GENERATED_BODY()

	/** Sättigung nahe den Extremen (0 = linear). Extreme entstehen nur durch viele Handlungen. */
	UPROPERTY(EditAnywhere, Category = "Karma", meta = (ClampMin = "0"))
	float SaturationExponent = 1.5f;

	/** Wie schnell sich Gewohnheiten pro Handlung ausbilden (0..1). */
	UPROPERTY(EditAnywhere, Category = "Karma", meta = (ClampMin = "0", ClampMax = "1"))
	float HabitSmoothing = 0.15f;

	/** Verstärkung, wenn eine Handlung der bestehenden Gewohnheit entspricht. */
	UPROPERTY(EditAnywhere, Category = "Karma", meta = (ClampMin = "0"))
	float HabitAmplification = 0.25f;
};

/** Verborgenes Karma-Profil einer Person. */
USTRUCT()
struct GENESISLIFESIMULATION_API FGenesisKarmaProfile
{
	GENERATED_BODY()

	/** −100 … +100 je Dimension. */
	UPROPERTY()
	FGenesisKarmaVector Values;

	/** −1 … +1 – Richtung der jüngsten Handlungen (Gewohnheit). */
	UPROPERTY()
	FGenesisKarmaVector Habit;

	UPROPERTY()
	int32 IntegratedActions = 0;

	/** Integriert den Karma-Impuls einer Handlung mit Sättigung und Gewohnheitsbildung. */
	void ApplyImpulse(const FGenesisKarmaVector& Impulse, const FGenesisKarmaTuning& Tuning);
};
