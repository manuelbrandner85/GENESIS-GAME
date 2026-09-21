// GENESIS: Der Kreislauf des Lebens

#include "GenesisEarlyLifeLogic.h"
#include "GenesisRandom.h"

namespace GenesisEarlyLifeLogic
{
	namespace
	{
		constexpr double StepMinutes = 0.1;
	}

	FGenesisNewbornState BeginNewborn(const FGuid& EntityId, const FGuid& MotherId, uint64 Seed, const FGenesisTimestamp& BirthTime)
	{
		FGenesisNewbornState State;
		State.EntityId = EntityId;
		State.MotherId = MotherId;
		State.Seed = Seed;
		State.BirthTime = BirthTime;
		State.Stage = EGenesisNewbornStage::FirstBreaths;
		// Im Mutterleib ist es ein halbes Grad wärmer als in der Mutter selbst
		State.BodyTemperature = 37.2f;
		State.Calm = 0.15f;
		State.VisualAcuity = 0.04f;
		return State;
	}

	bool Advance(FGenesisNewbornState& State, const FGenesisEarlyLifeTuning& Tuning, double Minutes)
	{
		const EGenesisNewbornStage StartStage = State.Stage;
		if (State.Stage == EGenesisNewbornStage::NotBorn)
		{
			return false;
		}

		double Remaining = FMath::Max(0.0, Minutes);
		// Sechs Sekunden feine Schritte sind genau genug – aber ein Sprung über Tage (Weltuhr, Ladevorgang)
		// darf daraus keine Millionen Durchläufe machen. Deshalb wächst der Schritt bei großen Sprüngen mit.
		const double MaxStep = FMath::Max(StepMinutes, Remaining / 2000.0);
		while (Remaining > 0.0)
		{
			const double Step = FMath::Min(MaxStep, Remaining);
			Remaining -= Step;
			const float Delta = static_cast<float>(Step);
			State.MinutesSinceBirth += Step;

			// 1. Wärme. Das ist in der ersten Stunde die Hauptsache: Ein Neugeborenes hat viel Oberfläche
			// und wenig Masse, es ist nass und kann nicht zittern.
			// Nach dem Newtonschen Abkühlungsgesetz: Die Änderung folgt dem Gefälle, nicht der Uhr.
			// Deshalb fällt die Temperatur anfangs steil und flacht dann ab – und das Aufwärmen dauert länger,
			// je näher das Kind der Hauttemperatur der Mutter kommt.
			const float Ambient = State.bSkinToSkin ? Tuning.TargetTemperature : Tuning.RoomTemperature;
			const float Rate = State.bSkinToSkin ? Tuning.SkinContactRatePerMinute : Tuning.CoolingRatePerMinute;
			State.BodyTemperature += (Ambient - State.BodyTemperature) * (1.0f - FMath::Exp(-Rate * Delta));

			// 2. Ruhe: Wärme und Stimme beruhigen, Kälte und Hunger tun das Gegenteil
			const float Cold = FMath::Clamp((36.5f - State.BodyTemperature) / 1.5f, 0.0f, 1.0f);
			float CalmChange = State.bSkinToSkin ? Tuning.CalmGainPerMinute : -Tuning.CalmLossPerMinute;
			if (State.bMotherSpeaking)
			{
				CalmChange += Tuning.VoiceCalmPerMinute;
			}
			CalmChange -= Cold * Tuning.CalmLossPerMinute * 1.5f;
			CalmChange -= State.Hunger * Tuning.CalmLossPerMinute;
			State.Calm = FMath::Clamp(State.Calm + CalmChange * Delta, 0.0f, 1.0f);

			if (State.IsCrying())
			{
				State.CryingMinutes += Delta;
			}

			// Schreien aus eigenem Antrieb: Es ruft die Welt, aber es kostet auch. Ein schreiendes Kind
			// atmet schneller, strampelt und verliert dabei Wärme – und ruhiger wird es davon nicht.
			if (State.CryEffort > 0.05f)
			{
				State.CalledMinutes += Delta * State.CryEffort;
				State.BodyTemperature -= 0.012f * State.CryEffort * Delta;
				State.Calm = FMath::Clamp(State.Calm - 0.02f * State.CryEffort * Delta, 0.0f, 1.0f);
			}

			// 3. Hunger – bis zum ersten Trinken
			if (!State.bHasFed)
			{
				State.Hunger = FMath::Clamp(State.Hunger + Tuning.HungerPerMinute * Delta, 0.0f, 1.0f);
			}
			else
			{
				State.Hunger = FMath::Clamp(State.Hunger - 0.05f * Delta, 0.0f, 1.0f);
			}

			// 4. Bindung. Sie wächst nicht mit der Zeit, sondern mit dem, was geschieht:
			// Haut, Stimme, Sattsein. Ein Kind allein im Wärmebett bindet sich an niemanden.
			float BondingRate = 0.0f;
			if (State.bSkinToSkin)
			{
				BondingRate += Tuning.BondingPerMinute;
			}
			if (State.bMotherSpeaking)
			{
				BondingRate += Tuning.BondingPerMinute * 0.6f;
			}
			if (State.bHasFed)
			{
				BondingRate += Tuning.BondingPerMinute * 0.4f;
			}
			// Gegenseitiger Blick: In der ruhigen Wachheit sucht ein Neugeborenes Gesichter, und
			// eine Mutter, die den Blick erwidert, hält ihn. Er zählt nur, wo auch Haut ist.
			if (State.bEyeContact && State.bSkinToSkin)
			{
				BondingRate += Tuning.BondingPerMinute * Tuning.EyeContactBondingFactor;
			}
			// Sättigung: Eine perfekte erste Stunde legt eine Bindung an, sie vollendet sie nicht.
			// Ohne diese Bremse stünde nach einer halben Stunde 1.00 – und der Rest des Lebens
			// könnte nichts mehr dazu beitragen.
			State.Bonding = FMath::Clamp(State.Bonding + (1.0f - State.Bonding) * BondingRate * Delta, 0.0f, 1.0f);

			// 5. Sehschärfe: Sie nimmt in der ersten Stunde nur unmerklich zu – die Reifung braucht Monate
			State.VisualAcuity = FMath::Clamp(State.VisualAcuity + 0.0002f * Delta, 0.0f, 1.0f);

			// 6. Müdigkeit: Nach der ruhigen Wachheit wird das Kind schläfrig
			const float AlertWindow = Tuning.FirstBreathsMinutes + Tuning.QuietAlertMinutes;
			if (State.MinutesSinceBirth > AlertWindow * 0.6)
			{
				State.Sleepiness = FMath::Clamp(State.Sleepiness + 0.02f * Delta, 0.0f, 1.0f);
			}

			// 7. Stufen. Das eigene Suchen verkürzt den Weg zur Brust – aber nur auf der Haut,
			// denn dort ist überhaupt etwas zu suchen.
			const float RequiredMinutesToFeed = Tuning.MinutesToFirstFeed
				* (1.0f - Tuning.RootingSpeedUp * FMath::Clamp(State.RootingEffort, 0.0f, 1.0f));

			if (State.BodyTemperature <= Tuning.HypothermiaTemperature)
			{
				State.Stage = EGenesisNewbornStage::Hypothermic;
			}
			else if (State.Stage == EGenesisNewbornStage::Hypothermic && State.BodyTemperature > Tuning.HypothermiaTemperature + 0.4f)
			{
				// Wieder aufgewärmt: Das Kind ist zurück
				State.Stage = State.bHasFed ? EGenesisNewbornStage::FirstFeed : EGenesisNewbornStage::SkinContact;
			}
			else if (State.Stage == EGenesisNewbornStage::FirstBreaths && State.MinutesSinceBirth >= Tuning.FirstBreathsMinutes)
			{
				State.Stage = EGenesisNewbornStage::QuietAlert;
			}
			else if (State.Stage == EGenesisNewbornStage::QuietAlert && State.bSkinToSkin)
			{
				State.Stage = EGenesisNewbornStage::SkinContact;
			}
			else if (State.Stage == EGenesisNewbornStage::SkinContact && !State.bHasFed
				&& State.MinutesSinceBirth >= RequiredMinutesToFeed && State.Calm > 0.45f)
			{
				// Der Brustkrabbelgang: Das Kind findet die Brust von selbst, wenn man es lässt –
				// und schneller, wenn es sucht. Das ist seine eigene Leistung, nicht die der Mutter.
				State.bHasFed = true;
				State.Stage = EGenesisNewbornStage::FirstFeed;
			}
			else if (State.Stage == EGenesisNewbornStage::FirstFeed && State.Sleepiness > 0.6f
				&& State.Calm >= Tuning.SleepCalmThreshold)
			{
				State.Stage = EGenesisNewbornStage::FirstSleep;
			}
		}

		return State.Stage != StartStage;
	}

