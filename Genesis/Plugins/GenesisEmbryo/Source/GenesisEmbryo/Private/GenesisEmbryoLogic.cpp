// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoLogic.h"
#include "GenesisRandom.h"

namespace GenesisEmbryoLogic
{
	namespace
	{
		/** Fester Rechenschritt. Feiner als jede sichtbare Änderung, grob genug für eine Woche in einem Rutsch. */
		constexpr double StepHours = 0.25;

		/** Deterministischer Strom für ein Ereignis: gleiche Zelle, gleiche Runde → gleicher Zufall. */
		FGenesisRandomStream StreamFor(const FGenesisEmbryoState& State, int32 CellIndex, int32 Generation, uint64 Salt)
		{
			const uint64 Cell = GenesisHash::Combine(static_cast<uint64>(CellIndex), static_cast<uint64>(Generation));
			return FGenesisRandomStream(GenesisHash::Combine(GenesisHash::Combine(State.Seed, Cell), Salt));
		}

		FVector UnitVector(FGenesisRandomStream& Rng)
		{
			// Gleichverteilt auf der Kugel: Höhe gleichverteilt, Winkel gleichverteilt
			const float Z = Rng.FRandRange(-1.0f, 1.0f);
			const float Angle = Rng.FRandRange(0.0f, 2.0f * PI);
			const float R = FMath::Sqrt(FMath::Max(0.0f, 1.0f - Z * Z));
			return FVector(R * FMath::Cos(Angle), R * FMath::Sin(Angle), Z);
		}

		/**
		 * Innenradius, der dem Keim zur Verfügung steht. Bis zur Morula ist das die Zona;
		 * die Blastozyste dehnt sich darüber hinaus – das ist das erste Mal, dass der Keim wächst.
		 */
		float AvailableRadius(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
		{
			return Tuning.InnerRadiusUm * (1.0f + 0.35f * State.Cavity);
		}

		/** Radius, den n gleich große Zellen haben, wenn das Gesamtvolumen gleich bleibt. */
		float RadiusForCount(float StartRadius, int32 Count)
		{
			return StartRadius * FMath::Pow(1.0f / FMath::Max(1, Count), 1.0f / 3.0f);
		}

		/**
		 * Zellen dürfen sich nicht durchdringen und nicht aus der Zona treten.
		 * Wenige Iterationen je Schritt reichen – der Keim hat Zeit.
		 */
		void Relax(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning, int32 Iterations)
		{
			const int32 Count = State.Cells.Num();
			if (Count < 2)
			{
				if (Count == 1)
				{
					State.Cells[0].Position = FVector::ZeroVector;
				}
				return;
			}

			// Bei Kompaktierung rücken die Zellen enger zusammen, als ihre Kugelform erlaubt (sie verformen sich)
			const float Overlap = 1.0f - 0.22f * State.Compaction;

			for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
			{
				for (int32 A = 0; A < Count; ++A)
				{
					for (int32 B = A + 1; B < Count; ++B)
					{
						FVector Delta = State.Cells[B].Position - State.Cells[A].Position;
						const double Distance = Delta.Size();
						const float Minimum = (State.Cells[A].RadiusUm + State.Cells[B].RadiusUm) * Overlap;
						if (Distance < UE_DOUBLE_SMALL_NUMBER)
						{
							Delta = FVector(0.0, 0.0, 1.0);
						}
						else if (Distance >= Minimum)
						{
							continue;
						}
						const FVector Push = Delta.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector) * (Minimum - Distance) * 0.5;
						State.Cells[A].Position -= Push;
						State.Cells[B].Position += Push;
					}
				}

				// In der Zona bleiben; in der Blastozyste liegen die Zellen ohnehin außen
				for (FGenesisBlastomere& Cell : State.Cells)
				{
					const float Limit = AvailableRadius(State, Tuning) - Cell.RadiusUm;
					const double Radius = Cell.Position.Size();
					if (Radius > Limit && Radius > UE_DOUBLE_SMALL_NUMBER)
					{
						Cell.Position *= Limit / Radius;
					}
				}
			}
		}

