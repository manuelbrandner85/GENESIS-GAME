// GENESIS: Der Kreislauf des Lebens

#include "GenesisFetalLogic.h"

FGenesisFetalReference::FGenesisFetalReference()
{
	// SSW, Scheitel-Steiß (cm), Scheitel-Ferse (cm), Gewicht (g), Herz (/min) – Quellen in Docs/34:
	// Scheitel-Steiß bis SSW 14 nach Robinson & Fleming; Gewicht ab SSW 14 WHO-Wachstumskurve (50. Perzentile);
	// Scheitel-Ferse ab SSW 14 nach der WHO-basierten Tabelle; Herz: 90–110 in SSW 6, 150–170 in SSW 9–10, 130–140 zum Termin.
	// Tag 28 des Embryos (SSW 6) hat 4,6 mm – dasselbe Modell wie in der Fruchthöhle.
	struct FRow { float Weeks, Crl, Chl, Grams, Bpm; };
	static const FRow Rows[] = {
		{ 6.0f, 0.46f, 0.0f, 0.02f, 110.0f },
		{ 7.0f, 1.0f, 0.0f, 0.2f, 130.0f },
		{ 8.0f, 1.6f, 0.0f, 1.0f, 150.0f },
		{ 9.0f, 2.3f, 0.0f, 2.0f, 170.0f },
		{ 10.0f, 3.1f, 0.0f, 4.0f, 168.0f },
		{ 11.0f, 4.1f, 0.0f, 8.0f, 163.0f },
		{ 12.0f, 5.4f, 0.0f, 18.0f, 158.0f },
		{ 13.0f, 6.7f, 0.0f, 40.0f, 155.0f },
		{ 14.0f, 8.4f, 14.7f, 90.0f, 152.0f },
		{ 16.0f, 0.0f, 18.6f, 144.0f, 150.0f },
		{ 18.0f, 0.0f, 22.2f, 222.0f, 148.0f },
		{ 20.0f, 0.0f, 25.7f, 330.0f, 145.0f },
		{ 22.0f, 0.0f, 29.0f, 476.0f, 144.0f },
		{ 24.0f, 0.0f, 32.2f, 665.0f, 143.0f },
		{ 26.0f, 0.0f, 35.1f, 902.0f, 142.0f },
		{ 28.0f, 0.0f, 37.6f, 1189.0f, 141.0f },
		{ 30.0f, 0.0f, 40.5f, 1523.0f, 140.0f },
		{ 32.0f, 0.0f, 43.0f, 1901.0f, 139.0f },
		{ 34.0f, 0.0f, 45.3f, 2312.0f, 138.0f },
		{ 36.0f, 0.0f, 47.3f, 2745.0f, 137.0f },
		{ 38.0f, 0.0f, 49.3f, 3186.0f, 136.0f },
		{ 40.0f, 0.0f, 51.0f, 3617.0f, 135.0f },
	};
	for (const FRow& Row : Rows)
	{
		FGenesisFetalGrowthPoint Point;
		Point.GestationalWeeks = Row.Weeks;
		Point.CrownRumpCm = Row.Crl;
		Point.CrownHeelCm = Row.Chl;
		Point.WeightGrams = Row.Grams;
		Point.HeartRateBpm = Row.Bpm;
		Growth.Add(Point);
	}
	// Beckenendlage: ~20 % in SSW 28, 7–15 % in SSW 32, 6,9–10 % zwischen 32 und 36, 3–4 % zum Termin
	BreechShare = { FVector2D(20.0f, 0.35f), FVector2D(28.0f, 0.20f), FVector2D(32.0f, 0.10f), FVector2D(36.0f, 0.06f), FVector2D(40.0f, 0.035f) };
}

namespace GenesisFetalLogic
{
	namespace
	{
		float Smooth(float Edge0, float Edge1, float Value)
		{
			const float T = FMath::Clamp((Value - Edge0) / FMath::Max(KINDA_SMALL_NUMBER, Edge1 - Edge0), 0.0f, 1.0f);
			return T * T * (3.0f - 2.0f * T);
		}

