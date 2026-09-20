// GENESIS: Der Kreislauf des Lebens

#include "GenesisBirthLogic.h"
#include "GenesisRandom.h"

namespace GenesisBirthLogic
{
	namespace
	{
		/** Fester Rechenschritt (Sekunden). Fein genug für den Verlauf einer einzelnen Wehe. */
		constexpr double StepSeconds = 2.0;

		struct FStageProfile
		{
			float IntervalSeconds;
			float DurationSeconds;
			float Intensity;
			float DilationPerHour;
		};

		FStageProfile GetProfile(const FGenesisBirthState& State, const FGenesisBirthTuning& Tuning)
		{
			switch (State.Stage)
			{
			case EGenesisLaborStage::Latent:
				return { Tuning.LatentIntervalSeconds, Tuning.LatentDurationSeconds, Tuning.LatentIntensity, Tuning.LatentDilationPerHour };
			case EGenesisLaborStage::Active:
				return { Tuning.ActiveIntervalSeconds, Tuning.ActiveDurationSeconds, Tuning.ActiveIntensity, Tuning.ActiveDilationPerHour };
			case EGenesisLaborStage::Transition:
				return { Tuning.TransitionIntervalSeconds, Tuning.TransitionDurationSeconds, Tuning.TransitionIntensity, Tuning.TransitionDilationPerHour };
			case EGenesisLaborStage::Pushing:
				return { Tuning.PushingIntervalSeconds, Tuning.PushingDurationSeconds, Tuning.PushingIntensity, 0.0f };
			default:
				return { 600.0f, 30.0f, 0.0f, 0.0f };
			}
		}

		/**
		 * Verlauf einer einzelnen Wehe: Sie baut sich auf, hält, und lässt langsamer nach, als sie gekommen ist.
		 * Eine Sinuswelle wäre symmetrisch – eine Wehe ist es nicht.
		 */
		float ContractionCurve(float Phase)
		{
			if (Phase <= 0.0f || Phase >= 1.0f)
			{
				return 0.0f;
			}
			const float Rise = FMath::SmoothStep(0.0f, 0.32f, Phase);
			const float Fall = 1.0f - FMath::SmoothStep(0.45f, 1.0f, Phase);
			return FMath::Clamp(Rise * Fall, 0.0f, 1.0f);
		}

		EGenesisBirthComplication PickComplication(const FGenesisBirthState& State, const FGenesisBirthTuning& Tuning)
		{
			FGenesisRandomStream Rng(GenesisHash::Combine(State.Seed, 0xB1877ull));
			// Frühgeburt und schwache Lungenreife erhöhen das Risiko – das ist kein Würfeln, sondern Folge
			const float Prematurity = FMath::Clamp((40.0f - State.GestationalWeeks) / 6.0f, 0.0f, 1.0f);
			const float Risk = FMath::Clamp(Tuning.ComplicationChance * (1.0f + 1.2f * Prematurity), 0.0f, 1.0f);
			if (!Rng.Bernoulli(Risk))
			{
				return EGenesisBirthComplication::None;
			}

			const float Roll = Rng.NextFloat();
			if (Roll < 0.42f)
			{
				return EGenesisBirthComplication::CordCompression;
			}
			if (Roll < 0.68f)
			{
				return EGenesisBirthComplication::StalledLabor;
			}
			if (Roll < 0.88f)
			{
				return EGenesisBirthComplication::Breech;
			}
			return EGenesisBirthComplication::ShoulderDystocia;
		}
	}

	FGenesisBirthState BeginLabor(const FGuid& EntityId, uint64 Seed, float GestationalWeeks, float LungMaturity,
		const FGenesisTimestamp& Now, const FGenesisBirthTuning& Tuning)
	{
		FGenesisBirthState State;
		State.EntityId = EntityId;
		State.Seed = Seed;
		State.GestationalWeeks = GestationalWeeks;
		State.LungMaturity = FMath::Clamp(LungMaturity, 0.0f, 1.0f);
		State.Stage = EGenesisLaborStage::Latent;
		State.BirthTime = Now;
		State.Oxygen = 1.0f;
		State.HeartRateBpm = 140.0f;
		State.Complication = PickComplication(State, Tuning);
		return State;
	}