		/** Teilt eine Zelle in zwei Tochterzellen. Die Teilungsebene ist zufällig, aber deterministisch. */
		void Divide(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning, int32 Index)
		{
			FGenesisBlastomere Parent = State.Cells[Index];
			FGenesisRandomStream Rng = StreamFor(State, Index, Parent.Generation, 0x5CA1Eull);

			const int32 NewCount = State.Cells.Num() + 1;
			const float Radius = FMath::Max(Tuning.MinimumBlastomereRadiusUm, RadiusForCount(55.0f, NewCount));
			const FVector Axis = UnitVector(Rng);
			const FVector Offset = Axis * Radius * 0.62f;

			FGenesisBlastomere First = Parent;
			First.Position = Parent.Position + Offset;
			First.RadiusUm = Radius;
			First.Generation = Parent.Generation + 1;
			First.Tint = FMath::Clamp(Parent.Tint + Rng.Gaussian(0.0f, 0.12f), 0.05f, 1.0f);
			// Ab der dritten Runde sind die Zellzyklen länger (Genomaktivierung)
			const FFloatInterval& Interval = Parent.Generation + 1 <= 1 ? Tuning.CleavageIntervalHours : Tuning.LaterCleavageIntervalHours;
			First.HoursToDivision = Rng.FRandRange(Interval.Min, Interval.Max);

			FGenesisBlastomere Second = Parent;
			Second.Position = Parent.Position - Offset;
			Second.RadiusUm = Radius;
			Second.Generation = Parent.Generation + 1;
			Second.Tint = FMath::Clamp(Parent.Tint + Rng.Gaussian(0.0f, 0.12f), 0.05f, 1.0f);
			// Teilungen laufen nicht synchron – deshalb gibt es auch 3-, 5- und 7-Zell-Stadien
			Second.HoursToDivision = Rng.FRandRange(Interval.Min, Interval.Max) * 1.15f;

			State.Cells[Index] = First;
			State.Cells.Add(Second);

			// Alle vorhandenen Zellen schrumpfen mit: Der Keim wächst nicht, er teilt sich nur auf
			for (FGenesisBlastomere& Cell : State.Cells)
			{
				Cell.RadiusUm = Radius;
			}

			// Fragmentierung und Stillstand gehören in die ersten Tage: Dort schaltet das eigene Erbgut
			// die Entwicklung an. Später teilt sich der Keim zuverlässig weiter.
			const bool bInRiskWindow = NewCount <= Tuning.RiskWindowCellCount;

			// Unsaubere Teilungen hinterlassen Zelltrümmer; kräftige Zellen teilen sich sauberer
			const float Cleanliness = FMath::Clamp(0.35f + 0.65f * State.Vitality, 0.0f, 1.0f);
			if (bInRiskWindow)
			{
				State.Fragmentation = FMath::Clamp(State.Fragmentation + Tuning.FragmentationPerDivision * (1.0f - Cleanliness), 0.0f, 1.0f);
			}

			// Risiko des Stillstands – für den Keim des Spielers ausgesetzt, sonst gäbe es kein Leben zu spielen
			if (!State.bPlayerEmbryo && bInRiskWindow)
			{
				const float Risk = Tuning.ArrestChancePerDivision * (1.6f - 0.8f * State.Vitality - 0.4f * State.Resilience);
				if (Rng.Bernoulli(FMath::Clamp(Risk, 0.0f, 1.0f)))
				{
					State.Stage = EGenesisEmbryoStage::Arrested;
					State.ArrestReason = Rng.Bernoulli(0.6f) ? EGenesisEmbryoArrestReason::Aneuploidy
						: (State.Fragmentation > 0.3f ? EGenesisEmbryoArrestReason::Fragmentation : EGenesisEmbryoArrestReason::EnergyFailure);
				}
			}
		}

