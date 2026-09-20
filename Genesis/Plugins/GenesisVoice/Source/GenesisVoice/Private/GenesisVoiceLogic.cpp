// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceLogic.h"
#include "GenesisRandom.h"

namespace GenesisVoiceLogic
{
	namespace
	{
		/** Stützstellen einer Kurve über dem Alter. */
		struct FAgePoint
		{
			float AgeYears;
			float Value;
		};

		float SampleCurve(const FAgePoint* Points, int32 Count, float AgeYears)
		{
			if (Count <= 0)
			{
				return 0.0f;
			}
			if (AgeYears <= Points[0].AgeYears)
			{
				return Points[0].Value;
			}
			for (int32 Index = 1; Index < Count; ++Index)
			{
				if (AgeYears <= Points[Index].AgeYears)
				{
					const float Span = FMath::Max(0.0001f, Points[Index].AgeYears - Points[Index - 1].AgeYears);
					const float Alpha = (AgeYears - Points[Index - 1].AgeYears) / Span;
					return FMath::Lerp(Points[Index - 1].Value, Points[Index].Value, Alpha);
				}
			}
			return Points[Count - 1].Value;
		}

		/**
		 * Grundfrequenz vor dem Stimmwechsel. Bis dahin klingen Jungen und Mädchen gleich –
		 * das ist kein Vereinfachen, sondern gemessen: Der Unterschied entsteht erst mit dem Kehlkopfwachstum.
		 */
		float ChildF0(float AgeYears, const FGenesisVoiceTuning& Tuning)
		{
			const FAgePoint Points[] = {
				{ 0.0f, Tuning.NewbornF0Hz },
				{ 0.5f, Tuning.NewbornF0Hz * 0.93f },
				{ 1.0f, Tuning.NewbornF0Hz * 0.87f },
				{ 3.0f, Tuning.ChildF0Hz * 1.11f },
				{ 5.0f, Tuning.ChildF0Hz },
				{ 8.0f, Tuning.ChildF0Hz * 0.94f },
				{ 11.5f, Tuning.ChildF0Hz * 0.89f }
			};
			return SampleCurve(Points, UE_ARRAY_COUNT(Points), AgeYears);
		}

		/** Länge des Ansatzrohrs vor dem Stimmwechsel (cm). Ein Säugling hat kaum 7 cm. */
		float ChildTractCm(float AgeYears, const FGenesisVoiceTuning& Tuning)
		{
			const FAgePoint Points[] = {
				{ 0.0f, Tuning.NewbornTractCm },
				{ 1.0f, 8.0f },
				{ 4.0f, 10.0f },
				{ 8.0f, 11.5f },
				{ 11.5f, 13.0f }
			};
			return SampleCurve(Points, UE_ARRAY_COUNT(Points), AgeYears);
		}

		/** −1..1 aus einem Seed und einem Merkmal. Gleicher Mensch, gleiche Stimme. */
		float Individual(uint64 Seed, uint64 Salt)
		{
			FGenesisRandomStream Rng(GenesisHash::Combine(Seed, Salt));
			return Rng.NextFloat() * 2.0f - 1.0f;
		}
	}

	float GetBaseF0Hz(EGenesisBiologicalSex Sex, float AgeYears, const FGenesisVoiceTuning& Tuning)
	{
		const float Age = FMath::Max(0.0f, AgeYears);
		const float AdultF0 = Sex == EGenesisBiologicalSex::Male ? Tuning.AdultMaleF0Hz : Tuning.AdultFemaleF0Hz;

		float F0 = 0.0f;
		if (Age <= Tuning.VoiceChangeStartYears)
		{
			F0 = ChildF0(Age, Tuning);
		}
		else if (Age < Tuning.VoiceChangeEndYears)
		{
			// Der Stimmwechsel. Bei Jungen fällt die Stimme um etwa eine Oktave, bei Mädchen um wenige Halbtöne.
			// In der Mitte geht es am schnellsten – deshalb eine S-Kurve und keine Gerade.
			const float Span = FMath::Max(0.1f, Tuning.VoiceChangeEndYears - Tuning.VoiceChangeStartYears);
			const float Alpha = FMath::SmoothStep(0.0f, 1.0f, (Age - Tuning.VoiceChangeStartYears) / Span);
			F0 = FMath::Lerp(ChildF0(Tuning.VoiceChangeStartYears, Tuning), AdultF0, Alpha);
		}
		else
		{
			F0 = AdultF0;
		}

		// Presbyphonie: Im Alter werden die Stimmlippen des Mannes dünner (die Stimme steigt),
		// die der Frau schwellen an (sie fällt). Die Stimmen nähern sich also wieder an.
		if (Age > Tuning.AgingVoiceStartYears)
		{
			const float Alpha = FMath::Clamp((Age - Tuning.AgingVoiceStartYears) / 20.0f, 0.0f, 1.0f);
			F0 += Sex == EGenesisBiologicalSex::Male
				? Tuning.OldMaleF0RiseHz * Alpha
				: -Tuning.OldFemaleF0DropHz * Alpha;
		}

		return F0;
	}

