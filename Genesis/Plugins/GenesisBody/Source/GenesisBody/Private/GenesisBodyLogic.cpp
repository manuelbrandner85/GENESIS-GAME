// GENESIS: Der Kreislauf des Lebens

#include "GenesisBodyLogic.h"
#include "GenesisFetalLogic.h"
#include "GenesisBodyGameplayTags.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisMath.h"

namespace GenesisBodyLogic
{
	namespace
	{
		constexpr double SecondsPerWeek = 7.0 * FGenesisTimestamp::SecondsPerDay;

		/** Vollständige Schwangerschaft in Wochen seit der Befruchtung. */
		constexpr double FullTermWeeks = 38.0;

		/** Pränataler Zeitplan: Beginn und Abschluss der strukturellen Ausbildung (Wochen seit Befruchtung). */
		struct FDevelopmentWindow
		{
			float StartWeek;
			float EndWeek;
		};

		const FDevelopmentWindow OrganSchedule[GenesisOrganCount] = {
			{ 3.0f, 8.0f },   // Herz: schlägt ab ca. Woche 5
			{ 4.0f, 36.0f },  // Lunge: reift als letztes
			{ 4.0f, 30.0f },  // Leber
			{ 3.0f, 38.0f },  // Gehirn (reift nach der Geburt über die Kapazität weiter)
			{ 6.0f, 38.0f },  // Immunsystem
			{ 5.0f, 36.0f },  // Muskulatur
			{ 6.0f, 38.0f },  // Skelett
			{ 3.0f, 25.0f },  // Nervensystem
			{ 4.0f, 32.0f }   // Stoffwechsel
		};

		const FDevelopmentWindow SenseSchedule[GenesisSenseCount] = {
			// Nach der Recherche in Docs/34 (dort in SSW; hier Wochen nach Befruchtung = SSW − 2)
			{ 22.0f, 34.0f }, // Sehen: Lider öffnen sich SSW 26–28, Pupillen reagieren ab SSW 31
			{ 17.0f, 33.0f }, // Hören: erster Ton SSW 19, 1000 Hz ab SSW 33, 3000 Hz ab SSW 35
			{ 18.0f, 28.0f }, // Riechen
			{ 6.0f, 12.0f },  // Schmecken: Geschmacksknospen SSW 8, schluckt Fruchtwasser ab SSW 12
			{ 5.5f, 12.0f }   // Tasten: um den Mund SSW 7,5–8, ganzer Körper SSW 14
		};

		/** Organspezifische Alterung: Höhepunkt und jährlicher Rückgang danach. */
		struct FAgingProfile
		{
			float PeakAge;
			float DeclinePerYear;
		};

		const FAgingProfile OrganAging[GenesisOrganCount] = {
			{ 25.0f, 0.006f }, // Herz
			{ 25.0f, 0.008f }, // Lunge
			{ 25.0f, 0.005f }, // Leber
			{ 25.0f, 0.004f }, // Gehirn
			{ 20.0f, 0.007f }, // Immunsystem
			{ 28.0f, 0.008f }, // Muskulatur
			{ 30.0f, 0.007f }, // Skelett
			{ 25.0f, 0.005f }, // Nervensystem
			{ 25.0f, 0.006f }  // Stoffwechsel
		};

		float SmoothStep(float Edge0, float Edge1, float Value)
		{
			const float T = FMath::Clamp((Value - Edge0) / FMath::Max(KINDA_SMALL_NUMBER, Edge1 - Edge0), 0.0f, 1.0f);
			return T * T * (3.0f - 2.0f * T);
		}

		float ActivityExertion(EGenesisActivity Activity)
		{
			switch (Activity)
			{
			case EGenesisActivity::Sleep:    return 0.0f;
			case EGenesisActivity::Rest:     return 0.05f;
			case EGenesisActivity::Light:    return 0.3f;
			case EGenesisActivity::Moderate: return 0.6f;
			case EGenesisActivity::Intense:  return 1.0f;
			default:                         return 0.05f;
			}
		}

		/** Kapazität eines Organs für ein biologisches Alter (Kindheit steigend, Alter sinkend, ab 60 beschleunigt). */
		float CapacityForAge(int32 OrganIndex, double BiologicalAge, float DeclineMultiplier)
		{
			const FAgingProfile& Profile = OrganAging[OrganIndex];
			const float Age = static_cast<float>(BiologicalAge);
			if (Age < Profile.PeakAge)
			{
				return 0.3f + 0.7f * SmoothStep(0.0f, Profile.PeakAge, Age);
			}
			const float YearsAfterPeak = Age - Profile.PeakAge;
			const float LateYears = FMath::Max(0.0f, Age - 60.0f);
			return FMath::Clamp(1.0f - Profile.DeclinePerYear * DeclineMultiplier * (YearsAfterPeak + 1.5f * LateYears), 0.0f, 1.0f);
		}

