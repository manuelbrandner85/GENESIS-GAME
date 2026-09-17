// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.generated.h"

/**
 * Stabile 64-Bit-Hashfunktionen.
 * Anders als GetTypeHash() sind diese Werte plattform- und versionsunabhängig und dürfen
 * deshalb in Spielständen, Seeds und deterministischen Ableitungen verwendet werden.
 */
namespace GenesisHash
{
	/** SplitMix64-Finalizer: verteilt Bits gleichmäßig. */
	inline uint64 Mix64(uint64 Value)
	{
		Value ^= Value >> 30;
		Value *= 0xBF58476D1CE4E5B9ull;
		Value ^= Value >> 27;
		Value *= 0x94D049BB133111EBull;
		Value ^= Value >> 31;
		return Value;
	}

	inline uint64 Combine(uint64 A, uint64 B)
	{
		return Mix64(A ^ (Mix64(B) + 0x9E3779B97F4A7C15ull + (A << 6) + (A >> 2)));
	}

	GENESISCORE_API uint64 FromGuid(const FGuid& Guid);

	/** FNV-1a über die Zeichen des Strings, anschließend gemischt. Groß-/Kleinschreibung wird ignoriert. */
	GENESISCORE_API uint64 FromString(FStringView Text);

	GENESISCORE_API uint64 FromName(FName Name);
}

/**
 * Deterministischer Zufallszahlengenerator (xoshiro256**).
 *
 * Warum nicht FRandomStream? GENESIS muss über Jahrzehnte und Wiedergeburten reproduzierbar bleiben:
 * - 64-Bit-Zustand mit sehr langer Periode
 * - vollständig serialisierbar (Spielstand setzt exakt an derselben Stelle fort)
 * - abgeleitete Teilströme (Derive) für Systeme und Personen, unabhängig von der Aufrufreihenfolge
 */
USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisRandomStream
{
	GENERATED_BODY()

	FGenesisRandomStream() { Reset(0); }
	explicit FGenesisRandomStream(uint64 InSeed) { Reset(InSeed); }

	/** Setzt den Strom vollständig auf einen Seed zurück. */
	void Reset(uint64 InSeed);

	uint64 GetSeed() const { return Seed; }

	uint64 NextUInt64();
	uint32 NextUInt32() { return static_cast<uint32>(NextUInt64() >> 32); }

	/** Gleichverteilt in [0, 1). */
	double NextDouble();

	/** Gleichverteilt in [0, 1). */
	float NextFloat() { return static_cast<float>(NextDouble()); }

	/** Gleichverteilte Ganzzahl in [Min, Max] (beide inklusive, ohne Modulo-Verzerrung). */
	int32 RandRange(int32 Min, int32 Max);

	/** Gleichverteilt in [Min, Max). */
	float FRandRange(float Min, float Max) { return Min + (Max - Min) * NextFloat(); }

	/** true mit Wahrscheinlichkeit Probability (geklemmt auf [0, 1]). */
	bool Bernoulli(float Probability);

	/** Normalverteilte Zahl (Box-Muller). */
	float Gaussian(float Mean, float StandardDeviation);

	/** Deterministische, gültige GUID aus dem Strom. */
	FGuid NewGuid();

	/**
	 * Leitet einen unabhängigen Teilstrom ab.
	 * Basis ist der ursprüngliche Seed (nicht der aktuelle Zustand): Derive(Salt) liefert immer denselben Strom,
	 * egal wie viele Zahlen dieser Strom bereits erzeugt hat.
	 */
	FGenesisRandomStream Derive(uint64 Salt) const;

private:
	UPROPERTY(SaveGame)
	uint64 Seed = 0;

	UPROPERTY(SaveGame)
	uint64 State0 = 0;

	UPROPERTY(SaveGame)
	uint64 State1 = 0;

	UPROPERTY(SaveGame)
	uint64 State2 = 0;

	UPROPERTY(SaveGame)
	uint64 State3 = 0;
};