	float GetVocalTractCm(EGenesisBiologicalSex Sex, float AgeYears, float HeightCm, const FGenesisVoiceTuning& Tuning)
	{
		const float Age = FMath::Max(0.0f, AgeYears);
		const float AdultTract = Sex == EGenesisBiologicalSex::Male ? Tuning.AdultMaleTractCm : Tuning.AdultFemaleTractCm;

		float Tract = 0.0f;
		if (Age <= Tuning.VoiceChangeStartYears)
		{
			Tract = ChildTractCm(Age, Tuning);
		}
		else if (Age < 18.0f)
		{
			// Beim Jungen sinkt der Kehlkopf zusätzlich – das Ansatzrohr wird länger als bei gleich großen Mädchen
			const float Alpha = FMath::Clamp((Age - Tuning.VoiceChangeStartYears) / (18.0f - Tuning.VoiceChangeStartYears), 0.0f, 1.0f);
			Tract = FMath::Lerp(ChildTractCm(Tuning.VoiceChangeStartYears, Tuning), AdultTract, FMath::SmoothStep(0.0f, 1.0f, Alpha));
		}
		else
		{
			Tract = AdultTract;
		}

		// Körpergröße wirkt erst beim Ausgewachsenen: Ein zwei Meter großer Mensch hat ein längeres Ansatzrohr
		// und damit eine dunklere Klangfarbe. Bei Kindern ist die Größe fast nur Alter, deshalb zählt sie dort nicht.
		if (Age >= 16.0f && HeightCm > 50.0f)
		{
			const float Reference = Sex == EGenesisBiologicalSex::Male ? 175.0f : 165.0f;
			Tract *= FMath::Clamp(FMath::Sqrt(HeightCm / Reference), 0.92f, 1.08f);
		}

		return Tract;
	}