		void UpdateDevelopment(FGenesisBodyState& Body, const FGenesisTimestamp& Now)
		{
			// Nach der Geburt reifen unreife Organe über die vergangene Zeit weiter
			double EffectiveWeeks = GetGestationalWeeks(Body, Now);
			if (Body.bBorn)
			{
				EffectiveWeeks = Body.GestationalWeeksAtBirth + static_cast<double>(Now - Body.BirthTime) / SecondsPerWeek;
			}

			for (int32 Index = 0; Index < GenesisOrganCount; ++Index)
			{
				FGenesisOrganState& Organ = Body.Organs[Index];
				const float Scheduled = SmoothStep(OrganSchedule[Index].StartWeek, OrganSchedule[Index].EndWeek, static_cast<float>(EffectiveWeeks));
				// Entwicklungsqualität begrenzt die erreichbare Ausbildung
				const float Target = Scheduled * (0.7f + 0.3f * Organ.DevelopmentQuality);
				Organ.Development = FMath::Max(Organ.Development, Target);
			}

			for (int32 Index = 0; Index < GenesisSenseCount; ++Index)
			{
				const float Scheduled = SmoothStep(SenseSchedule[Index].StartWeek, SenseSchedule[Index].EndWeek, static_cast<float>(EffectiveWeeks));
				Body.Senses[Index].Development = FMath::Max(Body.Senses[Index].Development, Scheduled);
			}
		}

		void UpdateSenses(FGenesisBodyState& Body, const FGenesisTimestamp& Now)
		{
			const float Age = static_cast<float>(GetAgeYears(Body, Now));
			const float BioAge = static_cast<float>(Body.BiologicalAgeYears);
			// Strukturelle Intaktheit des Nervensystems (nicht die altersabhängige Kapazität)
			const FGenesisOrganState& Nervous = Body.Organ(EGenesisOrgan::Nervous);
			const float NervousIntegrity = Nervous.Development * (1.0f - Nervous.Damage) * (1.0f - Nervous.Wear);

			auto Acuity = [&](EGenesisBodySense Type) -> float
			{
				if (!Body.bBorn)
				{
					// Im Mutterleib: gedämpfte, aber vorhandene Wahrnehmung
					return Body.Sense(Type).Development * 0.3f;
				}

				float Maturation = 1.0f;
				float Decline = 0.0f;
				switch (Type)
				{
				case EGenesisBodySense::Sight:
					// Neugeborene sehen sehr unscharf, volle Sehschärfe mit etwa 3 Jahren; Altersweitsichtigkeit ab ca. 45
					Maturation = 0.05f + 0.95f * SmoothStep(0.0f, 3.0f, Age);
					Decline = 0.015f * FMath::Max(0.0f, BioAge - 45.0f);
					break;
				case EGenesisBodySense::Hearing:
					Maturation = 0.7f + 0.3f * SmoothStep(0.0f, 0.5f, Age);
					Decline = 0.01f * FMath::Max(0.0f, BioAge - 50.0f);
					break;
				case EGenesisBodySense::Smell:
					Maturation = (0.8f + 0.2f * SmoothStep(0.0f, 1.0f, Age)) * (0.8f + 0.4f * Body.Genetics.SmellAcuity);
					Decline = 0.02f * FMath::Max(0.0f, BioAge - 60.0f);
					break;
				case EGenesisBodySense::Taste:
					Maturation = 0.9f + 0.1f * SmoothStep(0.0f, 1.0f, Age);
					Decline = 0.01f * FMath::Max(0.0f, BioAge - 60.0f);
					break;
				case EGenesisBodySense::Touch:
					Maturation = 0.9f + 0.1f * SmoothStep(0.0f, 1.0f, Age);
					Decline = 0.01f * FMath::Max(0.0f, BioAge - 65.0f);
					break;
				}
				return FMath::Clamp(Body.Sense(Type).Development * Maturation * (1.0f - Decline) * (0.8f + 0.2f * NervousIntegrity), 0.0f, 1.0f);
			};

			for (int32 Index = 0; Index < GenesisSenseCount; ++Index)
			{
				const EGenesisBodySense Type = static_cast<EGenesisBodySense>(Index);
				Body.Sense(Type).Acuity = Acuity(Type);
			}
		}

		void UpdateCapacities(FGenesisBodyState& Body)
		{
			const float VitalityFactor = 0.9f + 0.2f * Body.Vitality.Vitality;
			for (int32 Index = 0; Index < GenesisOrganCount; ++Index)
			{
				const EGenesisOrgan Type = static_cast<EGenesisOrgan>(Index);
				float DeclineMultiplier = 1.0f;
				float Bonus = 0.0f;

				switch (Type)
				{
				case EGenesisOrgan::Heart:
					DeclineMultiplier = 1.0f + 2.0f * FMath::Max(0.0f, Body.Genetics.CardioRisk - 0.3f);
					Bonus = 0.15f * Body.Fitness;
					break;
				case EGenesisOrgan::Lungs:
					Bonus = 0.15f * Body.Fitness;
					break;
				case EGenesisOrgan::Musculature:
					Bonus = 0.15f * Body.Fitness - 0.2f * (0.5f - Body.Genetics.MuscleBase);
					DeclineMultiplier = 1.0f - 0.5f * Body.Fitness;
					break;
				case EGenesisOrgan::Immune:
					Bonus = 0.1f * (Body.Vitality.Resilience - 0.5f) - 0.15f * FMath::Min(1.0f, Body.SleepDebtHours / 40.0f);
					break;
				case EGenesisOrgan::Brain:
					Bonus = -0.1f * FMath::Min(1.0f, Body.SleepDebtHours / 40.0f);
					break;
				default:
					break;
				}

				Body.Organs[Index].Capacity = FMath::Clamp(CapacityForAge(Index, Body.BiologicalAgeYears, DeclineMultiplier) * VitalityFactor + Bonus, 0.0f, 1.0f);
			}
		}

