// GENESIS: Der Kreislauf des Lebens

#include "GenesisMotherDay.h"

namespace GenesisMotherDay
{
	namespace
	{
		float Smooth(float Edge0, float Edge1, float Value)
		{
			const float T = FMath::Clamp((Value - Edge0) / FMath::Max(KINDA_SMALL_NUMBER, Edge1 - Edge0), 0.0f, 1.0f);
			return T * T * (3.0f - 2.0f * T);
		}

		bool Within(double Hour, double Start, double Minutes)
		{
			return Hour >= Start && Hour < Start + Minutes / 60.0;
		}

		/** Kleidung lässt nur einen Teil des Lichts auf die Haut: dünner Stoff im Sommer 10–20 %. */
		constexpr float ClothingTransmission = 0.15f;
	}

	float WombTransmission(const FGenesisMotherDayTuning& Tuning, float Weeks)
	{
		// Zwischen den gemessenen Punkten logarithmisch (die Bauchdecke wird mit dem Wachsen dünner und gedehnter);
		// vor der Mitte weniger, früh liegt die Gebärmutter noch hinter dem Becken
		const float Mid = FMath::Max(1.0e-6f, Tuning.WombTransmissionMid);
		const float Term = FMath::Max(Mid, Tuning.WombTransmissionTerm);
		if (Weeks <= 20.0f)
		{
			return Mid * Smooth(12.0f, 20.0f, Weeks);
		}
		const float Alpha = FMath::Clamp((Weeks - 20.0f) / 20.0f, 0.0f, 1.0f);
		return FMath::Exp(FMath::Lerp(FMath::Loge(Mid), FMath::Loge(Term), Alpha));
	}

	float DaylightLux(const FGenesisMotherDayTuning& Tuning, double Hour)
	{
		// Sonne von 6 bis 20 Uhr, am höchsten mittags; morgens und abends wenig (Parraguez: kleine Werte um 9 und 18 Uhr)
		const double Phase = (Hour - 6.0) / 14.0;
		if (Phase <= 0.0 || Phase >= 1.0)
		{
			return 0.0f;
		}
		return Tuning.OutdoorNoonLux * static_cast<float>(FMath::Pow(FMath::Sin(PI * Phase), 2.0));
	}

	FGenesisMotherMoment Evaluate(const FGenesisMotherDayTuning& Tuning, double Hour, int32 DayIndex, float Weeks, int32 Seed)
	{
		// Jeder Tag etwas anders, aber reproduzierbar
		FRandomStream Day(HashCombine(GetTypeHash(Seed), GetTypeHash(DayIndex)));
		const double Wake = Tuning.WakeHour + Day.FRandRange(-0.5f, 0.5f);
		const double Sleep = Tuning.SleepHour + Day.FRandRange(-0.5f, 0.75f);
		const bool bWalksToday = Day.FRand() < 0.7f;
		const double Walk = Tuning.WalkHour + Day.FRandRange(-0.75f, 0.75f);
		const bool bMusicToday = Day.FRand() < 0.6f;
		const double BellyTalk = Tuning.BellyTalkHour + Day.FRandRange(-0.4f, 0.4f);
		Hour = FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);

		FGenesisMotherMoment Moment;
		const bool bAsleep = Hour < Wake || Hour >= Sleep;
		bool bEating = false;
		for (float Meal : Tuning.MealHours)
		{
			bEating |= Within(Hour, Meal, Tuning.MealMinutes);
			if (Hour > Meal)
			{
				// Darmgeräusche nach dem Essen, über einige Stunden abklingend
				Moment.Digestion = FMath::Max(Moment.Digestion, FMath::Exp(-static_cast<float>(Hour - Meal) / 1.5f));
			}
		}
		const bool bWalking = !bAsleep && bWalksToday && Within(Hour, Walk, Tuning.WalkMinutes);
		const bool bWorking = !bAsleep && Hour >= Tuning.WorkStartHour && Hour < Tuning.WorkEndHour;

		if (bAsleep)            { Moment.Activity = EGenesisMotherActivity::Sleeping; }
		else if (bEating)       { Moment.Activity = EGenesisMotherActivity::Eating; }
		else if (bWalking)      { Moment.Activity = EGenesisMotherActivity::Walking; }
		else if (bWorking)      { Moment.Activity = EGenesisMotherActivity::Sitting; }
		else                    { Moment.Activity = EGenesisMotherActivity::Resting; }

		// Puls: in der Schwangerschaft bis +15/min, dazu Schlaf und Bewegung
		float Heart = Tuning.RestingHeartRate + Tuning.PregnancyHeartRateRise * Smooth(8.0f, 32.0f, Weeks);
		switch (Moment.Activity)
		{
		case EGenesisMotherActivity::Sleeping: Heart -= 8.0f; break;
		case EGenesisMotherActivity::Sitting:  Heart += 3.0f; break;
		case EGenesisMotherActivity::Eating:   Heart += 5.0f; break;
		case EGenesisMotherActivity::Walking:  Heart += 25.0f; break;
		default: break;
		}
		Moment.HeartRateBpm = Heart;

		// Licht auf dem Bauch: draußen Tageslicht, drinnen Fenster oder Lampe, im Schlaf dunkel
		const float Daylight = DaylightLux(Tuning, Hour);
		Moment.bOutdoors = bWalking;
		if (bAsleep)
		{
			Moment.BellyLux = 0.0f;
		}
		else if (bWalking)
		{
			Moment.BellyLux = Daylight * ClothingTransmission;
		}
		else
		{
			Moment.BellyLux = (Daylight > 200.0f ? Tuning.IndoorDayLux : Tuning.IndoorEveningLux) * ClothingTransmission;
		}
		Moment.WombLux = Moment.BellyLux * WombTransmission(Tuning, Weeks);

		// Stimme: bei der Arbeit, beim Essen, unterwegs; abends spricht sie mit dem Kind
		switch (Moment.Activity)
		{
		case EGenesisMotherActivity::Sitting: Moment.Speaking = 0.35f; break;
		case EGenesisMotherActivity::Eating:  Moment.Speaking = 0.4f; break;
		case EGenesisMotherActivity::Walking: Moment.Speaking = 0.2f; break;
		case EGenesisMotherActivity::Resting: Moment.Speaking = 0.25f; break;
		default: break;
		}
		if (!bAsleep && Weeks >= Tuning.BellyTalkFromWeeks && FMath::Abs(Hour - BellyTalk) < 0.25)
		{
			Moment.bTalkingToBelly = true;
			Moment.Speaking = 0.8f;
		}
		Moment.bMusic = !bAsleep && bMusicToday && FMath::Abs(Hour - Tuning.MusicHour) < 0.5;
		Moment.Rocking = bWalking ? 0.8f : 0.0f;
		return Moment;
	}

	FString GetActivityName(EGenesisMotherActivity Activity)
	{
		switch (Activity)
		{
		case EGenesisMotherActivity::Sleeping: return TEXT("schläft");
		case EGenesisMotherActivity::Resting:  return TEXT("ruht");
		case EGenesisMotherActivity::Sitting:  return TEXT("sitzt");
		case EGenesisMotherActivity::Walking:  return TEXT("geht");
		case EGenesisMotherActivity::Eating:   return TEXT("isst");
		default:                                return TEXT("?");
		}
	}
}
