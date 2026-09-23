// GENESIS: Der Kreislauf des Lebens

#include "GenesisFertilizationLogic.h"
#include "GenesisSpermSwimLogic.h"

namespace GenesisFertilizationLogic
{
	namespace
	{
		float Pick(const FFloatInterval& Range, float Alpha)
		{
			return FMath::Lerp(Range.Min, Range.Max, FMath::Clamp(Alpha, 0.0f, 1.0f));
		}

		FVector DirectionToOocyte(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte, double& OutDistance)
		{
			const FVector Delta = Oocyte.Position - Cell.Position;
			OutDistance = Delta.Size();
			return OutDistance > UE_KINDA_SMALL_NUMBER ? Delta / OutDistance : FVector::ForwardVector;
		}

		/** Richtung entlang der Eizelloberfläche, in die die Zelle zeigt (sonst irgendeine Senkrechte zur Einwärtsrichtung). */
		FVector TangentAt(const FVector& Heading, const FVector& Inward)
		{
			FVector Tangent = Heading - Inward * FVector::DotProduct(Heading, Inward);
			if (Tangent.SizeSquared() < 1.0e-6)
			{
				const FVector Reference = FMath::Abs(Inward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
				Tangent = FVector::CrossProduct(Reference, Inward);
			}
			return Tangent.GetSafeNormal();
		}

		/** Einwärtsrichtung um einen Winkel zur Oberfläche hin geneigt. */
		FVector Tilted(const FVector& Inward, const FVector& Tangent, float Degrees)
		{
			const float Radians = FMath::DegreesToRadians(Degrees);
			return (Inward * FMath::Cos(Radians) + Tangent * FMath::Sin(Radians)).GetSafeNormal();
		}

		void AdvanceAcrosome(FGenesisSpermCell& Cell, float Dt)
		{
			if (!Cell.bAcrosomeReacted && Cell.AcrosomeTimer > 0.0f)
			{
				Cell.AcrosomeTimer -= Dt;
				if (Cell.AcrosomeTimer <= 0.0f)
				{
					Cell.AcrosomeTimer = 0.0f;
					Cell.bAcrosomeReacted = true;
				}
			}
		}

		void EnterPerivitelline(FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte, const FGenesisFertilizationTuning& Tuning)
		{
			double Distance = 0.0;
			const FVector Inward = DirectionToOocyte(Cell, Oocyte, Distance);
			SetPhase(Cell, EGenesisSpermPhase::Perivitelline);
			// Der Kopf legt sich flach in den Spalt, die Spitze an der Membran; die Geißel folgt durch den Schlitz in der Zona
			Cell.Heading = Tilted(Inward, TangentAt(Cell.Heading, Inward), Tuning.PerivitellineTiltDegrees);
			Cell.Position = Oocyte.Position - Inward * (0.5f * (Oocyte.ZonaInnerRadiusUm + Oocyte.OoplasmRadiusUm));
			Cell.PenetrationDepthUm = Oocyte.ZonaOuterRadiusUm - Oocyte.ZonaInnerRadiusUm;
			Cell.FusionTimer = FMath::Clamp(Cell.Random.Gaussian(Tuning.PerivitellineMeanSeconds, Tuning.PerivitellineSigmaSeconds),
				Tuning.PerivitellineClampSeconds.Min, Tuning.PerivitellineClampSeconds.Max);
		}
	}

	EGenesisSpermPhase GetPhase(const FGenesisSpermCell& Cell)
	{
		return static_cast<EGenesisSpermPhase>(Cell.Phase);
	}

	void SetPhase(FGenesisSpermCell& Cell, EGenesisSpermPhase Phase)
	{
		Cell.Phase = static_cast<uint8>(Phase);
	}

	float DistanceToZona(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte)
	{
		return static_cast<float>(FVector::Dist(Cell.Position, Oocyte.Position)) - Oocyte.ZonaOuterRadiusUm;
	}

	bool IsAttached(const FGenesisSpermCell& Cell)
	{
		switch (GetPhase(Cell))
		{
		case EGenesisSpermPhase::Swimming:
			return false;
		case EGenesisSpermPhase::Blocked:
			// Abgewiesene schwimmen weiter – nur wer in der Zona steckt, hängt fest
			return Cell.PenetrationDepthUm > 0.0f;
		default:
			return true;
		}
	}