		/** Linear zwischen den Punkten einer Spalte; nur Punkte mit einem Wert (> 0) zählen, außerhalb 0. */
		template <typename TGetter>
		float Interpolate(const TArray<FGenesisFetalGrowthPoint>& Growth, float Weeks, TGetter Get, bool bClampEnds)
		{
			const FGenesisFetalGrowthPoint* Before = nullptr;
			const FGenesisFetalGrowthPoint* After = nullptr;
			for (const FGenesisFetalGrowthPoint& Point : Growth)
			{
				if (Get(Point) <= 0.0f)
				{
					continue;
				}
				if (Point.GestationalWeeks <= Weeks)
				{
					Before = &Point;
				}
				else if (!After)
				{
					After = &Point;
				}
			}
			if (Before && After)
			{
				const float Alpha = (Weeks - Before->GestationalWeeks) / FMath::Max(KINDA_SMALL_NUMBER, After->GestationalWeeks - Before->GestationalWeeks);
				return FMath::Lerp(Get(*Before), Get(*After), Alpha);
			}
			if (!bClampEnds)
			{
				return (Before && FMath::IsNearlyEqual(Before->GestationalWeeks, Weeks)) ? Get(*Before) : 0.0f;
			}
			return Before ? Get(*Before) : (After ? Get(*After) : 0.0f);
		}

		/** Logarithmisch zwischen Stützpunkten (Frequenzen). */
		float LogCurve(const TArray<FVector2D>& Points, float Weeks)
		{
			if (Points.Num() == 0)
			{
				return 0.0f;
			}
			if (Weeks <= Points[0].X)
			{
				return Points[0].Y;
			}
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				if (Weeks <= Points[Index].X)
				{
					const float Alpha = (Weeks - Points[Index - 1].X) / FMath::Max(KINDA_SMALL_NUMBER, Points[Index].X - Points[Index - 1].X);
					return FMath::Exp(FMath::Lerp(FMath::Loge(Points[Index - 1].Y), FMath::Loge(Points[Index].Y), Alpha));
				}
			}
			return Points.Last().Y;
		}

