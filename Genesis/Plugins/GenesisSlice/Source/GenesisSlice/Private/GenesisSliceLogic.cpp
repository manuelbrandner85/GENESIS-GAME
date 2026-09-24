// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceLogic.h"

FGenesisSliceTuning::FGenesisSliceTuning()
	: GestationMoments(GenesisSliceLogic::DefaultGestationMoments())
{
}

namespace GenesisSliceLogic
{
	EGenesisSlicePhase NextPhase(EGenesisSlicePhase Current, const FGenesisSliceSignals& Signals,
		const FGenesisSliceTuning& Tuning, EGenesisSliceEnding& OutEnding)
	{
		OutEnding = EGenesisSliceEnding::None;

		// Ein Keim, der sich nicht weiterentwickelt, beendet den Durchlauf – in jeder Phase davor.
		// Das ist kein Fehlschlag des Spiels, sondern der häufigste Ausgang einer Befruchtung.
		if (Signals.bEmbryoArrested && Current != EGenesisSlicePhase::Complete && Current != EGenesisSlicePhase::Ended)
		{
			OutEnding = EGenesisSliceEnding::EmbryoArrested;
			return EGenesisSlicePhase::Ended;
		}

		// Ein Kind, das die Geburt nicht überlebt, ebenso
		if (!Signals.bAlive && (Current == EGenesisSlicePhase::Birth || Current == EGenesisSlicePhase::FirstHour))
		{
			OutEnding = EGenesisSliceEnding::NotAlive;
			return EGenesisSlicePhase::Ended;
		}

		switch (Current)
		{
		case EGenesisSlicePhase::Idle:
			return EGenesisSlicePhase::Conception;

		case EGenesisSlicePhase::Conception:
			// Erst wenn eine Zelle verschmolzen ist, gibt es überhaupt etwas zu begleiten
			return Signals.bConceived ? EGenesisSlicePhase::Embryo : EGenesisSlicePhase::Conception;

		case EGenesisSlicePhase::Embryo:
			// Die erste Woche endet mit der Einnistung – ab da führt die Körpersimulation
			// Erst wenn der Bauplan steht (Ende der vierten Woche), übernimmt die Körpersimulation
			return Signals.bBodyPlanDone ? EGenesisSlicePhase::Gestation : EGenesisSlicePhase::Embryo;

		case EGenesisSlicePhase::Gestation:
			// Neun Monate im Zeitraffer, bis das Kind am Termin ist
			return Signals.GestationalWeeks >= Tuning.BirthAtWeeks ? EGenesisSlicePhase::Birth : EGenesisSlicePhase::Gestation;

		case EGenesisSlicePhase::Birth:
			return Signals.bBorn ? EGenesisSlicePhase::FirstHour : EGenesisSlicePhase::Birth;

		case EGenesisSlicePhase::FirstHour:
			// Der Schluss: Das Kind schläft ein. Wer es nicht auf die Haut legt, bekommt trotzdem
			// ein Ende – nach der vorgesehenen Zeit, nur ohne den schönen Schluss.
			if (Signals.bAsleep)
			{
				OutEnding = EGenesisSliceEnding::Asleep;
				return EGenesisSlicePhase::Complete;
			}
			if (Signals.MinutesSinceBirth >= Tuning.FirstHourLimitMinutes)
			{
				// Ehrlich bleiben: Wer das Kind liegen lässt, bekommt ein Ende, aber nicht dieses Ende.
				OutEnding = EGenesisSliceEnding::Unsettled;
				return EGenesisSlicePhase::Complete;
			}
			return EGenesisSlicePhase::FirstHour;

		default:
			return Current;
		}
	}

	FString GetPhaseName(EGenesisSlicePhase Phase)
	{
		switch (Phase)
		{
		case EGenesisSlicePhase::Conception: return TEXT("Befruchtung");
		case EGenesisSlicePhase::Embryo: return TEXT("Die ersten vier Wochen");
		case EGenesisSlicePhase::Gestation: return TEXT("Schwangerschaft");
		case EGenesisSlicePhase::Birth: return TEXT("Geburt");
		case EGenesisSlicePhase::FirstHour: return TEXT("Erste Stunde");
		case EGenesisSlicePhase::Complete: return TEXT("Angekommen");
		case EGenesisSlicePhase::Ended: return TEXT("Beendet");
		default: return TEXT("Kein Durchlauf");
		}
	}