	FTransform ComputeAttachedTransform(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte)
	{
		double Distance = 0.0;
		const FVector Inward = DirectionToOocyte(Cell, Oocyte, Distance);
		const FVector Forward = Cell.Heading.GetSafeNormal(UE_SMALL_NUMBER, Inward);
		const FVector Reference = FMath::Abs(Forward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
		const FVector Side = FVector::CrossProduct(Reference, Forward).GetSafeNormal();
		const FMatrix Basis = FRotationMatrix::MakeFromXY(Forward, Side);
		return FTransform(Basis.ToQuat(), Cell.Position * GenesisMicroScale::UnitsPerMicrometer);
	}

	bool Step(TArray<FGenesisSpermCell>& Cells, FGenesisOocyteState& Oocyte, const FGenesisOviductChannel& Channel,
		const FGenesisSpermSwimTuning& SwimTuning, const FGenesisFertilizationTuning& Tuning, float DeltaSeconds,
		FGenesisFertilizationResult& OutResult)
	{
		const float Dt = FMath::Max(0.0f, DeltaSeconds);
		if (Dt <= 0.0f)
		{
			return false;
		}

		// 7. Cortikalreaktion: Nach der Verschmelzung verändert sich die Zona über Minuten
		if (Oocyte.IsFertilized())
		{
			Oocyte.SecondsSinceFusion += Dt;
			Oocyte.CorticalReaction = FMath::Clamp(Oocyte.SecondsSinceFusion / FMath::Max(0.01f, Tuning.CorticalReactionSeconds), 0.0f, 1.0f);
			Oocyte.bZonaHardened = Oocyte.CorticalReaction >= Tuning.HardeningBlockThreshold;
		}

		const float ZonaThickness = FMath::Max(1.0f, Oocyte.ZonaOuterRadiusUm - Oocyte.ZonaInnerRadiusUm);
		const float EntryCosine = FMath::Max(0.2f, FMath::Cos(FMath::DegreesToRadians(Tuning.EntryAngleDegrees)));
		bool bFusedThisStep = false;
		int32 BoundCount = 0;
		int32 PerivitellineCount = 0;
		int32 BlockedCount = 0;

		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			FGenesisSpermCell& Cell = Cells[Index];
			Cell.LifeSeconds += Dt;
			const EGenesisSpermPhase Phase = GetPhase(Cell);

			if (Phase == EGenesisSpermPhase::Fused)
			{
				continue;
			}
			if (Phase == EGenesisSpermPhase::Blocked)
			{
				++BlockedCount;
				// In der veränderten Zona steckengeblieben: Die Zelle kommt weder vor noch zurück
				if (Cell.PenetrationDepthUm > 0.0f)
				{
					continue;
				}
			}

			double Distance = 0.0;
			const FVector Inward = DirectionToOocyte(Cell, Oocyte, Distance);

			if (Phase == EGenesisSpermPhase::Swimming || Phase == EGenesisSpermPhase::Blocked)
			{
				if (Distance < Tuning.ChemotaxisRangeUm && Cell.bCapacitated)
				{
					// 1. Lockwirkung: Der Progesteron-Gradient aus dem Cumulus zieht kapazitierte Zellen an – am
					// stärksten hyperaktivierte, progressive träger. Nicht kapazitierte spüren ihn nicht (Docs/38).
					const double Response = Cell.Motility == EGenesisSpermMotility::Hyperactivated ? 1.0
						: (Cell.Motility == EGenesisSpermMotility::Sluggish ? 0.2 : 0.45);
					const double Strength = (1.0 - Distance / Tuning.ChemotaxisRangeUm) * Response;
					const double MaxTurn = Tuning.ChemotaxisTurnRate * Strength * Dt;
					const double Between = FMath::Acos(FMath::Clamp(FVector::DotProduct(Cell.Heading, Inward), -1.0, 1.0));
					if (Between > UE_KINDA_SMALL_NUMBER)
					{
						const FQuat Full = FQuat::FindBetweenNormals(Cell.Heading, Inward);
						Cell.Heading = FQuat::Slerp(FQuat::Identity, Full, FMath::Min(1.0, MaxTurn / Between)).RotateVector(Cell.Heading);
					}

					// Progesteron öffnet in kapazitierten Zellen den Calciumkanal CatSper: Hyperaktivierung
					if (Cell.Motility == EGenesisSpermMotility::Progressive
						&& Cell.Random.Bernoulli(Tuning.ProgesteroneHyperactivationPerSecond * (1.0 - Distance / Tuning.ChemotaxisRangeUm) * Dt))
					{
						GenesisSpermSwimLogic::ApplyMotility(Cell, EGenesisSpermMotility::Hyperactivated, SwimTuning);
					}
				}

				// 2. Cumulus: Die Gallerte bremst – die Zelle muss sich hindurcharbeiten
				const float OriginalSpeed = Cell.Speed;
				const bool bInCumulus = Distance < Oocyte.CumulusRadiusUm;
				if (bInCumulus)
				{
					Cell.Speed *= Tuning.CumulusSpeedFactor;
					// Die Akrosomreaktion kann schon hier beginnen (Maus: bei 12 von 13 erfolgreichen Zellen, Jin 2011)
					if (Cell.bCapacitated && !Cell.bAcrosomeReacted && Cell.AcrosomeTimer <= 0.0f
						&& Cell.Random.Bernoulli(Tuning.AcrosomeInCumulusPerSecond * Dt))
					{
						Cell.AcrosomeTimer = Pick(Tuning.AcrosomeReactionSeconds, Cell.Random.NextFloat());
					}
				}
				AdvanceAcrosome(Cell, Dt);
				// Der Cumulus ist eine Gallertmasse: Die Eileiterflüssigkeit strömt um ihn herum, nicht
				// durch ihn hindurch. Vorher wirkte der Strom auch darin – eine hyperaktivierte Zelle, von
				// der Gallerte gebremst, wurde von ihm auf der Stelle gehalten und erreichte die Zona nie.
				if (bInCumulus)
				{
					FGenesisOviductChannel StillWater = Channel;
					StillWater.WallFlowSpeedUm = 0.0f;
					GenesisSpermSwimLogic::Step(Cell, StillWater, SwimTuning, Dt);
				}
				else
				{
					GenesisSpermSwimLogic::Step(Cell, Channel, SwimTuning, Dt);
				}
				Cell.Speed = OriginalSpeed;

				// 2b. Die Zona ist ohne Bindung undurchdringlich: Eine Zelle, die auf sie trifft, bleibt auf
				// ihr liegen und gleitet an ihr entlang. Vorher schwammen ungebundene Zellen durch die Hülle
				// hindurch ins Innere der Eizelle – aufgefallen, als die eigene Zelle im Rennen nie band.
				{
					double NewDistance = 0.0;
					const FVector NewInward = DirectionToOocyte(Cell, Oocyte, NewDistance);
					if (NewDistance < Oocyte.ZonaOuterRadiusUm)
					{
						Cell.Position = Oocyte.Position - NewInward * Oocyte.ZonaOuterRadiusUm;
						const double IntoZona = FVector::DotProduct(Cell.Heading, NewInward);
						if (IntoZona > 0.0)
						{
							Cell.Heading = (Cell.Heading - NewInward * IntoZona).GetSafeNormal(UE_SMALL_NUMBER, Cell.Heading);
						}
					}
				}

				// 3. Bindung an die Zona – Abgewiesene binden nie wieder
				const float ZonaDistance = DistanceToZona(Cell, Oocyte);
				if (Phase == EGenesisSpermPhase::Swimming && ZonaDistance < Tuning.BindingDistanceUm && ZonaDistance > -ZonaThickness)
				{
					if (Oocyte.bZonaHardened)
					{
						// Die veränderte Zona (ZP2 gespalten) bindet nicht mehr: Die Zelle gleitet ab und schwimmt weiter
						SetPhase(Cell, EGenesisSpermPhase::Blocked);
						++BlockedCount;
					}
					else if (Cell.bCapacitated)
					{
						const float Chance = Cell.Motility == EGenesisSpermMotility::Hyperactivated
							? Tuning.BindingChancePerSecondHyper
							: (Cell.Motility == EGenesisSpermMotility::Progressive ? Tuning.BindingChancePerSecondProgressive : 0.0f);
						if (Cell.Random.Bernoulli(Chance * Dt))
						{
							// Kopf an die Zona setzen; die Zelle liegt flach an und wird schräg eindringen
							const FVector Tangent = TangentAt(Cell.Heading, Inward);
							Cell.Position = Oocyte.Position - Inward * Oocyte.ZonaOuterRadiusUm;
							Cell.PenetrationDepthUm = 0.0f;
							++BoundCount;
							if (Cell.bAcrosomeReacted)
							{
								// Die Akrosomreaktion liegt schon hinter ihr (im Cumulus): gleich hinein
								Cell.Heading = Tilted(Inward, Tangent, Tuning.EntryAngleDegrees);
								SetPhase(Cell, EGenesisSpermPhase::Penetrating);
							}
							else
							{
								Cell.Heading = Tilted(Inward, Tangent, Tuning.EntryAngleDegrees);
								SetPhase(Cell, EGenesisSpermPhase::Bound);
								if (Cell.AcrosomeTimer <= 0.0f)
								{
									Cell.AcrosomeTimer = Pick(Tuning.AcrosomeReactionSeconds, Cell.Random.NextFloat());
								}
							}
						}
					}
				}
				continue;
			}

			// Anhaftende Zellen schwimmen nicht mehr frei; die Geißel schlägt weiter
			Cell.BeatPhase = Cell.BeatPhase + Cell.BeatFrequencyHz * Dt;
			Cell.BeatPhase -= FMath::FloorToDouble(Cell.BeatPhase);

			if (Phase == EGenesisSpermPhase::Perivitelline)
			{
				++PerivitellineCount;
				// 6. Verschmelzung: Die Membranen finden sich nach Minuten. Ist schon eine andere Zelle
				// verschmolzen, lässt die Membran keine zweite zu – die Zelle bleibt im Spalt liegen.
				if (!Oocyte.IsFertilized())
				{
					Cell.FusionTimer -= Dt;
					if (Cell.FusionTimer <= 0.0f)
					{
						SetPhase(Cell, EGenesisSpermPhase::Fused);
						--PerivitellineCount;
						Oocyte.FertilizedByCell = Index;
						Oocyte.SecondsSinceFusion = 0.0f;
						Oocyte.CorticalReaction = 0.0f;
						bFusedThisStep = true;
						OutResult.CellIndex = Index;
						OutResult.Vitality = Cell.Vitality;
						OutResult.SecondsToFusion = Cell.LifeSeconds;
					}
				}
				continue;
			}

			if (Oocyte.bZonaHardened)
			{
				SetPhase(Cell, EGenesisSpermPhase::Blocked);
				++BlockedCount;
				if (Phase == EGenesisSpermPhase::Bound)
				{
					// Gebunden, aber noch nicht eingedrungen: Die Bindung löst sich, die Zelle treibt davon
					Cell.PenetrationDepthUm = 0.0f;
					Cell.Position = Oocyte.Position - Inward * (Oocyte.ZonaOuterRadiusUm + 3.0f);
					Cell.Heading = -Inward;
				}
				else
				{
					Cell.PenetrationDepthUm = FMath::Max(Cell.PenetrationDepthUm, 0.01f);
				}
				continue;
			}

			if (Phase == EGenesisSpermPhase::Bound)
			{
				++BoundCount;
				// 4. Akrosomreaktion an der Zona: erst danach kann die Zelle eindringen
				AdvanceAcrosome(Cell, Dt);
				if (Cell.bAcrosomeReacted)
				{
					SetPhase(Cell, EGenesisSpermPhase::Penetrating);
				}
				continue;
			}

			// 5. Durch die Zona: schräg, mit kräftigen Geißelschlägen – rund 13 Minuten
			++BoundCount;
			// Wie kräftig die Zelle schlägt: aus ihrer Veranlagung – oder, beim Spieler, aus seiner Anstrengung
			const float Drive = Cell.Vigor >= 0.0f ? Cell.Vigor : Cell.Individuality;
			const float RadialSpeed = Pick(Tuning.PenetrationSpeedUm, Drive) * (0.6f + 0.4f * Cell.Vitality);
			// Der Winkel bleibt zur jeweiligen Senkrechten gleich: Die Bahn krümmt sich mit der Zona
			Cell.Heading = Tilted(Inward, TangentAt(Cell.Heading, Inward), Tuning.EntryAngleDegrees);
			Cell.Position += Cell.Heading * (RadialSpeed * Dt / EntryCosine);
			const float NewDistance = static_cast<float>(FVector::Dist(Cell.Position, Oocyte.Position));
			Cell.PenetrationDepthUm = FMath::Max(0.0f, Oocyte.ZonaOuterRadiusUm - NewDistance);

			// Steckenbleiben: Wer schwach schlägt, bleibt eher hängen
			if (Cell.Random.Bernoulli(Tuning.PenetrationFailureRate * (1.0f - 0.7f * FMath::Clamp(Drive, 0.0f, 1.0f)) * Dt))
			{
				// Die Zelle löst sich wieder und schwimmt weiter; die Akrosomreaktion bleibt vollzogen
				SetPhase(Cell, EGenesisSpermPhase::Swimming);
				Cell.PenetrationDepthUm = 0.0f;
				Cell.Position = Oocyte.Position - Inward * (Oocyte.ZonaOuterRadiusUm + 3.0f);
				Cell.Heading = -Inward;
				continue;
			}

			// Durch: in den perivitellinen Spalt
			if (NewDistance <= Oocyte.ZonaInnerRadiusUm)
			{
				EnterPerivitelline(Cell, Oocyte, Tuning);
				--BoundCount;
				++PerivitellineCount;
			}
		}

		Oocyte.BoundCells = BoundCount;
		Oocyte.PerivitellineCells = PerivitellineCount;
		OutResult.CompetingCells = FMath::Max(OutResult.CompetingCells, BoundCount + PerivitellineCount);
		OutResult.BlockedCells = BlockedCount;
		return bFusedThisStep;
	}
}
