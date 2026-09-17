// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Gemeinsame mathematische Bausteine der Simulation.
 * Alle Funktionen sind zustandslos und deterministisch.
 */
namespace GenesisMath
{
	/**
	 * Addiert Delta mit Sättigung: Je näher ein Wert am Extrem liegt, desto schwerer lässt er sich weiter
	 * dorthin verschieben. Bewegungen zurück Richtung Mitte bleiben leicht.
	 * Modelliert Charakterbildung: Gewohnheiten verfestigen sich, Extreme entstehen nur durch viele Handlungen.
	 *
	 * @param Exponent 0 = lineare Addition, 1 = proportionale Sättigung, >1 = stärkere Sättigung
	 */
	inline float ApplySaturatingDelta(float Current, float Delta, float MinValue, float MaxValue, float Exponent = 1.0f)
	{
		if (FMath::IsNearlyZero(Delta) || MaxValue <= MinValue)
		{
			return FMath::Clamp(Current, MinValue, MaxValue);
		}

		const float Bound = Delta > 0.0f ? MaxValue : MinValue;
		const float HalfRange = (MaxValue - MinValue) * 0.5f;
		const float Room = FMath::Abs(Bound - Current);
		const float Headroom = FMath::Clamp(Room / HalfRange, 0.0f, 1.0f);
		const float Factor = Exponent <= 0.0f ? 1.0f : FMath::Pow(Headroom, Exponent);

		return FMath::Clamp(Current + Delta * Factor, MinValue, MaxValue);
	}

	/** Exponentieller Zerfall über eine Halbwertszeit (beliebige Zeiteinheit, beide Parameter gleich). */
	inline double ExponentialDecay(double Value, double ElapsedTime, double HalfLife)
	{
		if (HalfLife <= 0.0 || ElapsedTime <= 0.0)
		{
			return ElapsedTime > 0.0 && HalfLife <= 0.0 ? 0.0 : Value;
		}
		return Value * FMath::Pow(0.5, ElapsedTime / HalfLife);
	}

	/** Bewegt Value mit Rate (Anteil pro Zeiteinheit, 0..1) Richtung Target. Zeitschritt-unabhängig. */
	inline float ApproachExponential(float Value, float Target, float RatePerUnit, double ElapsedUnits)
	{
		const double Rate = FMath::Clamp(static_cast<double>(RatePerUnit), 0.0, 1.0);
		const double Keep = FMath::Pow(1.0 - Rate, FMath::Max(ElapsedUnits, 0.0));
		return static_cast<float>(Target + (Value - Target) * Keep);
	}

	/** Jaccard-Ähnlichkeit zweier Tag-Mengen (exakte Übereinstimmung). 1 = identisch, 0 = disjunkt. */
	inline float TagSimilarity(const FGameplayTagContainer& A, const FGameplayTagContainer& B)
	{
		const int32 NumA = A.Num();
		const int32 NumB = B.Num();
		if (NumA == 0 && NumB == 0)
		{
			return 1.0f;
		}

		int32 Intersection = 0;
		for (const FGameplayTag& Tag : A)
		{
			if (B.HasTagExact(Tag))
			{
				++Intersection;
			}
		}

		const int32 Union = NumA + NumB - Intersection;
		return Union > 0 ? static_cast<float>(Intersection) / static_cast<float>(Union) : 0.0f;
	}

	/** Anzahl der Tags aus A, die (hierarchisch) in B enthalten sind. */
	inline int32 CountMatchingTags(const FGameplayTagContainer& A, const FGameplayTagContainer& B)
	{
		int32 Count = 0;
		for (const FGameplayTag& Tag : A)
		{
			if (B.HasTag(Tag))
			{
				++Count;
			}
		}
		return Count;
	}
}