	FString GetEndingName(EGenesisSliceEnding Ending)
	{
		switch (Ending)
		{
		case EGenesisSliceEnding::Asleep: return TEXT("Das Kind schläft");
		case EGenesisSliceEnding::Unsettled: return TEXT("Die erste Stunde ist vorbei – das Kind ist nicht zur Ruhe gekommen");
		case EGenesisSliceEnding::EmbryoArrested: return TEXT("Der Keim hat sich nicht weiterentwickelt");
		case EGenesisSliceEnding::NotAlive: return TEXT("Das Kind hat die Geburt nicht überlebt");
		case EGenesisSliceEnding::Aborted: return TEXT("Abgebrochen");
		default: return TEXT("läuft");
		}
	}

	FString GetPhaseDescription(EGenesisSlicePhase Phase)
	{
		switch (Phase)
		{
		case EGenesisSlicePhase::Conception: return TEXT("Der Schwarm im Eileiter, eine Eizelle, ein Treffer.");
		case EGenesisSlicePhase::Embryo: return TEXT("Furchung, Einnistung, Keimblätter, Neuralrohr, der erste Herzschlag.");
		case EGenesisSlicePhase::Gestation: return TEXT("Neun Monate im Zeitraffer – der Körper wächst.");
		case EGenesisSlicePhase::Birth: return TEXT("Wehen, Enge, Drehung, Licht, der erste Atemzug.");
		case EGenesisSlicePhase::FirstHour: return TEXT("Wärme, eine vertraute Stimme, das erste Anlegen.");
		case EGenesisSlicePhase::Complete: return TEXT("Ein Mensch ist da.");
		default: return TEXT("");
		}
	}

	float EmbryoHoursPerSecond(EGenesisEmbryoStage Stage, const FGenesisSliceTuning& Tuning)
	{
		switch (Stage)
		{
		case EGenesisEmbryoStage::Zygote:
			return Tuning.ZygoteHoursPerSecond;
		case EGenesisEmbryoStage::Cleavage:
		case EGenesisEmbryoStage::Morula:
		case EGenesisEmbryoStage::Blastocyst:
		case EGenesisEmbryoStage::Hatching:
			return Tuning.CleavageHoursPerSecond;
		case EGenesisEmbryoStage::Implanting:
			return Tuning.ImplantationHoursPerSecond;
		default:
			// Eingenistet: die dritte und vierte Woche (Keimblätter, Neuralrohr, Herzschlag)
			return Tuning.BodyPlanHoursPerSecond;
		}
	}

	FName MapForEmbryoStage(EGenesisEmbryoStage Stage, const FGenesisSliceTuning& Tuning)
	{
		// Geschlüpft liegt der Keim in der Gebärmutter; eingenistet bleibt er dort. Ein Keim im Stillstand bleibt, wo er ist.
		const bool bUterus = Stage == EGenesisEmbryoStage::Implanting || Stage == EGenesisEmbryoStage::Implanted;
		return bUterus && !Tuning.ImplantationMap.IsNone() ? Tuning.ImplantationMap : Tuning.ConceptionMap;
	}

	FName MapForEmbryo(EGenesisEmbryoStage Stage, float DayPostFertilization, const FGenesisSliceTuning& Tuning)
	{
		// Eingenistet und alt genug: Der Blick geht in die Fruchthöhle, zum Embryo selbst
		if (Stage == EGenesisEmbryoStage::Implanted && DayPostFertilization >= Tuning.EmbryoSceneFromDay && !Tuning.EmbryoMap.IsNone())
		{
			return Tuning.EmbryoMap;
		}
		return MapForEmbryoStage(Stage, Tuning);
	}