	FGenesisVoiceProfile BuildProfile(const FGenesisVoiceInputs& Inputs, const FGenesisVoiceTuning& Tuning)
	{
		FGenesisVoiceProfile Profile;
		Profile.Sex = Inputs.Sex;
		Profile.AgeYears = FMath::Max(0.0f, Inputs.AgeYears);

		const float Age = Profile.AgeYears;
		const float Health = FMath::Clamp(Inputs.RespiratoryHealth, 0.0f, 1.0f);
		const float Illness = FMath::Clamp(Inputs.Illness, 0.0f, 1.0f);
		const float Exhaustion = FMath::Clamp(Inputs.Exhaustion, 0.0f, 1.0f);
		const float Arousal = FMath::Clamp(Inputs.Arousal, 0.0f, 1.0f);

		// 1. Grundfrequenz: Alter, Geschlecht, individuelle Streuung, Tagesform
		float F0 = GetBaseF0Hz(Inputs.Sex, Age, Tuning);
		F0 *= 1.0f + 0.07f * Individual(Inputs.IndividualSeed, 0x564F494345ull);
		// Erregung hebt die Stimme messbar – bis zu drei Halbtöne bei starker Aufregung
		F0 *= FMath::Lerp(0.98f, 1.19f, Arousal);
		// Erschöpfung senkt sie leicht, eine geschwollene Schleimhaut ebenfalls
		F0 *= 1.0f - 0.04f * Exhaustion - 0.05f * Illness;
		Profile.F0Hz = FMath::Max(50.0f, F0);

		// 2. Klangfarbe aus der Länge des Ansatzrohrs
		const float Tract = GetVocalTractCm(Inputs.Sex, Age, Inputs.HeightCm, Tuning)
			* (1.0f + 0.03f * Individual(Inputs.IndividualSeed, 0x545241435400ull));
		Profile.FormantScale = FMath::Clamp(Tuning.AdultMaleTractCm / FMath::Max(3.0f, Tract), 0.8f, 3.0f);

		// 3. Melodie: Kinder sprechen weit melodischer als Erwachsene, Erregung vergrößert den Umfang
		Profile.F0RangeSemitones = FMath::Lerp(7.5f, 3.5f, FMath::Clamp(Age / 18.0f, 0.0f, 1.0f)) + 4.0f * Arousal;

		// 4. Behauchtheit und Rauigkeit. Beide steigen im Alter – das ist der eigentliche Grund,
		// warum man eine alte Stimme sofort erkennt, noch bevor man die Tonhöhe beurteilt.
		const float AgeFactor = FMath::Clamp((Age - 55.0f) / 30.0f, 0.0f, 1.0f);
		Profile.Breathiness = FMath::Clamp(
			0.12f + Tuning.AgeBreathiness * AgeFactor + 0.35f * (1.0f - Health) + 0.2f * Exhaustion
			+ (Inputs.Sex == EGenesisBiologicalSex::Female ? 0.06f : 0.0f), 0.0f, 1.0f);
		Profile.Roughness = FMath::Clamp(
			0.03f + Tuning.AgeRoughness * AgeFactor + 0.45f * Illness + 0.1f * (1.0f - Health), 0.0f, 1.0f);
		// Der Schrei eines Neugeborenen ist von Natur aus rau – die Stimmlippen sind noch nicht eingeschwungen
		Profile.Roughness = FMath::Max(Profile.Roughness, FMath::Lerp(0.30f, 0.0f, FMath::Clamp(Age / 2.0f, 0.0f, 1.0f)));

		// 5. Näseln: verlegte Nase
		Profile.Nasality = FMath::Clamp(0.08f + 0.55f * Illness, 0.0f, 1.0f);

		// 6. Kraft und Tempo
		Profile.Strength = FMath::Clamp(Health * (1.0f - 0.45f * Exhaustion) * (1.0f - 0.25f * Illness)
			* FMath::Lerp(0.75f, 1.0f, FMath::Clamp(Age / 6.0f, 0.0f, 1.0f)) * (1.0f - 0.25f * AgeFactor), 0.05f, 1.0f);
		Profile.TempoScale = FMath::Clamp(FMath::Lerp(1.25f, 1.0f, FMath::Clamp(Age / 14.0f, 0.0f, 1.0f))
			* FMath::Lerp(0.92f, 1.25f, Arousal) * (1.0f - 0.25f * AgeFactor), 0.5f, 2.0f);

		return Profile;
	}

	bool CanMake(EGenesisUtterance Utterance, float AgeYears)
	{
		// Die Reihenfolge ist keine Spielregel, sondern Entwicklung: Schreien kann ein Kind sofort,
		// Gurren ab etwa zwei Monaten, Lachen ab vier, Lallen ab sechs, Silben mit Absicht ab etwa einem Jahr.
		switch (Utterance)
		{
		case EGenesisUtterance::Cry:
		case EGenesisUtterance::Fuss:
			return true;
		case EGenesisUtterance::Coo:
			return AgeYears >= 2.0f / 12.0f;
		case EGenesisUtterance::Laugh:
			return AgeYears >= 4.0f / 12.0f;
		case EGenesisUtterance::Babble:
			return AgeYears >= 6.0f / 12.0f;
		case EGenesisUtterance::Sigh:
		case EGenesisUtterance::Hum:
			return AgeYears >= 1.0f;
		case EGenesisUtterance::Speak:
		case EGenesisUtterance::Call:
		case EGenesisUtterance::Soothe:
			return AgeYears >= 1.5f;
		default:
			return true;
		}
	}

