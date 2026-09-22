// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoLogic.h"
#include "GenesisRandom.h"

namespace GenesisEmbryoLogic
{
	namespace
	{
		/** Fester Rechenschritt. Feiner als jede sichtbare Änderung, grob genug für eine Woche in einem Rutsch. */
		constexpr double StepHours = 0.25;

		/** Wert aus einer Stützstellentabelle (Stunden → Wert), dazwischen linear, außerhalb gehalten. */
		float NidationTable(std::initializer_list<FVector2f> List, float Hours)
		{
			const FVector2f* Points = List.begin();
			const int32 Count = static_cast<int32>(List.size());
			if (Hours <= Points[0].X)
			{
				return Points[0].Y;
			}
			for (int32 Index = 1; Index < Count; ++Index)
			{
				if (Hours <= Points[Index].X)
				{
					const float Alpha = (Hours - Points[Index - 1].X) / FMath::Max(1.0e-3f, Points[Index].X - Points[Index - 1].X);
					return FMath::Lerp(Points[Index - 1].Y, Points[Index].Y, Alpha);
				}
			}
			return Points[Count - 1].Y;
		}

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

		/**
		 * Die zweite Woche. Alle Größen folgen der Uhr des mittleren Keims (Nominal): Ein Keim, der sich später anlegt,
		 * durchläuft dieselben Stufen entsprechend später. Jede Größe rechnet sich aus der Uhr, statt sich Schritt für
		 * Schritt aufzusummieren – dasselbe Ergebnis bei jeder Schrittweite und nach dem Laden.
		 */
		void AdvanceNidation(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
		{
			FGenesisImplantationState& Nid = State.Nidation;
			const float T = NominalImplantationHours(State, Tuning);
			if (T < Tuning.NominalAppositionHours)
			{
				Nid.Phase = EGenesisImplantationPhase::None;
				return;
			}

			// Stufen
			EGenesisImplantationPhase Phase = EGenesisImplantationPhase::Apposition;
			if (T >= Tuning.AdhesionHours) Phase = EGenesisImplantationPhase::Adhesion;
			if (T >= Tuning.InvasionHours) Phase = EGenesisImplantationPhase::Invasion;
			if (T >= Tuning.LacunarHours) Phase = EGenesisImplantationPhase::Lacunar;
			if (T >= Tuning.EmbeddedHours) Phase = EGenesisImplantationPhase::Embedded;
			if (T >= Tuning.UteroplacentalHours) Phase = EGenesisImplantationPhase::Uteroplacental;
			if (T >= Tuning.PrimaryVilliHours) Phase = EGenesisImplantationPhase::PrimaryVilli;
			Nid.Phase = Phase;

			// Größe: 0,2 mm beim Anlegen; Hertig-Rock-Präparate ~0,45 mm an Tag 9,5, ~0,9 mm an Tag 12
			Nid.ConceptusDiameterUm = NidationTable({ {144.0f, 200.0f}, {168.0f, 230.0f}, {216.0f, 330.0f}, {240.0f, 450.0f}, {264.0f, 600.0f}, {288.0f, 900.0f}, {312.0f, 1200.0f} }, T);

			// Versinken: vom Anheften bis Tag 10 ganz unter die Oberfläche, danach etwas tiefer unter dem nachgewachsenen Epithel
			Nid.Embedded = FMath::SmoothStep(Tuning.AdhesionHours, Tuning.EmbeddedHours, T);
			Nid.DepthUm = Nid.Embedded * Nid.ConceptusDiameterUm + 40.0f * FMath::SmoothStep(Tuning.EmbeddedHours, Tuning.PrimaryVilliHours, T);

			// Synzytium: entsteht mit der Invasion am Embryonalpol und wird zur dicken, vielkernigen Front
			Nid.SyncytiumThicknessUm = T < Tuning.InvasionHours ? 0.0f
				: NidationTable({ {168.0f, 5.0f}, {216.0f, 60.0f}, {264.0f, 90.0f}, {312.0f, 120.0f} }, T);

			// Lakunen öffnen sich ab Tag 8,5; ab Tag 10,5 fließt mütterliches Blut hinein, ab Tag 12 hindurch
			Nid.Lacunae = FMath::RoundToInt(40.0f * FMath::SmoothStep(Tuning.LacunarHours - 12.0f, Tuning.UteroplacentalHours, T));
			Nid.LacunarBlood = FMath::SmoothStep(Tuning.UteroplacentalHours - 12.0f, Tuning.UteroplacentalHours + 24.0f, T);

			// Oberfläche: Fibrinpfropf (Tag 9–10), darüber wächst das Epithel wieder zu (bis Tag 12)
			Nid.SurfaceClosure = 0.5f * FMath::SmoothStep(Tuning.LacunarHours, Tuning.EmbeddedHours, T)
				+ 0.5f * FMath::SmoothStep(Tuning.EmbeddedHours, Tuning.EmbeddedHours + 48.0f, T);
			Nid.Decidualization = FMath::SmoothStep(Tuning.InvasionHours, Tuning.PrimaryVilliHours, T);

			// Zweiblättrige Keimscheibe aus dem Embryoblasten (ab Tag 7,5). Die Zellen teilen sich wieder, rund einmal
			// am Tag; Carnegie 6: Scheibe ~0,2 mm.
			const float DiscStart = Tuning.InvasionHours + 12.0f;
			if (T >= DiscStart)
			{
				const int32 Inner = CountInnerCellMass(State);
				const float Epi0 = FMath::Clamp(0.3f * Inner, 8.0f, 24.0f);
				const float Hypo0 = FMath::Clamp(0.25f * Inner, 6.0f, 20.0f);
				Nid.EpiblastCells = FMath::RoundToInt(Epi0 * FMath::Pow(2.0f, (T - DiscStart) / 20.0f));
				Nid.HypoblastCells = FMath::RoundToInt(Hypo0 * FMath::Pow(2.0f, (T - DiscStart) / 24.0f));
				Nid.DiscDiameterUm = NidationTable({ {180.0f, 80.0f}, {240.0f, 130.0f}, {312.0f, 200.0f} }, T);
			}

			// Höhlen: Amnion (Tag 8), primärer Dottersack mit Heuser-Membran (Tag 9), der sich an Tag 12–13 abschnürt;
			// der sekundäre (definitive) Dottersack bleibt. Extraembryonales Mesoderm, darin die Chorionhöhle.
			Nid.AmnioticCavity = FMath::SmoothStep(Tuning.InvasionHours + 18.0f, Tuning.LacunarHours, T);
			Nid.SecondaryYolkSac = FMath::SmoothStep(Tuning.PrimaryVilliHours - 24.0f, Tuning.PrimaryVilliHours, T);
			Nid.PrimaryYolkSac = FMath::SmoothStep(Tuning.LacunarHours - 12.0f, Tuning.LacunarHours + 12.0f, T) * (1.0f - Nid.SecondaryYolkSac);
			Nid.ExtraembryonicMesoderm = FMath::SmoothStep(Tuning.EmbeddedHours, Tuning.UteroplacentalHours + 12.0f, T);
			Nid.ChorionicCavity = FMath::SmoothStep(Tuning.UteroplacentalHours + 6.0f, Tuning.PrimaryVilliHours, T);
			Nid.PrimaryVilli = FMath::RoundToInt(30.0f * FMath::SmoothStep(Tuning.PrimaryVilliHours - 16.0f, Tuning.PrimaryVilliHours, T));

			// Gesamtfortschritt für Anzeige und Regie
			State.Implantation = FMath::Clamp((T - Tuning.NominalAppositionHours) / FMath::Max(1.0f, Tuning.PrimaryVilliHours - Tuning.NominalAppositionHours), 0.0f, 1.0f);
		}

		/** hCG im Blut der Mutter: vom Synzytium gebildet, exponentiell steigend. */
		void UpdateHcg(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
		{
			const float T = NominalImplantationHours(State, Tuning);
			State.Nidation.HcgMilliIU = State.Nidation.AppositionAtHours <= 0.0f || T < Tuning.InvasionHours ? 0.0f
				: 5.0f * FMath::Pow(2.0f, (T - Tuning.HcgDetectableHours) / FMath::Max(1.0f, Tuning.HcgDoublingHours));
		}

		/**
		 * Beim Anheften entscheidet sich, ob die Schwangerschaft bleibt (Wilcox 1999). Der Zeitpunkt der Einnistung ist
		 * der stärkste bekannte Einzelfaktor; die Entwicklungsqualität des Keims (Chromosomen, Energie) verschiebt ihn.
		 */
		void RollImplantationRisk(FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
		{
			FGenesisImplantationState& Nid = State.Nidation;
			Nid.ImplantationDayPostOvulation = ImplantationDayPostOvulation(State, Tuning);
			const float Quality = GetDevelopmentQuality(State);
			Nid.EarlyLossRisk = FMath::Clamp(EarlyLossRiskForDay(Nid.ImplantationDayPostOvulation) * FMath::Lerp(1.4f, 0.7f, Quality), 0.0f, 0.95f);
			Nid.bRiskRolled = true;
			FGenesisRandomStream Rng(GenesisHash::Combine(State.Seed, 0x1A91A7ull));
			Nid.bWillFail = !State.bPlayerEmbryo && Rng.Bernoulli(Nid.EarlyLossRisk);
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

			if (State.Stage == EGenesisEmbryoStage::Implanted)
			{
				// Das hCG steigt weiter – es hält den Gelbkörper, bis die Plazenta selbst Progesteron bildet
				UpdateHcg(State, Tuning);
				continue;
			}
			if (State.Stage == EGenesisEmbryoStage::Arrested)
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
						// Geschlüpft treibt der Keim noch einige Stunden frei, dann legt er sich an
						State.Nidation.AppositionAtHours = static_cast<float>(State.HoursSinceFusion) + Tuning.FloatAfterHatchingHours;
					}
				}
			}
			if (State.Stage == EGenesisEmbryoStage::Implanting)
			{
				AdvanceNidation(State, Tuning);
				UpdateHcg(State, Tuning);
				if (!State.Nidation.bRiskRolled && State.Nidation.Phase >= EGenesisImplantationPhase::Adhesion)
				{
					RollImplantationRisk(State, Tuning);
				}
				// Wer scheitert, scheitert dort, wo das Synzytium mütterliches Blut erreichen müsste
				if (State.Nidation.bWillFail && State.Nidation.Phase >= EGenesisImplantationPhase::Uteroplacental)
				{
					State.Stage = EGenesisEmbryoStage::Arrested;
					State.ArrestReason = EGenesisEmbryoArrestReason::ImplantationFailed;
					State.Nidation.HcgMilliIU = 0.0f;
					continue;
				}
				if (State.Nidation.Phase == EGenesisImplantationPhase::PrimaryVilli)
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

	float NominalImplantationHours(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
	{
		if (State.Nidation.AppositionAtHours <= 0.0f)
		{
			return 0.0f;
		}
		return static_cast<float>(State.HoursSinceFusion) - State.Nidation.AppositionAtHours + Tuning.NominalAppositionHours;
	}

	float ImplantationDayPostOvulation(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
	{
		const float Shift = State.Nidation.AppositionAtHours - Tuning.NominalAppositionHours;
		return (Shift + Tuning.InvasionHours + Tuning.HcgFirstRiseAfterInvasionHours + Tuning.HoursFusionAfterOvulation) / 24.0f;
	}

	float EarlyLossRiskForDay(float DayPostOvulation)
	{
		// Wilcox, Baird, Weinberg 1999: bis Tag 9 13 %, Tag 10 26 %, Tag 11 52 %, danach 82 %
		return NidationTable({ {9.0f, 0.13f}, {10.0f, 0.26f}, {11.0f, 0.52f}, {12.0f, 0.82f} }, DayPostOvulation);
	}

	FString GetImplantationPhaseName(EGenesisImplantationPhase Phase)
	{
		switch (Phase)
		{
		case EGenesisImplantationPhase::None: return TEXT("frei in der Gebärmutter");
		case EGenesisImplantationPhase::Apposition: return TEXT("Anlagerung");
		case EGenesisImplantationPhase::Adhesion: return TEXT("Anheftung");
		case EGenesisImplantationPhase::Invasion: return TEXT("Invasion");
		case EGenesisImplantationPhase::Lacunar: return TEXT("Lakunenstadium");
		case EGenesisImplantationPhase::Embedded: return TEXT("ganz eingebettet");
		case EGenesisImplantationPhase::Uteroplacental: return TEXT("uteroplazentarer Kreislauf");
		case EGenesisImplantationPhase::PrimaryVilli: return TEXT("Primärzotten");
		default: return TEXT("unbekannt");
		}
	}

	FString DescribeImplantation(EGenesisImplantationPhase Phase)
	{
		switch (Phase)
		{
		case EGenesisImplantationPhase::None:
			return TEXT("Geschlüpft – der Keim treibt frei in der Gebärmutter, gut einen Fünftelmillimeter groß.");
		case EGenesisImplantationPhase::Apposition:
			return TEXT("Tag 6 – der Keim legt sich mit dem Embryoblast-Pol an die Schleimhaut der Gebärmutter.");
		case EGenesisImplantationPhase::Adhesion:
			return TEXT("Anheftung – Haftmoleküle halten ihn fest, er rollt nicht mehr ab.");
		case EGenesisImplantationPhase::Invasion:
			return TEXT("Tag 7–8 – die äußere Zellschicht verschmilzt zum Synzytium und frisst sich in die Schleimhaut. Innen trennen sich Epiblast und Hypoblast: die zweiblättrige Keimscheibe. Das hCG beginnt.");
		case EGenesisImplantationPhase::Lacunar:
			return TEXT("Tag 9 – im Synzytium öffnen sich Lakunen. Amnionhöhle und Dottersack sind da. Ein Fibrinpfropf verschließt die Eintrittsstelle.");
		case EGenesisImplantationPhase::Embedded:
			return TEXT("Tag 10 – ganz in der Schleimhaut. Darüber wächst die Oberfläche wieder zu.");
		case EGenesisImplantationPhase::Uteroplacental:
			return TEXT("Tag 11–12 – das Synzytium öffnet die Kapillaren der Mutter. Ihr Blut strömt durch die Lakunen: der erste gemeinsame Kreislauf.");
		case EGenesisImplantationPhase::PrimaryVilli:
			return TEXT("Tag 13 – die ersten Zotten wachsen. Die zweite Woche ist vorbei.");
		default:
			return FString();
		}
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
