// GENESIS: Der Kreislauf des Lebens

#include "GenesisKarma.h"
#include "GenesisMath.h"

float FGenesisKarmaVector::Get(EGenesisKarmaDimension Dimension) const
{
	switch (Dimension)
	{
	case EGenesisKarmaDimension::Compassion: return Compassion;
	case EGenesisKarmaDimension::Honesty:    return Honesty;
	case EGenesisKarmaDimension::Courage:    return Courage;
	case EGenesisKarmaDimension::Generosity: return Generosity;
	case EGenesisKarmaDimension::Wisdom:     return Wisdom;
	case EGenesisKarmaDimension::Love:       return Love;
	default:                                 return 0.0f;
	}
}

void FGenesisKarmaVector::Set(EGenesisKarmaDimension Dimension, float Value)
{
	switch (Dimension)
	{
	case EGenesisKarmaDimension::Compassion: Compassion = Value; break;
	case EGenesisKarmaDimension::Honesty:    Honesty = Value; break;
	case EGenesisKarmaDimension::Courage:    Courage = Value; break;
	case EGenesisKarmaDimension::Generosity: Generosity = Value; break;
	case EGenesisKarmaDimension::Wisdom:     Wisdom = Value; break;
	case EGenesisKarmaDimension::Love:       Love = Value; break;
	default: break;
	}
}

FGenesisKarmaVector FGenesisKarmaVector::operator*(float Scale) const
{
	FGenesisKarmaVector Result;
	for (int32 Index = 0; Index < GenesisKarmaDimensionCount; ++Index)
	{
		const EGenesisKarmaDimension Dimension = static_cast<EGenesisKarmaDimension>(Index);
		Result.Set(Dimension, Get(Dimension) * Scale);
	}
	return Result;
}

FGenesisKarmaVector& FGenesisKarmaVector::operator+=(const FGenesisKarmaVector& Other)
{
	for (int32 Index = 0; Index < GenesisKarmaDimensionCount; ++Index)
	{
		const EGenesisKarmaDimension Dimension = static_cast<EGenesisKarmaDimension>(Index);
		Set(Dimension, Get(Dimension) + Other.Get(Dimension));
	}
	return *this;
}

bool FGenesisKarmaVector::IsNearlyZero(float Tolerance) const
{
	for (int32 Index = 0; Index < GenesisKarmaDimensionCount; ++Index)
	{
		if (!FMath::IsNearlyZero(Get(static_cast<EGenesisKarmaDimension>(Index)), Tolerance))
		{
			return false;
		}
	}
	return true;
}

void FGenesisKarmaProfile::ApplyImpulse(const FGenesisKarmaVector& Impulse, const FGenesisKarmaTuning& Tuning)
{
	if (Impulse.IsNearlyZero())
	{
		return;
	}

	for (int32 Index = 0; Index < GenesisKarmaDimensionCount; ++Index)
	{
		const EGenesisKarmaDimension Dimension = static_cast<EGenesisKarmaDimension>(Index);
		float Delta = Impulse.Get(Dimension);
		if (FMath::IsNearlyZero(Delta))
		{
			continue;
		}

		// Gewohnheit: Wer oft so handelt, wird dadurch stärker geprägt
		const float CurrentHabit = Habit.Get(Dimension);
		if (FMath::Sign(Delta) == FMath::Sign(CurrentHabit))
		{
			Delta *= 1.0f + Tuning.HabitAmplification * FMath::Abs(CurrentHabit);
		}

		Values.Set(Dimension, GenesisMath::ApplySaturatingDelta(Values.Get(Dimension), Delta, -100.0f, 100.0f, Tuning.SaturationExponent));

		const float HabitTarget = FMath::Clamp(Delta / 10.0f, -1.0f, 1.0f);
		Habit.Set(Dimension, FMath::Lerp(CurrentHabit, HabitTarget, Tuning.HabitSmoothing));
	}

	++IntegratedActions;
}