	FGenesisNewbornPerception GetPerception(const FGenesisNewbornState& State, const FGenesisEarlyLifeTuning& Tuning)
	{
		FGenesisNewbornPerception Perception;
		Perception.VisualAcuity = State.VisualAcuity;
		// Ein Neugeborenes sieht auf etwa 25 cm scharf – die Entfernung zum Gesicht auf dem Arm
		Perception.FocusDistanceMm = 250.0f;
		// Die erste Minute blendet; danach gewöhnt sich das Auge
		Perception.Glare = FMath::Clamp(1.0f - static_cast<float>(State.MinutesSinceBirth) / 6.0f, 0.15f, 1.0f);
		Perception.Warmth = FMath::Clamp((State.BodyTemperature - Tuning.HypothermiaTemperature)
			/ FMath::Max(0.1f, Tuning.TargetTemperature - Tuning.HypothermiaTemperature), 0.0f, 1.0f);
		Perception.Calm = State.Calm;
		Perception.Hunger = State.Hunger;
		Perception.Sleepiness = State.Sleepiness;
		// Die Stimme der Mutter kennt das Kind aus dem Mutterleib – sie ist von Anfang an vertraut,
		// und die Vertrautheit wächst mit der Bindung
		Perception.VoiceFamiliarity = State.bMotherSpeaking ? FMath::Clamp(0.55f + 0.45f * State.Bonding, 0.0f, 1.0f) : 0.0f;
		return Perception;
	}

	FString GetStageName(EGenesisNewbornStage Stage)
	{
		switch (Stage)
		{
		case EGenesisNewbornStage::FirstBreaths: return TEXT("erste Atemzüge");
		case EGenesisNewbornStage::QuietAlert: return TEXT("ruhige Wachheit");
		case EGenesisNewbornStage::SkinContact: return TEXT("Haut an Haut");
		case EGenesisNewbornStage::FirstFeed: return TEXT("erstes Anlegen");
		case EGenesisNewbornStage::FirstSleep: return TEXT("erster Schlaf");
		case EGenesisNewbornStage::Hypothermic: return TEXT("unterkühlt");
		default: return TEXT("noch nicht geboren");
		}
	}
}