	bool Advance(FGenesisBirthState& State, const FGenesisBirthTuning& Tuning, double Minutes)
	{
		const EGenesisLaborStage StartStage = State.Stage;
		double RemainingSeconds = FMath::Max(0.0, Minutes) * 60.0;

		while (RemainingSeconds > 0.0)
		{
			const double Step = FMath::Min(StepSeconds, RemainingSeconds);
			RemainingSeconds -= Step;
			const float StepMinutes = static_cast<float>(Step) / 60.0f;

			if (State.Stage == EGenesisLaborStage::NotStarted)
			{
				continue;
			}

			if (State.Stage == EGenesisLaborStage::Delivered)
			{
				// Nach der Geburt: Der erste Atemzug kommt nicht sofort, und danach steigt der Sauerstoff schnell
				State.SecondsSinceBirth += static_cast<float>(Step);
				if (!State.bFirstBreath && State.SecondsSinceBirth >= Tuning.FirstBreathSeconds * (2.0f - State.LungMaturity))
				{
					State.bFirstBreath = true;
				}
				if (State.bFirstBreath)
				{
					State.Oxygen = FMath::Clamp(State.Oxygen + 1.2f * StepMinutes * State.LungMaturity, 0.0f, 1.0f);
					State.HeartRateBpm = FMath::FInterpTo(State.HeartRateBpm, 130.0f, StepMinutes, 3.0f);
				}
				else
				{
					State.Oxygen = FMath::Clamp(State.Oxygen - 0.25f * StepMinutes, 0.0f, 1.0f);
				}
				State.ApgarScore = ComputeApgar(State);
				continue;
			}

			State.MinutesInLabor += StepMinutes;
			const FStageProfile Profile = GetProfile(State, Tuning);

			// 1. Wehenzyklus: Die Uhr läuft von Wehenbeginn zu Wehenbeginn
			State.CycleSeconds += static_cast<float>(Step);
			if (State.CycleSeconds >= Profile.IntervalSeconds)
			{
				State.CycleSeconds -= Profile.IntervalSeconds;
				++State.ContractionCount;
			}
			const float Phase = State.CycleSeconds / FMath::Max(1.0f, Profile.DurationSeconds);
			State.ContractionIntensity = ContractionCurve(Phase) * Profile.Intensity;

			// 2. Fortschritt – nur unter der Wehe. Zwischen den Wehen geschieht nichts:
			// Die Gebärmutter arbeitet in Schüben, nicht gleichmäßig.
			//
			// Damit die angegebenen cm je Stunde trotzdem stimmen, wird der Fortschritt auf den Anteil
			// der Wehenzeit am Zyklus hochgerechnet (eine Wehe trägt im Mittel etwa 45 Prozent ihrer Spitzenkraft).
			const float RelativeForce = State.ContractionIntensity / FMath::Max(0.01f, Profile.Intensity);
			const float DutyCycle = FMath::Max(0.02f, Profile.DurationSeconds / FMath::Max(1.0f, Profile.IntervalSeconds) * 0.45f);

			if (State.Stage == EGenesisLaborStage::Pushing)
			{
				const float Resistance = State.Complication == EGenesisBirthComplication::Breech ? 0.55f
					: (State.Complication == EGenesisBirthComplication::ShoulderDystocia && State.Descent > 0.72f ? 0.18f : 1.0f);
				// Tiefertreten je Presswehe, verteilt über ihre Dauer
				const float PerMinute = Tuning.DescentPerContraction * 60.0f / FMath::Max(1.0f, Profile.IntervalSeconds) / DutyCycle;
				State.Descent = FMath::Clamp(State.Descent + RelativeForce * PerMinute * StepMinutes * Resistance, 0.0f, 1.0f);
				// Das Kind dreht sich im Becken, um mit dem schmalsten Durchmesser durchzupassen
				State.Rotation = FMath::Clamp(State.Descent * 1.25f, 0.0f, 1.0f);
			}
			else
			{
				const float Stall = State.Complication == EGenesisBirthComplication::StalledLabor
					&& State.DilationCm > 4.0f && State.DilationCm < 7.0f ? 0.25f : 1.0f;
				State.DilationCm = FMath::Clamp(State.DilationCm + Profile.DilationPerHour * StepMinutes / 60.0f
					* RelativeForce / DutyCycle * Stall, 0.0f, 10.0f);
			}

			// 3. Sauerstoff: Unter der Wehe wird der Mutterkuchen schlechter durchblutet
			const float CordFactor = State.Complication == EGenesisBirthComplication::CordCompression ? 1.7f : 1.0f;
			if (State.ContractionIntensity > 0.05f)
			{
				State.Oxygen = FMath::Clamp(State.Oxygen - Tuning.OxygenDipPerMinute * State.ContractionIntensity * CordFactor * StepMinutes, 0.0f, 1.0f);
			}
			else
			{
				const float Recovery = Tuning.OxygenRecoveryPerMinute / CordFactor;
				State.Oxygen = FMath::Clamp(State.Oxygen + Recovery * StepMinutes, 0.0f, 1.0f);
			}

			if (State.Oxygen < Tuning.HypoxiaThreshold)
			{
				State.HypoxiaMinutes += StepMinutes;
				State.Stress = FMath::Clamp(State.Stress + StepMinutes / FMath::Max(1.0f, Tuning.HypoxiaBudgetMinutes), 0.0f, 1.0f);
			}
			else
			{
				State.Stress = FMath::Clamp(State.Stress - 0.12f * StepMinutes, 0.0f, 1.0f);
			}

			// 4. Herzschlag: Er fällt mit dem Sauerstoff ab und erholt sich danach – das ist die Kurve,
			// die im Kreißsaal auf dem Monitor läuft
			const float TargetHeartRate = FMath::Lerp(85.0f, 145.0f, FMath::Clamp((State.Oxygen - 0.35f) / 0.55f, 0.0f, 1.0f));
			State.HeartRateBpm = FMath::FInterpTo(State.HeartRateBpm, TargetHeartRate, StepMinutes, 6.0f);

			// 5. Abschnittswechsel
			if (State.Stage == EGenesisLaborStage::Latent && State.DilationCm >= 3.0f)
			{
				State.Stage = EGenesisLaborStage::Active;
			}
			else if (State.Stage == EGenesisLaborStage::Active && State.DilationCm >= 7.0f)
			{
				State.Stage = EGenesisLaborStage::Transition;
			}
			else if (State.Stage == EGenesisLaborStage::Transition && State.DilationCm >= 10.0f)
			{
				State.Stage = EGenesisLaborStage::Pushing;
				State.CycleSeconds = 0.0f;
			}
			else if (State.Stage == EGenesisLaborStage::Pushing && State.Descent >= 1.0f)
			{
				State.Stage = EGenesisLaborStage::Delivered;
				State.ContractionIntensity = 0.0f;
				State.SecondsSinceBirth = 0.0f;
				State.ApgarScore = ComputeApgar(State);
			}
		}

		return State.Stage != StartStage;
	}