	float GetNaturalDurationSeconds(EGenesisUtterance Utterance)
	{
		switch (Utterance)
		{
		case EGenesisUtterance::Cry: return 3.2f;
		case EGenesisUtterance::Fuss: return 1.6f;
		case EGenesisUtterance::Coo: return 1.1f;
		case EGenesisUtterance::Babble: return 2.0f;
		case EGenesisUtterance::Laugh: return 1.4f;
		case EGenesisUtterance::Sigh: return 1.8f;
		case EGenesisUtterance::Hum: return 3.0f;
		case EGenesisUtterance::Soothe: return 1.6f;
		case EGenesisUtterance::Speak: return 2.2f;
		case EGenesisUtterance::Call: return 1.2f;
		default: return 1.5f;
		}
	}

	void GetVowelFormants(EGenesisVowel Vowel, float OutFormantsHz[4])
	{
		// Gemessene Mittelwerte erwachsener Männer (Peterson & Barney und Folgemessungen).
		switch (Vowel)
		{
		case EGenesisVowel::A: OutFormantsHz[0] = 730.0f; OutFormantsHz[1] = 1090.0f; OutFormantsHz[2] = 2440.0f; OutFormantsHz[3] = 3400.0f; break;
		case EGenesisVowel::E: OutFormantsHz[0] = 530.0f; OutFormantsHz[1] = 1840.0f; OutFormantsHz[2] = 2480.0f; OutFormantsHz[3] = 3500.0f; break;
		case EGenesisVowel::I: OutFormantsHz[0] = 270.0f; OutFormantsHz[1] = 2290.0f; OutFormantsHz[2] = 3010.0f; OutFormantsHz[3] = 3700.0f; break;
		case EGenesisVowel::O: OutFormantsHz[0] = 570.0f; OutFormantsHz[1] = 840.0f; OutFormantsHz[2] = 2410.0f; OutFormantsHz[3] = 3300.0f; break;
		case EGenesisVowel::U: OutFormantsHz[0] = 300.0f; OutFormantsHz[1] = 870.0f; OutFormantsHz[2] = 2240.0f; OutFormantsHz[3] = 3300.0f; break;
		default: OutFormantsHz[0] = 500.0f; OutFormantsHz[1] = 1500.0f; OutFormantsHz[2] = 2500.0f; OutFormantsHz[3] = 3400.0f; break;
		}
	}

	FString GetUtteranceName(EGenesisUtterance Utterance)
	{
		switch (Utterance)
		{
		case EGenesisUtterance::Cry: return TEXT("Schreien");
		case EGenesisUtterance::Fuss: return TEXT("Quengeln");
		case EGenesisUtterance::Coo: return TEXT("Gurren");
		case EGenesisUtterance::Babble: return TEXT("Lallen");
		case EGenesisUtterance::Laugh: return TEXT("Lachen");
		case EGenesisUtterance::Sigh: return TEXT("Seufzen");
		case EGenesisUtterance::Hum: return TEXT("Summen");
		case EGenesisUtterance::Soothe: return TEXT("Beruhigen");
		case EGenesisUtterance::Speak: return TEXT("Sprechen");
		case EGenesisUtterance::Call: return TEXT("Rufen");
		default: return TEXT("Laut");
		}
	}

	FString GetVowelName(EGenesisVowel Vowel)
	{
		switch (Vowel)
		{
		case EGenesisVowel::A: return TEXT("a");
		case EGenesisVowel::E: return TEXT("e");
		case EGenesisVowel::I: return TEXT("i");
		case EGenesisVowel::O: return TEXT("o");
		case EGenesisVowel::U: return TEXT("u");
		default: return TEXT("ə");
		}
	}

	FString DescribeVoice(const FGenesisVoiceProfile& Profile)
	{
		FString Color;
		if (Profile.FormantScale > 2.0f)
		{
			Color = TEXT("Säuglingsklang");
		}
		else if (Profile.FormantScale > 1.45f)
		{
			Color = TEXT("Kinderklang");
		}
		else if (Profile.FormantScale > 1.12f)
		{
			Color = TEXT("heller Klang");
		}
		else
		{
			Color = TEXT("dunkler Klang");
		}

		FString Quality;
		if (Profile.Roughness > 0.35f)
		{
			Quality = Profile.Breathiness > 0.4f ? TEXT(", rau und behaucht") : TEXT(", rau");
		}
		else if (Profile.Breathiness > 0.4f)
		{
			Quality = TEXT(", behaucht");
		}

		return FString::Printf(TEXT("%s, %.0f Hz%s"), *Color, Profile.F0Hz, *Quality);
	}
}
