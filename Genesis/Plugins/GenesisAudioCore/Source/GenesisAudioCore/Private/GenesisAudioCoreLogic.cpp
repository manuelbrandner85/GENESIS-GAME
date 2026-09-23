// GENESIS: Der Kreislauf des Lebens

#include "GenesisAudioCoreLogic.h"
#include "GenesisBodyGameplayTags.h"
#include "GenesisBodyLogic.h"
#include "GenesisFetalLogic.h"
#include "GenesisTypes.h"

FGenesisAudioMixTuning::FGenesisAudioMixTuning()
{
	auto Rule = [this](EGenesisAudioBus Trigger, EGenesisAudioBus Target, float DuckDb, float Attack, float Release)
	{
		FGenesisDuckingRule& Entry = DuckingRules.AddDefaulted_GetRef();
		Entry.Trigger = Trigger;
		Entry.Target = Target;
		Entry.DuckDb = DuckDb;
		Entry.AttackSeconds = Attack;
		Entry.ReleaseSeconds = Release;
	};

	// Dialog hat Vorrang: Musik und Umgebung treten dezent zurück
	Rule(EGenesisAudioBus::Dialogue, EGenesisAudioBus::Music, -8.0f, 0.35f, 1.2f);
	Rule(EGenesisAudioBus::Dialogue, EGenesisAudioBus::Ambient, -5.0f, 0.35f, 1.5f);
	Rule(EGenesisAudioBus::Dialogue, EGenesisAudioBus::Foley, -3.0f, 0.35f, 1.0f);
	// Lebenswichtige Signale schneiden schneller durch
	Rule(EGenesisAudioBus::VitalSignal, EGenesisAudioBus::Music, -6.0f, 0.15f, 1.5f);
	Rule(EGenesisAudioBus::VitalSignal, EGenesisAudioBus::Ambient, -3.0f, 0.15f, 1.5f);
	// Wird der Körper hörbar (Angst, Atemnot), rückt die Musik leicht weg
	Rule(EGenesisAudioBus::Body, EGenesisAudioBus::Music, -2.0f, 0.8f, 2.0f);
}

namespace GenesisAudioCoreLogic
{
	namespace
	{
		float TimeConstantAlpha(float DeltaSeconds, float TimeConstant)
		{
			return TimeConstant <= 0.0f ? 1.0f : 1.0f - FMath::Exp(-FMath::Max(0.0f, DeltaSeconds) / TimeConstant);
		}

		float HeadInjurySeverity(const FGenesisBodyState& Body)
		{
			float Severity = 0.0f;
			for (const FGenesisBodyCondition& Condition : Body.Conditions)
			{
				if (Condition.Condition == GenesisBodyTags::Condition_Injury && Condition.Region == GenesisBodyTags::Region_Head)
				{
					Severity = FMath::Max(Severity, Condition.Severity);
				}
			}
			return Severity;
		}
	}