	FGenesisBirthPerception GetPerception(const FGenesisBirthState& State)
	{
		FGenesisBirthPerception Perception;
		Perception.Oxygen = State.Oxygen;
		Perception.HeartRateBpm = State.HeartRateBpm;

		if (State.Stage == EGenesisLaborStage::NotStarted)
		{
			Perception.SoundMuffling = 1.0f;
			return Perception;
		}

		if (State.IsBorn())
		{
			// Draußen: Licht, klare Luft, Kälte. Der Übergang ist ein Sprung, kein weicher Verlauf.
			const float Since = FMath::Clamp(State.SecondsSinceBirth / 6.0f, 0.0f, 1.0f);
			Perception.Light = Since;
			Perception.SoundMuffling = FMath::Lerp(0.55f, 0.05f, Since);
			Perception.Cold = FMath::Clamp(State.SecondsSinceBirth / 20.0f, 0.0f, 1.0f);
			Perception.Pressure = 0.0f;
			Perception.Tightness = 0.0f;
			return Perception;
		}

		Perception.Pressure = FMath::Clamp(State.ContractionIntensity, 0.0f, 1.0f);
		// Die Enge nimmt mit dem Tiefertreten zu: Der Kopf presst sich durch den knöchernen Ring
		Perception.Tightness = FMath::Clamp(0.25f * State.DilationCm / 10.0f + 0.75f * State.Descent, 0.0f, 1.0f);
		// Erst beim Durchtritt des Kopfes fällt Licht auf das Gesicht
		Perception.Light = FMath::Clamp((State.Descent - 0.82f) / 0.18f, 0.0f, 1.0f) * 0.7f;
		Perception.SoundMuffling = FMath::Lerp(1.0f, 0.6f, FMath::Clamp((State.Descent - 0.7f) / 0.3f, 0.0f, 1.0f));
		return Perception;
	}

	int32 ComputeApgar(const FGenesisBirthState& State)
	{
		// Angelehnt an das erste Zustandsbild nach der Geburt: Herzschlag, Atmung, Muskelspannung,
		// Reaktion und Hautfarbe – hier aus Sauerstoff, Belastung und Reife abgeleitet.
		const float HypoxiaLoad = FMath::Clamp(State.HypoxiaMinutes / 20.0f, 0.0f, 1.0f);

		int32 Score = 0;
		Score += State.HeartRateBpm >= 100.0f ? 2 : (State.HeartRateBpm > 0.0f ? 1 : 0);
		Score += State.bFirstBreath ? (State.LungMaturity > 0.8f ? 2 : 1) : 0;
		Score += State.Stress < 0.4f ? 2 : (State.Stress < 0.75f ? 1 : 0);
		Score += HypoxiaLoad < 0.35f ? 2 : (HypoxiaLoad < 0.7f ? 1 : 0);
		Score += State.Oxygen > 0.8f ? 2 : (State.Oxygen > 0.5f ? 1 : 0);
		return FMath::Clamp(Score, 0, 10);
	}

	FString GetStageName(EGenesisLaborStage Stage)
	{
		switch (Stage)
		{
		case EGenesisLaborStage::NotStarted: return TEXT("vor den Wehen");
		case EGenesisLaborStage::Latent: return TEXT("Eröffnung (früh)");
		case EGenesisLaborStage::Active: return TEXT("Eröffnung (aktiv)");
		case EGenesisLaborStage::Transition: return TEXT("Übergangsphase");
		case EGenesisLaborStage::Pushing: return TEXT("Austreibung");
		case EGenesisLaborStage::Delivered: return TEXT("geboren");
		default: return TEXT("unbekannt");
		}
	}

	FString GetComplicationName(EGenesisBirthComplication Complication)
	{
		switch (Complication)
		{
		case EGenesisBirthComplication::CordCompression: return TEXT("Nabelschnur unter Druck");
		case EGenesisBirthComplication::Breech: return TEXT("Steißlage");
		case EGenesisBirthComplication::ShoulderDystocia: return TEXT("Schulter bleibt hängen");
		case EGenesisBirthComplication::StalledLabor: return TEXT("Geburtsstillstand");
		default: return TEXT("keine");
		}
	}
}
