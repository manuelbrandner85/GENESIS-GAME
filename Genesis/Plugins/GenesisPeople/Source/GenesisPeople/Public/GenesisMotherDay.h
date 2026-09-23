// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisMotherDay.generated.h"

/**
 * Der Tag der Mutter in der Schwangerschaft (GENESIS-044 Teil 1b, Docs/34): Was sie gerade tut, wie schnell ihr Herz
 * schlägt, ob sie spricht, wie viel Licht auf den Bauch fällt und wie viel davon beim Kind ankommt. Das Kind erlebt
 * die Welt zuerst durch sie. Reine Logik; der Tag ist aus Stunde und Tagesnummer reproduzierbar.
 *
 * Grundlagen: Puls in der Schwangerschaft +10–20/min, am höchsten im dritten Drittel (Sanghavi & Rutherford 2014);
 * Licht im Mutterleib 0,2 % des Außenlichts zur Mitte, bis 5,4 % am Ende der Tragzeit, mit dem Tageslauf
 * (Parraguez et al. 1998); die Bewegungen des Kindes am stärksten abends 21–22 Uhr, am schwächsten 1–5 Uhr; Gehen
 * der Mutter wiegt das Kind in Ruhe.
 */

UENUM(BlueprintType)
enum class EGenesisMotherActivity : uint8
{
	Sleeping,
	Resting,
	Sitting,
	Walking,
	Eating
};

USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherDayTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Day") float WakeHour = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float SleepHour = 23.0f;
	UPROPERTY(EditAnywhere, Category = "Day") TArray<float> MealHours = { 7.5f, 12.5f, 19.0f };
	UPROPERTY(EditAnywhere, Category = "Day") float MealMinutes = 30.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float WorkStartHour = 9.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float WorkEndHour = 16.5f;
	/** Spaziergang draußen, am späten Nachmittag. */
	UPROPERTY(EditAnywhere, Category = "Day") float WalkHour = 17.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float WalkMinutes = 45.0f;
	/** Abends spricht sie mit dem Bauch – viele Mütter tun das schon, bevor sie das Kind spüren (SSW 20). */
	UPROPERTY(EditAnywhere, Category = "Day") float BellyTalkHour = 21.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float BellyTalkFromWeeks = 16.0f;
	UPROPERTY(EditAnywhere, Category = "Day") float MusicHour = 20.0f;

	/** Ruhepuls außerhalb der Schwangerschaft und der Anstieg bis zum dritten Drittel. */
	UPROPERTY(EditAnywhere, Category = "Heart") float RestingHeartRate = 70.0f;
	UPROPERTY(EditAnywhere, Category = "Heart") float PregnancyHeartRateRise = 15.0f;

	/** Beleuchtungsstärke am Bauch (lx): draußen im Schatten mittags, drinnen tagsüber, drinnen abends mit Lampe. */
	UPROPERTY(EditAnywhere, Category = "Light") float OutdoorNoonLux = 20000.0f;
	UPROPERTY(EditAnywhere, Category = "Light") float IndoorDayLux = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Light") float IndoorEveningLux = 120.0f;
	/** Anteil, der im Mutterleib ankommt: zur Mitte (SSW 20) und am Ende (SSW 40). */
	UPROPERTY(EditAnywhere, Category = "Light") float WombTransmissionMid = 0.002f;
	UPROPERTY(EditAnywhere, Category = "Light") float WombTransmissionTerm = 0.05f;
};

/** Ein Augenblick im Tag der Mutter. */
USTRUCT(BlueprintType)
struct GENESISPEOPLE_API FGenesisMotherMoment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") EGenesisMotherActivity Activity = EGenesisMotherActivity::Resting;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float HeartRateBpm = 70.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") bool bOutdoors = false;
	/** Licht auf dem Bauch und im Mutterleib (lx). Im Mutterleib fast nur Rot. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float BellyLux = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float WombLux = 0.0f;
	/** Sie spricht (0..1 – Anteil der Zeit). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float Speaking = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") bool bTalkingToBelly = false;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") bool bMusic = false;
	/** Verdauung nach dem Essen (0..1): Darmgeräusche. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float Digestion = 0.0f;
	/** Wiegen durch Gehen (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Mother") float Rocking = 0.0f;
};

namespace GenesisMotherDay
{
	/**
	 * Der Augenblick zur Tageszeit HourOfDay (0..24) am Tag DayIndex in der SSW GestationalWeeks.
	 * Seed macht jeden Tag etwas anders (Spaziergang ja oder nein, Uhrzeiten streuen), aber reproduzierbar.
	 */
	GENESISPEOPLE_API FGenesisMotherMoment Evaluate(const FGenesisMotherDayTuning& Tuning, double HourOfDay, int32 DayIndex,
		float GestationalWeeks, int32 Seed);

	/** Anteil des Außenlichts, der in dieser SSW im Mutterleib ankommt. */
	GENESISPEOPLE_API float WombTransmission(const FGenesisMotherDayTuning& Tuning, float GestationalWeeks);

	/** Tageslicht draußen im Schatten (lx) zur Stunde – 0 nachts. */
	GENESISPEOPLE_API float DaylightLux(const FGenesisMotherDayTuning& Tuning, double HourOfDay);

	GENESISPEOPLE_API FString GetActivityName(EGenesisMotherActivity Activity);
}