		/** Weist dem Keim Embryoblast und Trophoblast zu: eine Seite wird zum Menschen, der Rest zum Mutterkuchen. */
		void AssignCellFates(FGenesisEmbryoState& State)
		{
			if (State.Cells.Num() == 0)
			{
				return;
			}

			FGenesisRandomStream Rng(GenesisHash::Combine(State.Seed, 0xB1A57ull));
			const FVector Pole = UnitVector(Rng);

			// Ein Drittel der Zellen bildet den Embryoblasten – die, die beim Kompaktieren innen lagen
			TArray<int32> Order;
			Order.Reserve(State.Cells.Num());
			for (int32 Index = 0; Index < State.Cells.Num(); ++Index)
			{
				Order.Add(Index);
			}
			Order.Sort([&State, &Pole](int32 A, int32 B)
			{
				return FVector::DotProduct(State.Cells[A].Position, Pole) > FVector::DotProduct(State.Cells[B].Position, Pole);
			});

			const int32 InnerCount = FMath::Max(3, FMath::RoundToInt(0.3f * State.Cells.Num()));
			for (int32 Rank = 0; Rank < Order.Num(); ++Rank)
			{
				State.Cells[Order[Rank]].bInnerCellMass = Rank < InnerCount;
			}
		}

		/** Ordnet die Zellen der Blastozyste an: Trophoblast als Hülle, Embryoblast als Knoten an einem Pol. */
		void ShapeBlastocyst(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning, float Alpha)
		{
			if (State.Cells.Num() == 0)
			{
				return;
			}

			FGenesisRandomStream Rng(GenesisHash::Combine(State.Seed, 0xB1A57ull));
			const FVector Pole = UnitVector(Rng);
			// Die Zellen legen sich als einlagige Hülle um den Hohlraum – daher der Name Blastozyste
			const float Shell = AvailableRadius(State, Tuning);

			for (FGenesisBlastomere& Cell : State.Cells)
			{
				FVector Target;
				if (Cell.bInnerCellMass)
				{
					// Der Embryoblast sitzt als Zellknoten innen an einem Pol
					const FVector Direction = (Cell.Position - Pole * Shell * 0.55f).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, Pole);
					Target = Pole * (Shell - Cell.RadiusUm * 1.1f) + Direction * Cell.RadiusUm * 1.2f;
				}
				else
				{
					const FVector Direction = Cell.Position.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, -Pole);
					Target = Direction * (Shell - Cell.RadiusUm * 0.5f);
				}
				Cell.Position = FMath::Lerp(Cell.Position, Target, Alpha);
			}
		}
	}

	FGenesisEmbryoState CreateZygote(const FGuid& EntityId, const FGuid& GenomeId, float Vitality, float Resilience,
		const FGenesisTimestamp& FusionTime, const FGenesisEmbryoTuning& Tuning)
	{
		FGenesisEmbryoState State;
		State.EntityId = EntityId;
		State.GenomeId = GenomeId;
		State.Seed = GenesisHash::Combine(GenesisHash::FromGuid(GenomeId), 0xE11B2A0ull);
		State.Vitality = FMath::Clamp(Vitality, 0.0f, 1.0f);
		State.Resilience = FMath::Clamp(Resilience, 0.0f, 1.0f);
		State.FusionTime = FusionTime;

		FGenesisRandomStream Rng(State.Seed);
		FGenesisBlastomere Zygote;
		Zygote.Position = FVector::ZeroVector;
		Zygote.RadiusUm = 55.0f;
		Zygote.Generation = 0;
		Zygote.Tint = Rng.FRandRange(0.35f, 0.65f);
		// Kräftige Zellen teilen sich etwas früher – ein früher erster Schnitt gilt als gutes Zeichen
		Zygote.HoursToDivision = FMath::Lerp(Tuning.FirstCleavageHours.Max, Tuning.FirstCleavageHours.Min, State.Vitality);
		State.Cells.Add(Zygote);
		return State;
	}

	bool Advance(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning, double Hours)
	{
		const EGenesisEmbryoStage StartStage = State.Stage;
		double Remaining = FMath::Max(0.0, Hours);

		while (Remaining > 0.0)
		{
			const double Step = FMath::Min(StepHours, Remaining);
			Remaining -= Step;
			State.HoursSinceFusion += Step;

			if (State.Stage == EGenesisEmbryoStage::Arrested || State.Stage == EGenesisEmbryoStage::Implanted)
			{
				continue;
			}

			// 1. Teilungen – jede Zelle hat ihre eigene Uhr
			const bool bCanDivide = State.Stage <= EGenesisEmbryoStage::Blastocyst;
			if (bCanDivide)
			{
				const int32 CountBefore = State.Cells.Num();
				for (int32 Index = 0; Index < CountBefore && State.IsAlive(); ++Index)
				{
					State.Cells[Index].HoursToDivision -= static_cast<float>(Step);
					if (State.Cells[Index].HoursToDivision <= 0.0f && State.Cells[Index].RadiusUm > Tuning.MinimumBlastomereRadiusUm)
					{
						Divide(State, Tuning, Index);
					}
				}
			}

			if (!State.IsAlive())
			{
				continue;
			}

			// 2. Stufen
			const int32 CellCount = State.Cells.Num();
			if (State.Stage == EGenesisEmbryoStage::Zygote && CellCount > 1)
			{
				State.Stage = EGenesisEmbryoStage::Cleavage;
			}
			if (State.Stage == EGenesisEmbryoStage::Cleavage && CellCount >= Tuning.CompactionCellCount)
			{
				State.Stage = EGenesisEmbryoStage::Morula;
			}
			if (State.Stage == EGenesisEmbryoStage::Morula)
			{
				State.Compaction = FMath::Clamp(State.Compaction + static_cast<float>(Step) / FMath::Max(1.0f, Tuning.CompactionHours), 0.0f, 1.0f);
				if (State.Compaction >= 1.0f && CellCount >= Tuning.CavitationCellCount)
				{
					State.Stage = EGenesisEmbryoStage::Blastocyst;
					AssignCellFates(State);
				}
			}
			if (State.Stage >= EGenesisEmbryoStage::Blastocyst && State.Stage <= EGenesisEmbryoStage::Hatching)
			{
				State.Cavity = FMath::Clamp(State.Cavity + static_cast<float>(Step) / FMath::Max(1.0f, Tuning.ExpansionHours), 0.0f, 1.0f);
				ShapeBlastocyst(State, Tuning, FMath::Clamp(static_cast<float>(Step) * 0.6f, 0.0f, 1.0f));

				if (State.Stage == EGenesisEmbryoStage::Blastocyst && State.Cavity >= Tuning.HatchingCavity)
				{
					State.Stage = EGenesisEmbryoStage::Hatching;
				}
				if (State.Stage == EGenesisEmbryoStage::Hatching)
				{
					// Die Zona wird beim Ausdehnen dünner, bis sie aufreißt
					State.ZonaThicknessUm = FMath::Max(0.0f, State.ZonaThicknessUm - static_cast<float>(Step) * 0.55f);
					if (State.ZonaThicknessUm <= 0.0f)
					{
						State.Stage = EGenesisEmbryoStage::Implanting;
					}
				}
			}
			if (State.Stage == EGenesisEmbryoStage::Implanting)
			{
				State.Implantation = FMath::Clamp(State.Implantation + static_cast<float>(Step) / FMath::Max(1.0f, Tuning.ImplantationHours), 0.0f, 1.0f);
				if (State.Implantation >= 1.0f)
				{
					State.Stage = EGenesisEmbryoStage::Implanted;
				}
			}

			// 3. Form
			Relax(State, Tuning, State.Stage >= EGenesisEmbryoStage::Blastocyst ? 1 : 2);

			// 4. Qualität: Trümmer und zu langsame Entwicklung kosten
			const float Expected = FMath::Clamp(static_cast<float>(State.HoursSinceFusion) / 120.0f, 0.0f, 1.0f);
			const float Reached = FMath::Clamp(static_cast<float>(CellCount) / 32.0f, 0.0f, 1.0f);
			const float Pace = FMath::Clamp(1.0f - 0.5f * FMath::Max(0.0f, Expected - Reached), 0.0f, 1.0f);
			State.Quality = FMath::Clamp((1.0f - 0.7f * State.Fragmentation) * Pace, 0.0f, 1.0f);
		}

		return State.Stage != StartStage;
	}

	float GetDevelopmentQuality(const FGenesisEmbryoState& State)
	{
		if (!State.IsAlive())
		{
			return 0.0f;
		}
		// Der Embryoblast ist die eigentliche Anlage des Menschen: zu wenige Zellen dort wiegen schwer
		const int32 Inner = CountInnerCellMass(State);
		const float InnerFactor = FMath::Clamp(static_cast<float>(Inner) / 12.0f, 0.4f, 1.0f);
		return FMath::Clamp(State.Quality * (0.7f + 0.3f * InnerFactor), 0.0f, 1.0f);
	}

	int32 CountInnerCellMass(const FGenesisEmbryoState& State)
	{
		int32 Count = 0;
		for (const FGenesisBlastomere& Cell : State.Cells)
		{
			Count += Cell.bInnerCellMass ? 1 : 0;
		}
		return Count;
	}

	void GetNucleusDisplay(const FGenesisEmbryoState& State, int32 CellIndex, const FGenesisEmbryoTuning& Tuning,
		float& OutVisibility, bool& bOutPronuclei)
	{
		OutVisibility = 0.0f;
		bOutPronuclei = false;
		if (!State.Cells.IsValidIndex(CellIndex) || !State.IsAlive())
		{
			return;
		}

		const float Hours = static_cast<float>(State.HoursSinceFusion);
		if (State.Cells.Num() == 1)
		{
			// Zygote: zwei Vorkerne, dann Syngamie – danach ist bis zur ersten Teilung kein Kern zu sehen
			bOutPronuclei = true;
			const float In = FMath::Clamp((Hours - Tuning.PronucleiAppearHours) / 1.5f, 0.0f, 1.0f);
			const float Out = FMath::Clamp((Tuning.PronucleiFadeHours - Hours) / 1.0f, 0.0f, 1.0f);
			OutVisibility = FMath::Min(In, Out);
			return;
		}

		// Furchungszelle: Kern sichtbar, bis sich vor der Teilung die Kernhülle auflöst
		const float ToDivision = State.Cells[CellIndex].HoursToDivision;
		const float Mitosis = FMath::Max(0.1f, Tuning.MitosisHours);
		const bool bStillDividing = State.Cells[CellIndex].RadiusUm > Tuning.MinimumBlastomereRadiusUm;
		OutVisibility = State.Stage <= EGenesisEmbryoStage::Blastocyst && bStillDividing
			? FMath::Clamp((ToDivision - 0.4f * Mitosis) / (0.6f * Mitosis), 0.0f, 1.0f)
			: 1.0f;
	}

	FString GetStageName(EGenesisEmbryoStage Stage)
	{
		switch (Stage)
		{
		case EGenesisEmbryoStage::Zygote: return TEXT("Zygote");
		case EGenesisEmbryoStage::Cleavage: return TEXT("Furchung");
		case EGenesisEmbryoStage::Morula: return TEXT("Morula");
		case EGenesisEmbryoStage::Blastocyst: return TEXT("Blastozyste");
		case EGenesisEmbryoStage::Hatching: return TEXT("Schlüpfen");
		case EGenesisEmbryoStage::Implanting: return TEXT("Einnistung");
		case EGenesisEmbryoStage::Implanted: return TEXT("eingenistet");
		case EGenesisEmbryoStage::Arrested: return TEXT("Stillstand");
		default: return TEXT("unbekannt");
		}
	}

	FString GetArrestReasonName(EGenesisEmbryoArrestReason Reason)
	{
		switch (Reason)
		{
		case EGenesisEmbryoArrestReason::Aneuploidy: return TEXT("Chromosomen-Fehlverteilung");
		case EGenesisEmbryoArrestReason::EnergyFailure: return TEXT("Energiemangel");
		case EGenesisEmbryoArrestReason::Fragmentation: return TEXT("Fragmentierung");
		case EGenesisEmbryoArrestReason::ImplantationFailed: return TEXT("Einnistung gescheitert");
		default: return TEXT("keiner");
		}
	}
}