		void UpdateGrowth(FGenesisBodyState& Body, const FGenesisTimestamp& Now)
		{
			if (!Body.bBorn)
			{
				// Nach der Referenzkurve (WHO, Robinson & Fleming; Docs/34). Länge: Scheitel–Ferse, solange sie nicht messbar ist
				// (vor SSW 14) die Scheitel-Steiß-Länge.
				const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(GenesisFetalLogic::GetReference(),
					static_cast<float>(GetGestationalWeeks(Body, Now) + GenesisFetalLogic::WeeksFromConceptionToGestational));
				Body.HeightCm = Fetal.CrownHeelCm > 0.0f ? Fetal.CrownHeelCm : Fetal.CrownRumpCm;
				Body.WeightKg = Fetal.WeightGrams / 1000.0f;
				return;
			}

			const float Age = static_cast<float>(GetAgeYears(Body, Now));
			const float NewbornFraction = 50.0f / Body.Genetics.AdultHeightCm;
			// Schnelles Wachstum in den ersten Jahren, abgeschlossen mit ~18 (1 J ≈ 71 cm, 5 J ≈ 107 cm, 10 J ≈ 136 cm bei 172 cm Zielgröße)
			float Fraction = NewbornFraction + (1.0f - NewbornFraction) * FMath::Pow(FMath::Clamp(Age / 18.0f, 0.0f, 1.0f), 0.6f);
			// Im Alter wird der Körper etwas kleiner
			Fraction *= 1.0f - 0.001f * FMath::Max(0.0f, static_cast<float>(Body.BiologicalAgeYears) - 60.0f);
			Body.HeightCm = Body.Genetics.AdultHeightCm * Fraction;

			// Körperfett folgt Ernährung, Bewegung und Stoffwechsel
			const float FatTarget = FMath::Clamp(0.12f + 0.15f * (1.0f - Body.Fitness) + 0.08f * (1.0f - Body.Genetics.MetabolismRate) - 0.05f * (1.0f - Body.NutritionQuality), 0.05f, 0.45f);
			Body.BodyFat = FatTarget;

			const float HeightMeters = Body.HeightCm / 100.0f;
			const float Bmi = Age < 2.0f ? 15.0f : (Age < 12.0f ? 16.0f : 19.5f) + 12.0f * (Body.BodyFat - 0.15f) + 3.0f * Body.Organ(EGenesisOrgan::Musculature).Capacity;
			Body.WeightKg = Bmi * HeightMeters * HeightMeters;
		}

		void UpdateSexHormones(FGenesisBodyState& Body, float Age)
		{
			// Pubertät etwa 10–16, Rückgang ab etwa 45
			Body.Hormones.SexHormones = FMath::Clamp(0.05f + 0.85f * SmoothStep(10.0f, 16.0f, Age) - 0.012f * FMath::Max(0.0f, Age - 45.0f), 0.0f, 1.0f);
		}

		float MaxConditionSeverity(const FGenesisBodyState& Body, bool bFever, bool bPain)
		{
			float Highest = 0.0f;
			for (const FGenesisBodyCondition& Condition : Body.Conditions)
			{
				if ((bFever && Condition.bCausesFever) || (bPain && Condition.bCausesPain))
				{
					Highest = FMath::Max(Highest, Condition.Severity);
				}
			}
			return Highest;
		}
	}

