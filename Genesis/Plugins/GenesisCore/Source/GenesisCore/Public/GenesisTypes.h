// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisTypes.generated.h"

/**
 * Absoluter Zeitpunkt in der simulierten Welt.
 *
 * Gespeichert als Sekunden seit dem GENESIS-Epochenursprung (Jahr 0, Tag 0, 00:00 Uhr).
 * int64 deckt ±292 Milliarden Jahre ab – genug für Epochen, Generationen und kosmische Zeiträume.
 *
 * Kalender: bewusst vereinfacht auf 365 Tage pro Jahr ohne Schaltjahre. Historische Kalender
 * (julianisch, gregorianisch, fiktive Zukunftskalender) sind reine Darstellung und werden in
 * GenesisWorld/GenesisUI auf diesen Zeitstrahl abgebildet.
 */
USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisTimestamp
{
	GENERATED_BODY()

	static constexpr int64 SecondsPerMinute = 60;
	static constexpr int64 SecondsPerHour = 60 * SecondsPerMinute;
	static constexpr int64 SecondsPerDay = 24 * SecondsPerHour;
	static constexpr int64 DaysPerYear = 365;
	static constexpr int64 SecondsPerYear = DaysPerYear * SecondsPerDay;

	/** Sekunden seit Epochenursprung. Negative Werte liegen vor dem Ursprung. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Genesis|Time")
	int64 Seconds = 0;

	FGenesisTimestamp() = default;
	explicit FGenesisTimestamp(int64 InSeconds) : Seconds(InSeconds) {}

	/** Baut einen Zeitpunkt aus Kalenderangaben. DayOfYear ist 0-basiert (0..364). */
	static FGenesisTimestamp FromCalendar(int64 Year, int32 DayOfYear = 0, int32 Hour = 0, int32 Minute = 0);

	static int64 YearsToSeconds(double Years) { return static_cast<int64>(Years * static_cast<double>(SecondsPerYear)); }
	static int64 DaysToSeconds(double Days) { return static_cast<int64>(Days * static_cast<double>(SecondsPerDay)); }
	static int64 HoursToSeconds(double Hours) { return static_cast<int64>(Hours * static_cast<double>(SecondsPerHour)); }

	int64 GetYear() const { return FloorDiv(Seconds, SecondsPerYear); }
	int32 GetDayOfYear() const { return static_cast<int32>(FloorMod(Seconds, SecondsPerYear) / SecondsPerDay); }
	int32 GetHourOfDay() const { return static_cast<int32>(FloorMod(Seconds, SecondsPerDay) / SecondsPerHour); }
	int32 GetMinuteOfHour() const { return static_cast<int32>(FloorMod(Seconds, SecondsPerHour) / SecondsPerMinute); }

	/** Zeitpunkt als Gleitkomma-Jahre (für Alters- und Zerfallsberechnungen). */
	double ToYears() const { return static_cast<double>(Seconds) / static_cast<double>(SecondsPerYear); }

	/** Vergangene Jahre zwischen zwei Zeitpunkten (negativ, wenn To vor From liegt). */
	static double YearsBetween(const FGenesisTimestamp& From, const FGenesisTimestamp& To)
	{
		return static_cast<double>(To.Seconds - From.Seconds) / static_cast<double>(SecondsPerYear);
	}

	FString ToString() const;

	FGenesisTimestamp operator+(int64 DeltaSeconds) const { return FGenesisTimestamp(Seconds + DeltaSeconds); }
	FGenesisTimestamp operator-(int64 DeltaSeconds) const { return FGenesisTimestamp(Seconds - DeltaSeconds); }
	FGenesisTimestamp& operator+=(int64 DeltaSeconds) { Seconds += DeltaSeconds; return *this; }
	int64 operator-(const FGenesisTimestamp& Other) const { return Seconds - Other.Seconds; }

	bool operator==(const FGenesisTimestamp& Other) const { return Seconds == Other.Seconds; }
	bool operator!=(const FGenesisTimestamp& Other) const { return Seconds != Other.Seconds; }
	bool operator<(const FGenesisTimestamp& Other) const { return Seconds < Other.Seconds; }
	bool operator<=(const FGenesisTimestamp& Other) const { return Seconds <= Other.Seconds; }
	bool operator>(const FGenesisTimestamp& Other) const { return Seconds > Other.Seconds; }
	bool operator>=(const FGenesisTimestamp& Other) const { return Seconds >= Other.Seconds; }

	/** Ganzzahlige Division mit Abrundung Richtung minus unendlich (korrekt für negative Zeitpunkte). */
	static int64 FloorDiv(int64 A, int64 B)
	{
		int64 Quotient = A / B;
		if ((A % B != 0) && ((A < 0) != (B < 0)))
		{
			--Quotient;
		}
		return Quotient;
	}

	static int64 FloorMod(int64 A, int64 B)
	{
		return A - FloorDiv(A, B) * B;
	}
};

/** Gewichteter Tag – universeller Baustein für Werte, Tendenzen, Normen und Muster. */
USTRUCT(BlueprintType)
struct GENESISCORE_API FGenesisWeightedTag
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Genesis")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Genesis")
	float Weight = 0.0f;

	FGenesisWeightedTag() = default;
	FGenesisWeightedTag(const FGameplayTag& InTag, float InWeight) : Tag(InTag), Weight(InWeight) {}
};

namespace GenesisWeightedTags
{
	/** Liefert das Gewicht eines exakt passenden Tags oder 0. */
	GENESISCORE_API float GetWeight(const TArray<FGenesisWeightedTag>& Tags, const FGameplayTag& Tag);

	/** Addiert Delta auf den Tag (legt ihn bei Bedarf an) und klemmt das Ergebnis. Gibt den neuen Wert zurück. */
	GENESISCORE_API float AddWeight(TArray<FGenesisWeightedTag>& Tags, const FGameplayTag& Tag, float Delta, float MinValue, float MaxValue);

	/** Entfernt Einträge, deren Betrag unter Epsilon liegt. */
	GENESISCORE_API void RemoveNegligible(TArray<FGenesisWeightedTag>& Tags, float Epsilon);
}