	FString DescribeEmbryoStage(EGenesisEmbryoStage Stage)
	{
		switch (Stage)
		{
		case EGenesisEmbryoStage::Zygote:
			return TEXT("Zygote – nach etwa 8 h erscheinen zwei Vorkerne: das Erbgut von Mutter und Vater, noch getrennt. Nach etwa 23 h lösen sie sich auf und vereinen sich.");
		case EGenesisEmbryoStage::Cleavage:
			return TEXT("Furchung – die Zellen teilen sich, ohne zu wachsen: erst nach etwa 12 h, ab vier Zellen nach 15–20 h. Der Keim bleibt so groß wie die Eizelle.");
		case EGenesisEmbryoStage::Morula:
			return TEXT("Morula – ab Tag 4 verzahnen sich die Zellen zu einem dichten Verband. Seit dem 4- bis 8-Zell-Stadium arbeitet das eigene Erbgut.");
		case EGenesisEmbryoStage::Blastocyst:
			return TEXT("Blastozyste – innen sammelt sich Flüssigkeit. Außen der Trophoblast, später Mutterkuchen; innen der Embryoblast: daraus wirst du.");
		case EGenesisEmbryoStage::Hatching:
			return TEXT("Schlüpfen – die Hülle wird dünn und reißt, der Keim zwängt sich heraus.");
		case EGenesisEmbryoStage::Implanting:
			return TEXT("Einnistung – der Trophoblast dringt in die Schleimhaut der Gebärmutter ein.");
		case EGenesisEmbryoStage::Implanted:
			return TEXT("Eingenistet – jetzt entsteht der Bauplan des Körpers.");
		case EGenesisEmbryoStage::Arrested:
			return TEXT("Der Keim teilt sich nicht mehr.");
		default:
			return FString();
		}
	}

	TArray<FGenesisGestationMoment> DefaultGestationMoments()
	{
		// SSW, Uhrzeit, Echtzeit (s), Kapitelzeile. Nach der Wochentafel in Docs/34 – jeder Moment zeigt, was sich in
		// dieser Woche für das Kind verändert: der erste Ton (Signature Moment 4), die Mutter spürt das Kind, bewusstes
		// Erleben, Licht durch den Bauch (Signature Moment 5), Mittag, Stimmen, Enge. Aus Sicht des Kindes beginnt es mit
		// SSW 19: Davor nimmt es weder Ton noch Licht wahr – die frühen Wochen zeigt die Kamera von außen (Docs/37, Teil 2c).
		auto Moment = [](float Weeks, float Hour, float Seconds, const TCHAR* Title, const TCHAR* Subtitle,
			EGenesisGestationSituation Situation = EGenesisGestationSituation::AtHour)
		{
			FGenesisGestationMoment Entry;
			Entry.Situation = Situation;
			Entry.GestationalWeeks = Weeks;
			Entry.HourOfDay = Hour;
			Entry.Seconds = Seconds;
			Entry.Title = Title;
			Entry.Subtitle = Subtitle;
			Entry.bOnlyFromOutside = Weeks < 19.0f;
			return Entry;
		};
		return {
			// Was das Kind in diesen Wochen tut, nach Ultraschall-Beobachtungen (de Vries, Visser & Prechtl 1982): ab
			// SSW ~8 allgemeine Bewegungen und Schreck, ab 9–10 Schluckauf, Arme und Beine einzeln, Hand zum Gesicht,
			// ab 11 Gähnen, ab 12–13 Saugen und Schlucken. Die Mutter spürt davon erst ab SSW 18–20 etwas.
			Moment(10.0f, 11.0f, 20.0f, TEXT("Woche 10"), TEXT("Es bewegt sich – und niemand spürt es")),
			Moment(12.0f, 15.0f, 20.0f, TEXT("Woche 12"), TEXT("Es gähnt, schluckt, legt die Hand ans Gesicht")),
			Moment(16.0f, 21.0f, 24.0f, TEXT("Woche 16"), TEXT("Sie spricht mit ihm – hören kann es sie noch nicht"),
				EGenesisGestationSituation::BellyTalk),
			Moment(19.0f, 21.0f, 40.0f, TEXT("Woche 19"), TEXT("Das Hören beginnt"), EGenesisGestationSituation::BellyTalk),
			Moment(20.5f, 21.0f, 35.0f, TEXT("Woche 20"), TEXT("Sie spürt die ersten Bewegungen"), EGenesisGestationSituation::BellyTalk),
			Moment(24.0f, 3.0f, 30.0f, TEXT("Woche 24"), TEXT("Nachts: ihr Herz, ihr Atem")),
			Moment(28.0f, 17.0f, 40.0f, TEXT("Woche 28"), TEXT("Die Lider öffnen sich"), EGenesisGestationSituation::Walk),
			Moment(31.0f, 17.0f, 30.0f, TEXT("Woche 31"), TEXT("Licht durch den Bauch"), EGenesisGestationSituation::Walk),
			Moment(34.0f, 20.0f, 35.0f, TEXT("Woche 34"), TEXT("Das Hören wird feiner")),
			Moment(37.0f, 21.0f, 30.0f, TEXT("Woche 37"), TEXT("Es wird eng"), EGenesisGestationSituation::BellyTalk),
		};
	}