	FGenesisBodyGenetics MakeGenetics(const FGenesisGenome* Genome, const TArray<FGenesisTraitDefinition>& Traits)
	{
		FGenesisBodyGenetics Genetics;
		if (!Genome)
		{
			return Genetics;
		}

		using namespace GenesisGeneticsTags;
		const float Height = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, Traits, Trait_Body_Height, 0.5f);
		Genetics.AdultHeightCm = 150.0f + 45.0f * Height;
		Genetics.MuscleBase = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, Traits, Trait_Body_MuscleMass, 0.5f);
		Genetics.CardioRisk = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, Traits, Trait_Risk_Cardiovascular, 0.3f);
		Genetics.MetabolismRate = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, Traits, Trait_Body_Metabolism, 0.5f);
		Genetics.SmellAcuity = GenesisGeneticsLogic::ExpressTraitByTag(*Genome, Traits, Trait_Sense_SmellAcuity, 0.5f);
		return Genetics;
	}

	FGenesisBodyState CreateAtConception(const FGuid& EntityId, const FGuid& GenomeId, const FGenesisBodyGenetics& Genetics,
		const FGenesisConceptionVitality& Vitality, const FGenesisTimestamp& ConceptionTime, EGenesisSimulationLevel Level)
	{
		FGenesisBodyState Body;
		Body.EntityId = EntityId;
		Body.GenomeId = GenomeId;
		Body.SimulationLevel = Level;
		Body.ConceptionTime = ConceptionTime;
		Body.Genetics = Genetics;
		Body.Vitality = Vitality;
		Body.Organs.SetNum(GenesisOrganCount);
		Body.Senses.SetNum(GenesisSenseCount);
		Body.Vitals.HeartRate = 0.0f;
		Body.Vitals.RespiratoryRate = 0.0f;
		Body.Vitals.Hunger = 0.0f;
		UpdateCapacities(Body);
		return Body;
	}

	double GetGestationalWeeks(const FGenesisBodyState& Body, const FGenesisTimestamp& Now)
	{
		const FGenesisTimestamp& Reference = Body.bBorn ? Body.BirthTime : Now;
		return FMath::Max(0.0, static_cast<double>(Reference - Body.ConceptionTime) / SecondsPerWeek);
	}

	double GetAgeYears(const FGenesisBodyState& Body, const FGenesisTimestamp& Now)
	{
		return Body.bBorn ? FMath::Max(0.0, FGenesisTimestamp::YearsBetween(Body.BirthTime, Now)) : 0.0;
	}

	EGenesisDevelopmentStage GetStage(const FGenesisBodyState& Body, const FGenesisTimestamp& Now)
	{
		if (!Body.bAlive)
		{
			return EGenesisDevelopmentStage::Deceased;
		}
		if (!Body.bBorn)
		{
			const double Weeks = GetGestationalWeeks(Body, Now);
			return Weeks < 2.0 ? EGenesisDevelopmentStage::Zygote : (Weeks < 9.0 ? EGenesisDevelopmentStage::Embryo : EGenesisDevelopmentStage::Fetus);
		}

		const double Age = GetAgeYears(Body, Now);
		if (Age < 28.0 / 365.0) return EGenesisDevelopmentStage::Newborn;
		if (Age < 2.0) return EGenesisDevelopmentStage::Infant;
		if (Age < 13.0) return EGenesisDevelopmentStage::Child;
		if (Age < 19.0) return EGenesisDevelopmentStage::Adolescent;
		if (Age < 51.0) return EGenesisDevelopmentStage::Adult;
		return EGenesisDevelopmentStage::Elder;
	}

	void Birth(FGenesisBodyState& Body, const FGenesisTimestamp& Now, const FGenesisBodyTuning& Tuning)
	{
		if (Body.bBorn || !Body.bAlive)
		{
			return;
		}

		UpdateDevelopment(Body, Now);
		Body.GestationalWeeksAtBirth = static_cast<float>(GetGestationalWeeks(Body, Now));
		Body.bBorn = true;
		Body.BirthTime = Now;
		Body.BiologicalAgeYears = 0.0;

		// Erster Atemzug – Kälte, Licht, Geräuschflut: Adrenalinschub
		Body.Vitals.HeartRate = GetBaselineHeartRate(0.0) + 20.0f;
		Body.Vitals.RespiratoryRate = GetBaselineRespiratoryRate(0.0);
		Body.Vitals.BodyTemperature = 36.5f;
		Body.Vitals.Hunger = 0.3f;
		Body.Hormones.Adrenaline = 0.8f;
		Body.Hormones.Cortisol = 0.6f;
		Body.Activity = EGenesisActivity::Light;

		const float LungDevelopment = Body.Organ(EGenesisOrgan::Lungs).Development;
		if (LungDevelopment < Tuning.LungMaturityForBirth)
		{
			const float Severity = FMath::Clamp((Tuning.LungMaturityForBirth - LungDevelopment) * 3.0f, 0.1f, 1.0f);
			ApplyIllness(Body, GenesisBodyTags::Condition_RespiratoryDistress, Severity, 0.0f, 0.03f, false, false, EGenesisOrgan::Lungs, Now);
		}

		UpdateCapacities(Body);
		UpdateSenses(Body, Now);
		UpdateGrowth(Body, Now);
	}

	void AdvanceHours(FGenesisBodyState& Body, const FGenesisTimestamp& End, double Hours, float PsychologicalStress)
	{
		if (!Body.bAlive || Hours <= 0.0)
		{
			return;
		}

		if (!Body.bBorn)
		{
			// Fetaler Herzschlag, sobald das Herz ausgebildet ist; Frequenz nach der Referenz (am schnellsten in SSW 9–10, zum Termin 130–140), Adrenalin hebt sie an
			const float HeartDevelopment = Body.Organ(EGenesisOrgan::Heart).Development;
			const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(GenesisFetalLogic::GetReference(),
				static_cast<float>(GetGestationalWeeks(Body, End) + GenesisFetalLogic::WeeksFromConceptionToGestational));
			Body.Vitals.HeartRate = HeartDevelopment > 0.3f ? Fetal.HeartRateBpm * (1.0f + 0.12f * Body.Hormones.Adrenaline) : 0.0f;
			Body.Hormones.Adrenaline = static_cast<float>(GenesisMath::ExponentialDecay(Body.Hormones.Adrenaline, Hours, 0.25));
			return;
		}

		const double Age = GetAgeYears(Body, End);
		const int32 HourOfDay = End.GetHourOfDay();
		const bool bSleeping = Body.Activity == EGenesisActivity::Sleep;
		const float Exertion = ActivityExertion(Body.Activity);
		const float Stress = FMath::Clamp(PsychologicalStress, 0.0f, 1.0f);
		const float Fever = MaxConditionSeverity(Body, /*bFever*/ true, /*bPain*/ false);

		// --- Hormone (Tagesrhythmus) ---
		FGenesisHormoneState& Hormones = Body.Hormones;
		const bool bNight = HourOfDay >= 22 || HourOfDay < 6;
		const bool bMorning = HourOfDay >= 6 && HourOfDay < 9;

		Hormones.Melatonin = GenesisMath::ApproachExponential(Hormones.Melatonin, bNight ? 0.8f : 0.1f, 0.5f, Hours);
		const float CortisolTarget = FMath::Clamp(0.15f + (bMorning ? 0.25f : 0.0f) + 0.6f * Stress + 0.2f * Exertion + 0.2f * FMath::Max(0.0f, Body.Vitals.SleepPressure - 0.8f), 0.0f, 1.0f);
		Hormones.Cortisol = GenesisMath::ApproachExponential(Hormones.Cortisol, CortisolTarget, 0.3f, Hours);
		Hormones.Adrenaline = FMath::Max(static_cast<float>(GenesisMath::ExponentialDecay(Hormones.Adrenaline, Hours, 0.25)), Exertion >= 0.9f ? 0.4f : 0.0f);
		Hormones.Oxytocin = GenesisMath::ApproachExponential(Hormones.Oxytocin, 0.1f, 0.5f, Hours);
		Hormones.GrowthHormone = GenesisMath::ApproachExponential(Hormones.GrowthHormone, (bSleeping ? 0.7f : 0.2f) * (Age < 25.0 ? 1.0f : 0.5f), 0.4f, Hours);
		UpdateSexHormones(Body, static_cast<float>(Age));

		// --- Vitalwerte ---
		FGenesisVitalState& Vitals = Body.Vitals;
		const float BaseHeartRate = GetBaselineHeartRate(Age);
		const float BaseRespiration = GetBaselineRespiratoryRate(Age);
		const float LungFunction = FMath::Max(0.05f, Body.Organ(EGenesisOrgan::Lungs).GetFunction());
		float RespiratoryDistress = 0.0f;
		for (const FGenesisBodyCondition& Condition : Body.Conditions)
		{
			if (Condition.Condition == GenesisBodyTags::Condition_RespiratoryDistress)
			{
				RespiratoryDistress = FMath::Max(RespiratoryDistress, Condition.Severity);
			}
		}

		const float HeartTarget = BaseHeartRate * (bSleeping ? 0.85f : 1.0f)
			* (1.0f + 1.2f * Exertion * (1.0f - 0.4f * Body.Fitness) + 0.5f * Hormones.Adrenaline + 0.15f * Hormones.Cortisol)
			+ 10.0f * Fever;
		Vitals.HeartRate = GenesisMath::ApproachExponential(Vitals.HeartRate, HeartTarget, 0.8f, Hours);

		const float RespirationTarget = BaseRespiration * (bSleeping ? 0.8f : 1.0f) * (1.0f + 1.5f * Exertion + 0.4f * Hormones.Adrenaline + RespiratoryDistress) / (0.5f + 0.5f * LungFunction);
		Vitals.RespiratoryRate = GenesisMath::ApproachExponential(Vitals.RespiratoryRate, RespirationTarget, 0.8f, Hours);

		Vitals.BloodOxygen = FMath::Clamp(98.0f - 12.0f * (1.0f - LungFunction) - 5.0f * Exertion * (1.0f - LungFunction) - 15.0f * RespiratoryDistress, 70.0f, 100.0f);
		Vitals.BodyTemperature = 36.8f + 0.3f * FMath::Sin((HourOfDay - 10) / 24.0f * UE_TWO_PI) + 2.5f * Fever + 0.5f * FMath::Max(0.0f, Exertion - 0.6f);

		// --- Schlaf, Hunger, Durst, Energie ---
		const float HoursF = static_cast<float>(Hours);
		if (bSleeping)
		{
			Vitals.SleepPressure = FMath::Max(0.0f, Vitals.SleepPressure - HoursF / 8.0f);
			Body.SleptHoursToday += HoursF;
		}
		else
		{
			Vitals.SleepPressure = FMath::Min(1.2f, Vitals.SleepPressure + HoursF / 16.0f);
		}
		Vitals.Hunger = FMath::Clamp(Vitals.Hunger + HoursF / (bSleeping ? 10.0f : 6.0f), 0.0f, 1.0f);
		Vitals.Hydration = FMath::Clamp(Vitals.Hydration - HoursF / (bSleeping ? 16.0f : 10.0f), 0.0f, 1.0f);

		const float EnergyTarget = FMath::Clamp(1.0f - 0.6f * FMath::Max(0.0f, Vitals.SleepPressure - 0.3f) - 0.3f * Vitals.Hunger - 0.3f * (1.0f - Vitals.Hydration) - 0.5f * Exertion, 0.0f, 1.0f);
		Vitals.Energy = GenesisMath::ApproachExponential(Vitals.Energy, EnergyTarget, 0.5f, Hours);

		Body.TrainingLoad += Exertion * HoursF * (Exertion >= 0.6f ? 1.0f : 0.3f);
		Body.HoursSinceDailyUpdate += HoursF;
	}

	bool AdvanceDays(FGenesisBodyState& Body, const FGenesisTimestamp& End, double Days, float PsychologicalStress, const FGenesisBodyTuning& Tuning)
	{
		if (!Body.bAlive || Days <= 0.0)
		{
			return false;
		}

		// Lange Zeiträume in Monatsscheiben – Entwicklung, Alterung und Zustände bleiben stetig
		const double SliceDays = 30.0;
		double Remaining = Days;
		FGenesisTimestamp SliceEnd = End - FGenesisTimestamp::DaysToSeconds(Days);

		while (Remaining > 0.0)
		{
			const double Step = FMath::Min(Remaining, SliceDays);
			Remaining -= Step;
			SliceEnd += FGenesisTimestamp::DaysToSeconds(Step);
			const float StepF = static_cast<float>(Step);

			UpdateDevelopment(Body, SliceEnd);

			if (!Body.bBorn)
			{
				UpdateCapacities(Body);
				UpdateSenses(Body, SliceEnd);
				UpdateGrowth(Body, SliceEnd);
				continue;
			}

			const double Age = GetAgeYears(Body, SliceEnd);

			// --- Schlafschuld (Level 2 ohne Stundenschritt: ausreichender Schlaf angenommen) ---
			const float Required = GetRequiredSleepHours(Age);
			if (Body.SimulationLevel == EGenesisSimulationLevel::Full && Step < 2.0)
			{
				Body.SleepDebtHours = FMath::Clamp(Body.SleepDebtHours + (Required * StepF - Body.SleptHoursToday), 0.0f, 100.0f);
			}
			else
			{
				Body.SleepDebtHours = FMath::Max(0.0f, Body.SleepDebtHours - 2.0f * StepF);
			}
			Body.SleptHoursToday = 0.0f;

			// --- Chronischer Stress, Fitness ---
			const float StressTarget = FMath::Clamp(0.7f * PsychologicalStress + 0.3f * FMath::Min(1.0f, Body.SleepDebtHours / 40.0f), 0.0f, 1.0f);
			Body.ChronicStress = GenesisMath::ApproachExponential(Body.ChronicStress, StressTarget, 0.05f, Step);

			const float FitnessTarget = FMath::Clamp(Body.TrainingLoad / 4.0f, 0.05f, 1.0f);
			Body.Fitness = GenesisMath::ApproachExponential(Body.Fitness, FitnessTarget, 0.03f, Step);
			Body.TrainingLoad *= FMath::Pow(0.85f, StepF);

			// --- Biologische Alterung ---
			float AgingRate = 1.0f
				+ Tuning.StressAgingFactor * Body.ChronicStress
				+ Tuning.SleepDebtAgingFactor * FMath::Min(1.0f, Body.SleepDebtHours / 40.0f)
				- 0.1f * (Body.NutritionQuality - 0.5f);
			if (Age >= 18.0)
			{
				AgingRate -= Tuning.FitnessAgingReduction * Body.Fitness;
			}
			Body.BiologicalAgeYears += Step / 365.0 * FMath::Max(0.5f, AgingRate);

			// --- Zustände: Immunantwort gegen Verlauf ---
			const float ImmuneFunction = Body.Organ(EGenesisOrgan::Immune).GetFunction();
			for (int32 Index = Body.Conditions.Num() - 1; Index >= 0; --Index)
			{
				FGenesisBodyCondition& Condition = Body.Conditions[Index];
				Condition.Severity = FMath::Clamp(Condition.Severity + (Condition.ProgressionPerDay - Condition.RecoveryPerDay * ImmuneFunction) * StepF, 0.0f, 1.0f);
				Condition.PeakSeverity = FMath::Max(Condition.PeakSeverity, Condition.Severity);

				FGenesisOrganState& Organ = Body.Organ(Condition.AffectedOrgan);
				Organ.Damage = FMath::Max(Organ.Damage, 0.5f * Condition.Severity);

				if (Condition.bChronic)
				{
					Condition.Severity = FMath::Max(Condition.Severity, 0.05f);
				}
				else if (Condition.Severity <= 0.0f)
				{
					if (Condition.bCanScar && Condition.PeakSeverity > 0.4f)
					{
						FGenesisScar& Scar = Body.Scars.AddDefaulted_GetRef();
						Scar.Region = Condition.Region;
						Scar.Visibility = FMath::Clamp(Condition.PeakSeverity, 0.0f, 1.0f);
						Scar.Acquired = SliceEnd;
					}
					Body.Conditions.RemoveAt(Index);
				}
			}

			// --- Jahrelanger Stress verschleißt das Herz dauerhaft, akute Schäden heilen ---
			FGenesisOrganState& Heart = Body.Organ(EGenesisOrgan::Heart);
			Heart.Wear = FMath::Clamp(Heart.Wear + Tuning.StressHeartWearPerDay * Body.ChronicStress * (1.0f + 2.0f * Body.Genetics.CardioRisk) * StepF, 0.0f, 1.0f);
			for (FGenesisOrganState& Organ : Body.Organs)
			{
				Organ.Damage = FMath::Max(0.0f, Organ.Damage - Tuning.DamageRepairPerDay * ImmuneFunction * StepF);
			}

			UpdateCapacities(Body);
			UpdateSenses(Body, SliceEnd);
			UpdateGrowth(Body, SliceEnd);
			UpdateSexHormones(Body, static_cast<float>(Age));

			// --- Vitalfunktionen ---
			const bool bFailure = Body.Organ(EGenesisOrgan::Heart).GetHealth() < Tuning.VitalFailureThreshold
				|| Body.Organ(EGenesisOrgan::Brain).GetHealth() < Tuning.VitalFailureThreshold
				|| Body.Organ(EGenesisOrgan::Lungs).GetHealth() < Tuning.VitalFailureThreshold;
			if (bFailure)
			{
				Body.bAlive = false;
				Body.DeathTime = SliceEnd;
				Body.Vitals.HeartRate = 0.0f;
				Body.Vitals.RespiratoryRate = 0.0f;
				return true;
			}
		}

		Body.HoursSinceDailyUpdate = 0.0f;
		return false;
	}

	void ApplyInjury(FGenesisBodyState& Body, const FGameplayTag& Region, float Severity, const FGenesisTimestamp& Now)
	{
		FGenesisBodyCondition& Injury = Body.Conditions.AddDefaulted_GetRef();
		Injury.Condition = GenesisBodyTags::Condition_Injury;
		Injury.Region = Region;
		Injury.AffectedOrgan = Region == GenesisBodyTags::Region_Head ? EGenesisOrgan::Brain : EGenesisOrgan::Musculature;
		Injury.Severity = FMath::Clamp(Severity, 0.0f, 1.0f);
		Injury.PeakSeverity = Injury.Severity;
		Injury.RecoveryPerDay = 0.03f;
		Injury.bCausesPain = true;
		Injury.bCanScar = true;
		Injury.Onset = Now;

		Body.Hormones.Adrenaline = FMath::Max(Body.Hormones.Adrenaline, 0.5f + 0.5f * Injury.Severity);
	}

	void ApplyIllness(FGenesisBodyState& Body, const FGameplayTag& Condition, float Severity, float ProgressionPerDay, float RecoveryPerDay,
		bool bChronic, bool bCausesFever, EGenesisOrgan AffectedOrgan, const FGenesisTimestamp& Now)
	{
		FGenesisBodyCondition& Illness = Body.Conditions.AddDefaulted_GetRef();
		Illness.Condition = Condition;
		Illness.AffectedOrgan = AffectedOrgan;
		Illness.Severity = FMath::Clamp(Severity, 0.0f, 1.0f);
		Illness.PeakSeverity = Illness.Severity;
		Illness.ProgressionPerDay = ProgressionPerDay;
		Illness.RecoveryPerDay = RecoveryPerDay;
		Illness.bChronic = bChronic;
		Illness.bCausesFever = bCausesFever;
		Illness.bCausesPain = bCausesFever;
		Illness.Onset = Now;
	}

	void ApplyAcuteStressor(FGenesisBodyState& Body, float Intensity)
	{
		Body.Hormones.Adrenaline = FMath::Clamp(Body.Hormones.Adrenaline + Intensity, 0.0f, 1.0f);
		Body.Hormones.Cortisol = FMath::Clamp(Body.Hormones.Cortisol + 0.3f * Intensity, 0.0f, 1.0f);
	}

	void ApplyBonding(FGenesisBodyState& Body, float Intensity)
	{
		Body.Hormones.Oxytocin = FMath::Clamp(Body.Hormones.Oxytocin + Intensity, 0.0f, 1.0f);
		// Nähe beruhigt
		Body.Hormones.Cortisol = FMath::Clamp(Body.Hormones.Cortisol - 0.2f * Intensity, 0.0f, 1.0f);
	}

	void Eat(FGenesisBodyState& Body, float Quality)
	{
		Body.Vitals.Hunger = 0.0f;
		Body.NutritionQuality = FMath::Lerp(Body.NutritionQuality, FMath::Clamp(Quality, 0.0f, 1.0f), 0.1f);
	}

	void Drink(FGenesisBodyState& Body)
	{
		Body.Vitals.Hydration = 1.0f;
	}

	void RecordExercise(FGenesisBodyState& Body, float Hours, float Intensity)
	{
		Body.TrainingLoad += FMath::Max(0.0f, Hours) * FMath::Clamp(Intensity, 0.0f, 1.0f);
	}

	TArray<FGenesisSymptom> DeriveSymptoms(const FGenesisBodyState& Body, const FGenesisTimestamp& Now)
	{
		TArray<FGenesisSymptom> Symptoms;
		if (!Body.bAlive)
		{
			return Symptoms;
		}

		auto Add = [&Symptoms](const FGameplayTag& Tag, float Intensity)
		{
			const float Clamped = FMath::Clamp(Intensity, 0.0f, 1.0f);
			if (Clamped > 0.05f)
			{
				FGenesisSymptom& Symptom = Symptoms.AddDefaulted_GetRef();
				Symptom.Symptom = Tag;
				Symptom.Intensity = Clamped;
			}
		};

		// Sinne gibt es schon im Mutterleib – gedämpft
		Add(GenesisBodyTags::Symptom_BlurredVision, 1.0f - Body.Sense(EGenesisBodySense::Sight).Acuity);
		Add(GenesisBodyTags::Symptom_MuffledHearing, 1.0f - Body.Sense(EGenesisBodySense::Hearing).Acuity);

		if (!Body.bBorn)
		{
			return Symptoms;
		}

		const double Age = GetAgeYears(Body, Now);
		const FGenesisVitalState& Vitals = Body.Vitals;
		const float BaseHeartRate = GetBaselineHeartRate(Age);

		Add(GenesisBodyTags::Symptom_Fatigue, FMath::Max(2.0f * (Vitals.SleepPressure - 0.5f), 1.0f - Vitals.Energy - 0.2f));
		Add(GenesisBodyTags::Symptom_HeartPounding, FMath::Max((Vitals.HeartRate / BaseHeartRate - 1.4f), Body.Hormones.Adrenaline - 0.4f));
		Add(GenesisBodyTags::Symptom_Breathlessness, FMath::Max((Vitals.RespiratoryRate / GetBaselineRespiratoryRate(Age) - 1.8f) / 1.5f, (93.0f - Vitals.BloodOxygen) / 12.0f));
		Add(GenesisBodyTags::Symptom_Pain, MaxConditionSeverity(Body, false, true));
		Add(GenesisBodyTags::Symptom_Fever, (Vitals.BodyTemperature - 37.6f) / 2.0f);
		Add(GenesisBodyTags::Symptom_Hunger, (Vitals.Hunger - 0.5f) * 2.0f);
		Add(GenesisBodyTags::Symptom_Thirst, (0.5f - Vitals.Hydration) * 2.0f);
		Add(GenesisBodyTags::Symptom_Tremor, FMath::Max(Body.Hormones.Adrenaline - 0.6f, (36.0f - Vitals.BodyTemperature) / 2.0f) * 2.0f);
		Add(GenesisBodyTags::Symptom_Dizziness, FMath::Max((90.0f - Vitals.BloodOxygen) / 10.0f, (0.2f - Vitals.Hydration) * 5.0f));
		Add(GenesisBodyTags::Symptom_Stiffness, (0.6f - FMath::Min(Body.Organ(EGenesisOrgan::Musculature).Capacity, Body.Organ(EGenesisOrgan::Skeleton).Capacity)) * 2.0f * (Age > 40.0 ? 1.0f : 0.0f));

		float Infection = 0.0f;
		for (const FGenesisBodyCondition& Condition : Body.Conditions)
		{
			if (Condition.Condition.MatchesTag(GenesisBodyTags::Condition_Infection))
			{
				Infection = FMath::Max(Infection, Condition.Severity);
			}
		}
		Add(GenesisBodyTags::Symptom_Nausea, (Infection - 0.4f) * 2.0f);

		return Symptoms;
	}

	float GetSymptomIntensity(const TArray<FGenesisSymptom>& Symptoms, const FGameplayTag& Symptom)
	{
		for (const FGenesisSymptom& Entry : Symptoms)
		{
			if (Entry.Symptom == Symptom)
			{
				return Entry.Intensity;
			}
		}
		return 0.0f;
	}

	float GetBaselineHeartRate(double AgeYears)
	{
		// Neugeborene ~140, Erwachsene ~70
		return 70.0f + 70.0f * FMath::Exp(-static_cast<float>(AgeYears) / 2.5f);
	}

	float GetBaselineRespiratoryRate(double AgeYears)
	{
		// Neugeborene ~45, Erwachsene ~14
		return 14.0f + 31.0f * FMath::Exp(-static_cast<float>(AgeYears) / 2.0f);
	}

	float GetRequiredSleepHours(double AgeYears)
	{
		if (AgeYears < 0.25) return 16.0f;
		if (AgeYears < 1.0) return 14.0f;
		if (AgeYears < 3.0) return 12.0f;
		if (AgeYears < 13.0) return 10.0f;
		if (AgeYears < 18.0) return 9.0f;
		if (AgeYears < 65.0) return 8.0f;
		return 7.0f;
	}
}
