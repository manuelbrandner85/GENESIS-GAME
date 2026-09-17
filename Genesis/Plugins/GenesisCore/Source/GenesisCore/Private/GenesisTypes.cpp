// GENESIS: Der Kreislauf des Lebens

#include "GenesisTypes.h"

FGenesisTimestamp FGenesisTimestamp::FromCalendar(int64 Year, int32 DayOfYear, int32 Hour, int32 Minute)
{
	return FGenesisTimestamp(
		Year * SecondsPerYear
		+ static_cast<int64>(DayOfYear) * SecondsPerDay
		+ static_cast<int64>(Hour) * SecondsPerHour
		+ static_cast<int64>(Minute) * SecondsPerMinute);
}

FString FGenesisTimestamp::ToString() const
{
	return FString::Printf(TEXT("Jahr %lld, Tag %d, %02d:%02d"), GetYear(), GetDayOfYear() + 1, GetHourOfDay(), GetMinuteOfHour());
}

namespace GenesisWeightedTags
{
	float GetWeight(const TArray<FGenesisWeightedTag>& Tags, const FGameplayTag& Tag)
	{
		for (const FGenesisWeightedTag& Entry : Tags)
		{
			if (Entry.Tag == Tag)
			{
				return Entry.Weight;
			}
		}
		return 0.0f;
	}

	float AddWeight(TArray<FGenesisWeightedTag>& Tags, const FGameplayTag& Tag, float Delta, float MinValue, float MaxValue)
	{
		for (FGenesisWeightedTag& Entry : Tags)
		{
			if (Entry.Tag == Tag)
			{
				Entry.Weight = FMath::Clamp(Entry.Weight + Delta, MinValue, MaxValue);
				return Entry.Weight;
			}
		}

		const float NewWeight = FMath::Clamp(Delta, MinValue, MaxValue);
		Tags.Emplace(Tag, NewWeight);
		return NewWeight;
	}

	void RemoveNegligible(TArray<FGenesisWeightedTag>& Tags, float Epsilon)
	{
		Tags.RemoveAll([Epsilon](const FGenesisWeightedTag& Entry)
		{
			return FMath::Abs(Entry.Weight) < Epsilon;
		});
	}
}
