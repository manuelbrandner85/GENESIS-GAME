// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceLogic.h"

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
}