		float LinearCurve(const TArray<FVector2D>& Points, float Weeks)
		{
			if (Points.Num() == 0)
			{
				return 0.0f;
			}
			if (Weeks <= Points[0].X)
			{
				return Points[0].Y;
			}
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				if (Weeks <= Points[Index].X)
				{
					const float Alpha = (Weeks - Points[Index - 1].X) / FMath::Max(KINDA_SMALL_NUMBER, Points[Index].X - Points[Index - 1].X);
					return FMath::Lerp(Points[Index - 1].Y, Points[Index].Y, Alpha);
				}
			}
			return Points.Last().Y;
		}

		int32 StateIndex(EGenesisFetalState State) { return static_cast<int32>(State); }
	}

	const FGenesisFetalReference& GetReference()
	{
		if (const UGenesisFetalSettings* Settings = GetDefault<UGenesisFetalSettings>())
		{
			if (Settings->Reference.Growth.Num() > 0)
			{
				return Settings->Reference;
			}
		}
		static const FGenesisFetalReference Builtin;
		return Builtin;
	}

	FGenesisFetalView Evaluate(const FGenesisFetalReference& Reference, float Weeks)
	{
		const FGenesisFetalMilestones& M = Reference.Milestones;
		FGenesisFetalView View;
		View.GestationalWeeks = Weeks;
		View.CrownRumpCm = Interpolate(Reference.Growth, Weeks, [](const FGenesisFetalGrowthPoint& P) { return P.CrownRumpCm; }, false);
		View.CrownHeelCm = Interpolate(Reference.Growth, Weeks, [](const FGenesisFetalGrowthPoint& P) { return P.CrownHeelCm; }, false);
		View.WeightGrams = Interpolate(Reference.Growth, Weeks, [](const FGenesisFetalGrowthPoint& P) { return P.WeightGrams; }, true);
		View.HeartRateBpm = Weeks < 5.0f ? 0.0f : Interpolate(Reference.Growth, Weeks, [](const FGenesisFetalGrowthPoint& P) { return P.HeartRateBpm; }, true);

		// Hören: zuerst ein Band um 500 Hz, dann nach unten (100–250 Hz), zuletzt nach oben (1000 Hz in SSW 33, 3000 Hz in 35)
		if (Weeks >= M.HearingOnset)
		{
			View.HearingLowHz = LogCurve({ FVector2D(M.HearingOnset, 400.0f), FVector2D(M.HearingLowFrequencies, 180.0f),
				FVector2D(M.HearingOnset + 8.0f, 90.0f), FVector2D(40.0f, 60.0f) }, Weeks);
			View.HearingHighHz = LogCurve({ FVector2D(M.HearingOnset, 620.0f), FVector2D(M.HearingOnset + 8.0f, 800.0f),
				FVector2D(M.Hearing1000Hz, 1100.0f), FVector2D(M.Hearing3000Hz, 3000.0f), FVector2D(40.0f, 5000.0f) }, Weeks);
			// Hörschwelle: ~65 dB beim Einsetzen, 40 dB um SSW 28, 13,5 dB zum Termin. Die Laute der Mutter liegen im
			// Mutterleib um 75 dB – so weit sie über der Schwelle liegen, so deutlich kommen sie an.
			const float ThresholdDb = LinearCurve({ FVector2D(M.HearingOnset, 65.0f), FVector2D(28.0f, M.HearingThresholdAt28Db),
				FVector2D(40.0f, M.HearingThresholdAtTermDb) }, Weeks);
			View.HearingSensitivity = FMath::Clamp((75.0f - ThresholdDb) / (75.0f - M.HearingThresholdAtTermDb), 0.0f, 1.0f);
		}

		View.EyesOpen = Smooth(M.EyesBeginToOpen, M.EyesOpen, Weeks);
		// Helligkeit: mit offenen Lidern schwach, mit der Pupillenreaktion voll
		View.LightPerception = 0.4f * View.EyesOpen + 0.6f * Smooth(M.PupilResponse - 2.0f, M.PupilResponse + 1.0f, Weeks);
		View.Touch = 0.3f * Smooth(M.TouchAroundMouth - 0.5f, M.TouchAroundMouth + 0.5f, Weeks) + 0.7f * Smooth(M.TouchAroundMouth + 2.0f, M.TouchWholeBody, Weeks);
		View.ConsciousAccess = Smooth(M.ConsciousAccessBegins, M.ConsciousAccessEstablished, Weeks);
		View.bMotherFeelsMovement = Weeks >= M.QuickeningFirstPregnancy;
		View.BreechShare = LinearCurve(Reference.BreechShare, Weeks);
		return View;
	}

	float MeanStateSeconds(EGenesisFetalState State, float Weeks, const FGenesisFetalMilestones& M)
	{
		// Vor SSW 32: Ruhe 58 %, Aktivität 42 %, in kurzen Wechseln. Danach deutliche Zustände; ab SSW 36 wie beim
		// Neugeborenen: ruhiger Schlaf 24 %, aktiver Schlaf 65 %, aktiv wach 11 % (Nijhuis 1982; Docs/34).
		const float Clear = Smooth(M.ClearSleepStates - 1.0f, M.ClearSleepStates + 1.0f, Weeks);
		const float Four = Smooth(M.FourBehaviouralStates - 1.0f, M.FourBehaviouralStates + 1.0f, Weeks);
		const float CycleScale = FMath::Lerp(0.4f, 1.0f, Smooth(M.RestActivityCycles - 5.0f, M.ClearSleepStates, Weeks));
		switch (State)
		{
		case EGenesisFetalState::Quiet:       return CycleScale * FMath::Lerp(7.0f * 60.0f, 21.0f * 60.0f, Clear);
		case EGenesisFetalState::Active:      return CycleScale * FMath::Lerp(5.0f * 60.0f, 57.0f * 60.0f, Clear);
		case EGenesisFetalState::ActiveAwake: return Four > 0.0f ? FMath::Lerp(0.0f, 9.6f * 60.0f, Four) : 0.0f;
		default:                               return 0.0f;
		}
	}

	FString GetStateName(EGenesisFetalState State, float Weeks, const FGenesisFetalMilestones& M)
	{
		const bool bSleepStates = Weeks >= M.ClearSleepStates;
		switch (State)
		{
		case EGenesisFetalState::Quiet:       return bSleepStates ? TEXT("ruhiger Schlaf") : TEXT("Ruhe");
		case EGenesisFetalState::Active:      return bSleepStates ? TEXT("aktiver Schlaf") : TEXT("Aktivität");
		case EGenesisFetalState::QuietAwake:  return TEXT("ruhig wach");
		case EGenesisFetalState::ActiveAwake: return TEXT("aktiv wach");
		default:                               return TEXT("?");
		}
	}

	TArray<EGenesisFetalEvent> AdvanceBehaviour(FGenesisFetalBehaviour& B, const FGenesisFetalView& View, const FGenesisFetalMilestones& M,
		float DeltaSeconds, float MaternalMovement, FRandomStream& Random)
	{
		TArray<EGenesisFetalEvent> Events;
		const float Weeks = View.GestationalWeeks;
		if (B.SecondsInState.Num() != 4)
		{
			B.SecondsInState.Init(0.0f, 4);
		}
		if (Weeks < M.FirstMovement || DeltaSeconds <= 0.0f)
		{
			return Events;
		}

		// Zustandswechsel: Die Dauer streut um den Mittelwert der Woche; wach wird das Kind erst ab SSW 36
		B.StateSecondsLeft -= DeltaSeconds;
		if (B.StateSecondsLeft <= 0.0f)
		{
			const float QuietMean = MeanStateSeconds(EGenesisFetalState::Quiet, Weeks, M);
			const float ActiveMean = MeanStateSeconds(EGenesisFetalState::Active, Weeks, M);
			const float AwakeMean = MeanStateSeconds(EGenesisFetalState::ActiveAwake, Weeks, M);
			EGenesisFetalState Next = EGenesisFetalState::Quiet;
			if (B.State == EGenesisFetalState::Quiet)
			{
				Next = EGenesisFetalState::Active;
			}
			else if (B.State == EGenesisFetalState::Active && AwakeMean > 0.0f)
			{
				// Ab SSW 36 folgt auf den aktiven Schlaf eine Wachphase (so ergeben sich 24 / 65 / 11 %), davor noch nicht
				const float WakeChance = FMath::Clamp(AwakeMean / (9.6f * 60.0f), 0.0f, 1.0f);
				Next = Random.FRand() < WakeChance ? EGenesisFetalState::ActiveAwake : EGenesisFetalState::Quiet;
			}
			// Geht die Mutter, wiegt sie das Kind: Aktivphasen werden kürzer
			const float Mean = Next == EGenesisFetalState::Quiet ? QuietMean
				: (Next == EGenesisFetalState::ActiveAwake ? AwakeMean : ActiveMean * (1.0f - 0.4f * MaternalMovement));
			B.State = Next;
			B.StateSecondsLeft = FMath::Max(20.0f, Mean * Random.FRandRange(0.5f, 1.5f));
		}
		B.SecondsInState[StateIndex(B.State)] += DeltaSeconds;

		const bool bActive = B.State != EGenesisFetalState::Quiet;
		const float Awake = B.State == EGenesisFetalState::ActiveAwake ? 1.0f : 0.0f;
		auto Maybe = [&](EGenesisFetalEvent Event, float Onset, float PerMinute)
		{
			if (Weeks >= Onset && Random.FRand() < PerMinute * DeltaSeconds / 60.0f)
			{
				Events.Add(Event);
			}
		};

		// Ereignisse je Minute; in Ruhe fast nichts (Schreckbewegungen bleiben). Häufigkeiten nach dem Verlauf bei
		// de Vries 1982: Atembewegungen, Kopfdrehen, Saugen nehmen zu; Schluckauf und Schreckbewegungen erst zu, dann ab.
		const float EarlyPeak = 1.0f - Smooth(16.0f, 30.0f, Weeks) * 0.7f;
		Maybe(EGenesisFetalEvent::Startle, M.Startle, (bActive ? 0.6f : 0.15f) * EarlyPeak);
		if (bActive)
		{
			// Tritte: mehr Raum früh, mehr Kraft spät; gegen Ende dämpft die Enge
			const float Space = 1.0f - 0.35f * Smooth(34.0f, 40.0f, Weeks);
			Maybe(EGenesisFetalEvent::Kick, M.IsolatedLimbs, (2.5f + 1.5f * Awake) * Space);
			Maybe(EGenesisFetalEvent::Stretch, M.IsolatedLimbs + 1.0f, 0.3f);
			Maybe(EGenesisFetalEvent::HeadTurn, M.HeadRotation, 0.6f + 0.4f * Awake);
			Maybe(EGenesisFetalEvent::Yawn, M.Yawn, 0.08f);
			Maybe(EGenesisFetalEvent::ThumbSuck, M.SuckAndSwallow, 0.25f);
			Maybe(EGenesisFetalEvent::Swallow, M.SuckAndSwallow, 0.8f);
			Maybe(EGenesisFetalEvent::BreathingBout, M.BreathingMovements, 0.4f * Smooth(M.BreathingMovements, 30.0f, Weeks));
		}

		// Schluckauf kommt in Serien von einigen Minuten (etwa eine Serie alle 1–2 Stunden), ein Hicks alle 2–4 Sekunden
		if (Weeks >= M.Hiccup)
		{
			if (B.HiccupSecondsLeft <= 0.0f && Random.FRand() < 0.01f * EarlyPeak * DeltaSeconds / 60.0f)
			{
				B.HiccupSecondsLeft = Random.FRandRange(120.0f, 480.0f);
				B.NextHiccupIn = 0.0f;
			}
			if (B.HiccupSecondsLeft > 0.0f)
			{
				B.HiccupSecondsLeft -= DeltaSeconds;
				B.NextHiccupIn -= DeltaSeconds;
				if (B.NextHiccupIn <= 0.0f)
				{
					Events.Add(EGenesisFetalEvent::Hiccup);
					B.NextHiccupIn = Random.FRandRange(2.0f, 4.0f);
				}
			}
		}
		return Events;
	}

	float ActionOnsetWeeks(EGenesisFetalAction Action, const FGenesisFetalMilestones& M)
	{
		// de Vries, Visser & Prechtl 1982; Augen Doc 34; Greifen Jakobovits 2007 (Docs/37)
		switch (Action)
		{
		case EGenesisFetalAction::MoveBody:    return M.GeneralMovement;
		case EGenesisFetalAction::MoveHand:    return M.IsolatedLimbs;
		case EGenesisFetalAction::TurnHead:    return M.HeadRotation;
		case EGenesisFetalAction::Kick:        return M.IsolatedLimbs;
		case EGenesisFetalAction::Stretch:     return M.GeneralMovement;
		case EGenesisFetalAction::Yawn:        return M.Yawn;
		case EGenesisFetalAction::HandToMouth: return M.HandFaceContact;
		case EGenesisFetalAction::Swallow:     return M.SuckAndSwallow;
		case EGenesisFetalAction::Grasp:       return M.Grasp;
		case EGenesisFetalAction::OpenEyes:    return M.EyesBeginToOpen;
		default:                               return 99.0f;
		}
	}

	bool CanPerform(EGenesisFetalAction Action, float Weeks, const FGenesisFetalMilestones& M)
	{
		return Weeks >= ActionOnsetWeeks(Action, M);
	}

	float RoomToMove(float Weeks, const FGenesisFetalMilestones& M)
	{
		// Bis zum letzten Drittel schwebt das Kind; am Termin füllt es die Höhle aus (etwa ein Drittel Spielraum bleibt)
		return 1.0f - 0.7f * Smooth(M.SpaceNarrows, 40.0f, Weeks);
	}

	FString GetActionName(EGenesisFetalAction Action)
	{
		switch (Action)
		{
		case EGenesisFetalAction::MoveBody:    return TEXT("bewegen");
		case EGenesisFetalAction::MoveHand:    return TEXT("Hand bewegen");
		case EGenesisFetalAction::TurnHead:    return TEXT("Kopf drehen");
		case EGenesisFetalAction::Kick:        return TEXT("strampeln");
		case EGenesisFetalAction::Stretch:     return TEXT("strecken");
		case EGenesisFetalAction::Yawn:        return TEXT("gähnen");
		case EGenesisFetalAction::HandToMouth: return TEXT("Daumen zum Mund");
		case EGenesisFetalAction::Swallow:     return TEXT("schlucken");
		case EGenesisFetalAction::Grasp:       return TEXT("greifen");
		case EGenesisFetalAction::OpenEyes:    return TEXT("Augen öffnen");
		default:                               return TEXT("?");
		}
	}
}