	FGenesisHearingPerception ComputeHearing(const FGenesisBodyState* Body, const FGenesisTimestamp& Now, const FGenesisHearingTuning& Tuning)
	{
		FGenesisHearingPerception Perception;
		if (!Body || !Body->bAlive || Body->Senses.Num() < GenesisSenseCount)
		{
			return Perception;
		}

		if (!Body->bBorn)
		{
			// Mutterleib (GENESIS-044, Docs/34): Das Kind hört erst ab SSW 19, zuerst ein schmales Band um 500 Hz, das sich
			// dann nach unten und zuletzt nach oben weitet; die Schwelle sinkt bis zum Termin um 20–30 dB. Bauchdecke und
			// Fruchtwasser dämpfen oberhalb 600–1000 Hz um ~30 dB – mehr als das Gehör später könnte, kommt nicht an.
			// Vor der Verbindung Thalamus–Rinde (SSW 23–26) antwortet der Körper, erlebt wird noch wenig.
			const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(GenesisFetalLogic::GetReference(),
				static_cast<float>(GenesisBodyLogic::GetGestationalWeeks(*Body, Now) + GenesisFetalLogic::WeeksFromConceptionToGestational));
			Perception.bInWomb = true;
			const bool bHears = Fetal.HearingHighHz > 0.0f;
			Perception.LowPassCutoffHz = bHears ? FMath::Min(Fetal.HearingHighHz, Tuning.WombCutoffMaxHz) : Tuning.WombCutoffMinHz;
			Perception.HighPassCutoffHz = bHears ? Fetal.HearingLowHz : 0.0f;
			const float Experienced = Fetal.HearingSensitivity * (0.35f + 0.65f * Fetal.ConsciousAccess);
			// Die Laute der Mutter (Herz, Blut, ihre Stimme durch den eigenen Körper) liegen im Mutterleib deutlich über
			// allem von draußen
			Perception.BodyAudibility = Experienced;
			Perception.ExternalAudibility = 0.6f * Experienced;
			return Perception;
		}

		const float Acuity = Body->Sense(EGenesisBodySense::Hearing).Acuity;
		const TArray<FGenesisSymptom> Symptoms = GenesisBodyLogic::DeriveSymptoms(*Body, Now);
		const float Fever = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_Fever);
		const float Pain = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_Pain);
		const float HeartPounding = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_HeartPounding);
		const float Breathlessness = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_Breathlessness);

		// Schock, Angst, Schmerz: Die Welt tritt zurück, der eigene Körper tritt hervor
		Perception.FocusNarrowing = FMath::Clamp((Body->Hormones.Adrenaline - 0.5f) * 2.0f + 0.3f * Pain, 0.0f, 1.0f);

		Perception.LowPassCutoffHz = FMath::Lerp(4000.0f, 20000.0f, FMath::Clamp(Acuity, 0.0f, 1.0f)) * (1.0f - 0.3f * Fever);

		const float YearsPastFifty = FMath::Max(0.0f, static_cast<float>(Body->BiologicalAgeYears) - 50.0f);
		Perception.HighShelfGainDb = FMath::Max(Tuning.MaxAgeHighShelfLossDb, Tuning.AgeHighShelfLossPerYearDb * YearsPastFifty);

		Perception.ExternalAudibility = FMath::Clamp(0.4f + 0.6f * Acuity, 0.0f, 1.0f) * (1.0f - 0.35f * Perception.FocusNarrowing);
		Perception.BodyAudibility = FMath::Clamp(0.05f + 0.5f * Perception.FocusNarrowing + 0.3f * HeartPounding + 0.3f * Breathlessness, 0.0f, 1.0f);
		Perception.TinnitusLevel = FMath::Clamp(0.8f * HeadInjurySeverity(*Body) + 0.15f * FMath::Max(0.0f, static_cast<float>(Body->BiologicalAgeYears) - 70.0f) / 30.0f, 0.0f, 0.6f);
		return Perception;
	}

	FGenesisBodyAudioParams ComputeBodyAudio(const FGenesisBodyState* Body, const FGenesisTimestamp& Now, const FGenesisHearingPerception& Perception)
	{
		FGenesisBodyAudioParams Audio;
		if (!Body || !Body->bAlive || Body->Organs.Num() < GenesisOrganCount)
		{
			return Audio;
		}

		const FGenesisOrganState& Heart = Body->Organ(EGenesisOrgan::Heart);
		Audio.HeartRateBpm = Body->Vitals.HeartRate;

		if (!Body->bBorn)
		{
			// Das Ungeborene hört vor allem die Mutter: ihren Herzschlag, ihr Blut, ihre Stimme von außen
			Audio.bMaternalSounds = true;
			Audio.MaternalHeartRateBpm = 72.0f;
			Audio.HeartStrength = 0.4f * Heart.Development;
			Audio.HeartAudibility = 0.5f * Perception.BodyAudibility * Heart.Development;
			return Audio;
		}

		const TArray<FGenesisSymptom> Symptoms = GenesisBodyLogic::DeriveSymptoms(*Body, Now);
		const double Age = GenesisBodyLogic::GetAgeYears(*Body, Now);
		const float BaseHeartRate = GenesisBodyLogic::GetBaselineHeartRate(Age);
		const float BaseRespiration = GenesisBodyLogic::GetBaselineRespiratoryRate(Age);

		float CardiacSeverity = 0.0f;
		for (const FGenesisBodyCondition& Condition : Body->Conditions)
		{
			if (Condition.Condition == GenesisBodyTags::Condition_Cardiac)
			{
				CardiacSeverity = FMath::Max(CardiacSeverity, Condition.Severity);
			}
		}

		Audio.HeartStrength = FMath::Clamp(0.4f + 0.4f * Body->Hormones.Adrenaline + 0.3f * (Audio.HeartRateBpm / FMath::Max(1.0f, BaseHeartRate) - 1.0f), 0.0f, 1.0f);
		Audio.HeartIrregularity = FMath::Clamp(0.6f * Heart.Wear + CardiacSeverity, 0.0f, 1.0f);
		Audio.HeartAudibility = Perception.BodyAudibility;

		Audio.BreathRate = Body->Vitals.RespiratoryRate;
		Audio.BreathStrain = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_Breathlessness);
		Audio.BreathDepth = FMath::Clamp(0.3f + 0.5f * (Audio.BreathRate / FMath::Max(1.0f, BaseRespiration) - 1.0f) + 0.3f * Audio.BreathStrain, 0.0f, 1.0f);
		Audio.BreathAudibility = FMath::Clamp(Perception.BodyAudibility + 0.4f * Audio.BreathStrain + (Body->Activity == EGenesisActivity::Sleep ? 0.1f : 0.0f), 0.0f, 1.0f);
		Audio.Tremor = GenesisBodyLogic::GetSymptomIntensity(Symptoms, GenesisBodyTags::Symptom_Tremor);
		return Audio;
	}

	FGenesisHearingPerception SmoothPerception(const FGenesisHearingPerception& Current, const FGenesisHearingPerception& Target,
		float DeltaSeconds, const FGenesisHearingTuning& Tuning)
	{
		// Ein Umgebungswechsel startet einen Übergang, der bis zum Ankommen schnell bleibt –
		// sonst würde nur der erste Frame schnell reagieren und der Rest träge nachziehen
		const bool bEnvironmentChange = Current.bInWomb != Target.bInWomb;
		const bool bFastTransition = bEnvironmentChange || Current.bInEnvironmentTransition;
		const float Alpha = TimeConstantAlpha(DeltaSeconds, bFastTransition ? Tuning.BirthTransitionSeconds : Tuning.PerceptionSmoothingSeconds);

		FGenesisHearingPerception Result = Target;
		const double CurrentLog = FMath::Loge(FMath::Max(20.0f, Current.LowPassCutoffHz));
		const double TargetLog = FMath::Loge(FMath::Max(20.0f, Target.LowPassCutoffHz));
		Result.LowPassCutoffHz = static_cast<float>(FMath::Exp(FMath::Lerp(CurrentLog, TargetLog, static_cast<double>(Alpha))));
		// Angekommen, wenn der Tiefpass auf 2 % am Ziel liegt
		Result.bInEnvironmentTransition = bFastTransition && FMath::Abs(FMath::Loge(FMath::Max(20.0f, Result.LowPassCutoffHz)) - TargetLog) > FMath::Loge(1.02);
		Result.HighPassCutoffHz = FMath::Lerp(Current.HighPassCutoffHz, Target.HighPassCutoffHz, Alpha);
		Result.HighShelfGainDb = FMath::Lerp(Current.HighShelfGainDb, Target.HighShelfGainDb, Alpha);
		Result.ExternalAudibility = FMath::Lerp(Current.ExternalAudibility, Target.ExternalAudibility, Alpha);
		Result.BodyAudibility = FMath::Lerp(Current.BodyAudibility, Target.BodyAudibility, Alpha);
		Result.FocusNarrowing = FMath::Lerp(Current.FocusNarrowing, Target.FocusNarrowing, Alpha);
		Result.TinnitusLevel = FMath::Lerp(Current.TinnitusLevel, Target.TinnitusLevel, Alpha);
		return Result;
	}

	void UpdateMix(FGenesisMixState& State, const TArray<FGenesisMixRequest>& ActiveRequests, const FGenesisAudioMixTuning& Tuning, float DeltaSeconds)
	{
		if (State.CurrentGainDb.Num() != GenesisAudioBusCount || State.TargetGainDb.Num() != GenesisAudioBusCount)
		{
			State = FGenesisMixState();
		}

		// Stärkste Wichtigkeit je Bus
		float Importance[GenesisAudioBusCount] = { 0.0f };
		for (const FGenesisMixRequest& Request : ActiveRequests)
		{
			const int32 Index = static_cast<int32>(Request.Bus);
			Importance[Index] = FMath::Max(Importance[Index], FMath::Clamp(Request.Importance, 0.0f, 1.0f));
		}

		float Target[GenesisAudioBusCount] = { 0.0f };
		float Attack[GenesisAudioBusCount];
		float Release[GenesisAudioBusCount];
		for (int32 Index = 0; Index < GenesisAudioBusCount; ++Index)
		{
			Attack[Index] = 0.35f;
			Release[Index] = 1.2f;
		}

		for (const FGenesisDuckingRule& Rule : Tuning.DuckingRules)
		{
			const int32 TargetIndex = static_cast<int32>(Rule.Target);
			Release[TargetIndex] = FMath::Max(Release[TargetIndex], Rule.ReleaseSeconds);

			const float TriggerImportance = Importance[static_cast<int32>(Rule.Trigger)];
			if (TriggerImportance > 0.0f)
			{
				Target[TargetIndex] += Rule.DuckDb * TriggerImportance;
				Attack[TargetIndex] = FMath::Min(Attack[TargetIndex], Rule.AttackSeconds);
			}
		}

		for (int32 Index = 0; Index < GenesisAudioBusCount; ++Index)
		{
			State.TargetGainDb[Index] = FMath::Max(Tuning.MaxDuckDb, Target[Index]);
			const float Current = State.CurrentGainDb[Index];
			const bool bDucking = State.TargetGainDb[Index] < Current;
			const float Alpha = TimeConstantAlpha(DeltaSeconds, bDucking ? Attack[Index] : Release[Index]);
			State.CurrentGainDb[Index] = FMath::Lerp(Current, State.TargetGainDb[Index], Alpha);
		}
	}

	float DbToLinear(float Db)
	{
		return FMath::Pow(10.0f, Db / 20.0f);
	}

	float UnbornPresentationGain(float Audibility)
	{
		return Audibility <= 0.001f ? 0.0f : 0.25f + 0.75f * FMath::Clamp(Audibility, 0.0f, 1.0f);
	}
}