	namespace
	{

		double Ease(double T)
		{
			T = FMath::Clamp(T, 0.0, 1.0);
			return T * T * (3.0 - 2.0 * T);
		}
	}

	double MomentStartHours(const FGenesisGestationMoment& Moment, int32 Seed)
	{
		// Tag der SSW (Zählweise Docs/34: SSW − 2), dann die Uhrzeit
		const double FirstDay = FMath::FloorToDouble((static_cast<double>(Moment.GestationalWeeks) - 2.0) * 7.0);
		if (Moment.Situation == EGenesisGestationSituation::AtHour)
		{
			return FirstDay * 24.0 + static_cast<double>(Moment.HourOfDay);
		}
		// Im Tag der Mutter suchen: an diesem und den folgenden Tagen, in Schritten von drei Minuten
		const FGenesisMotherDayTuning MotherTuning;
		for (int32 Day = 0; Day < 7; ++Day)
		{
			const int32 DayIndex = static_cast<int32>(FirstDay) + Day;
			for (double Hour = 6.0; Hour < 24.0; Hour += 0.05)
			{
				const FGenesisMotherMoment Mother = GenesisMotherDay::Evaluate(MotherTuning, Hour, DayIndex,
					Moment.GestationalWeeks + Day / 7.0f, Seed);
				const bool bMatch = Moment.Situation == EGenesisGestationSituation::Walk
					? Mother.Activity == EGenesisMotherActivity::Walking : Mother.bTalkingToBelly;
				if (bMatch)
				{
					// kurz nach Beginn der Situation einsteigen
					return static_cast<double>(DayIndex) * 24.0 + Hour + 0.05;
				}
			}
		}
		return FirstDay * 24.0 + static_cast<double>(Moment.HourOfDay);
	}

	TArray<double> ResolveGestationMoments(const FGenesisSliceTuning& Tuning, int32 MotherSeed)
	{
		TArray<double> Starts;
		for (const FGenesisGestationMoment& Moment : Tuning.GestationMoments)
		{
			Starts.Add(MomentStartHours(Moment, MotherSeed));
		}
		return Starts;
	}

	float GestationPlanSeconds(const FGenesisSliceTuning& Tuning)
	{
		float Total = Tuning.GestationTravelSeconds;           // der letzte Zeitraffer zur Geburt
		for (const FGenesisGestationMoment& Moment : Tuning.GestationMoments)
		{
			Total += Tuning.GestationTravelSeconds + Moment.Seconds;
		}
		return Total;
	}

	FGenesisGestationPlanPoint EvaluateGestationPlan(const FGenesisSliceTuning& Tuning, const TArray<double>& MomentStarts, double StartHours,
		float ElapsedSeconds)
	{
		FGenesisGestationPlanPoint Point;
		double From = StartHours;
		float Clock = FMath::Max(0.0f, ElapsedSeconds);
		const float Travel = FMath::Max(0.1f, Tuning.GestationTravelSeconds);
		for (int32 Index = 0; Index < Tuning.GestationMoments.Num(); ++Index)
		{
			const FGenesisGestationMoment& Moment = Tuning.GestationMoments[Index];
			const double Target = FMath::Max(From, MomentStarts.IsValidIndex(Index) ? MomentStarts[Index] : From);
			if (Clock < Travel)
			{
				Point.Alpha = Clock / Travel;
				Point.HoursAfterConception = FMath::Lerp(From, Target, Ease(Point.Alpha));
				return Point;
			}
			Clock -= Travel;
			if (Clock < Moment.Seconds)
			{
				Point.MomentIndex = Index;
				Point.Alpha = Clock / FMath::Max(0.1f, Moment.Seconds);
				Point.HoursAfterConception = Target + Clock * Tuning.GestationMomentTimeScale / 3600.0;
				return Point;
			}
			Clock -= Moment.Seconds;
			From = Target + Moment.Seconds * Tuning.GestationMomentTimeScale / 3600.0;
		}
		const double Birth = FMath::Max(From, static_cast<double>(Tuning.BirthAtWeeks) * 7.0 * 24.0);
		Point.Alpha = FMath::Min(1.0f, Clock / Travel);
		Point.HoursAfterConception = FMath::Lerp(From, Birth, Ease(Point.Alpha));
		Point.bFinished = Clock >= Travel;
		return Point;
	}
}
