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

	FTransform ComputeAttachedTransform(const FGenesisSpermCell& Cell, const FGenesisOocyteState& Oocyte)
	{
		double Distance = 0.0;
		const FVector Inward = DirectionToOocyte(Cell, Oocyte, Distance);
		const FVector Reference = FMath::Abs(Inward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
		const FVector Side = FVector::CrossProduct(Reference, Inward).GetSafeNormal();
		const FMatrix Basis = FRotationMatrix::MakeFromXY(Inward, Side);
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

		// 7. Cortikalreaktion: Nach der Verschmelzung härtet die Zona – erst danach ist der Weg für alle anderen zu
		if (Oocyte.IsFertilized())
		{
			Oocyte.SecondsSinceFusion += Dt;
			Oocyte.CorticalReaction = FMath::Clamp(Oocyte.SecondsSinceFusion / FMath::Max(0.01f, Tuning.CorticalReactionSeconds), 0.0f, 1.0f);
			Oocyte.bZonaHardened = Oocyte.CorticalReaction >= Tuning.HardeningBlockThreshold;
		}

		const float ZonaThickness = FMath::Max(1.0f, Oocyte.ZonaOuterRadiusUm - Oocyte.ZonaInnerRadiusUm);
		bool bFusedThisStep = false;
		int32 BoundCount = 0;
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
				continue;
			}

			double Distance = 0.0;
			const FVector Inward = DirectionToOocyte(Cell, Oocyte, Distance);

			if (Phase == EGenesisSpermPhase::Swimming)
			{
				// 1. Lockwirkung: Der Progesteron-Gradient aus dem Cumulus zieht kapazitierte Zellen an.
				// Am stärksten reagieren hyperaktivierte – aber nicht nur sie: Eine progressive Zelle,
				// die in die Nähe kommt, dreht ebenfalls bei, nur träger.
				if (Distance < Tuning.ChemotaxisRangeUm)
				{
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

					// Progesteron aus dem Cumulus öffnet den Calciumkanal CatSper und löst die Hyperaktivierung
					// aus – aber nur bei kapazitierten Zellen (im Modell: Zellen, die überhaupt hyperaktivieren können)
					if (Cell.Motility == EGenesisSpermMotility::Progressive && SwimTuning.HyperactivationRate > 0.0f
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
				}
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

				// 3. Bindung an die Zona
				const float ZonaDistance = DistanceToZona(Cell, Oocyte);
				if (ZonaDistance < Tuning.BindingDistanceUm && ZonaDistance > -ZonaThickness)
				{
					if (Oocyte.bZonaHardened)
					{
						SetPhase(Cell, EGenesisSpermPhase::Blocked);
						++BlockedCount;
					}
					else
					{
						const float Chance = Cell.Motility == EGenesisSpermMotility::Hyperactivated
							? Tuning.BindingChancePerSecondHyper
							: (Cell.Motility == EGenesisSpermMotility::Progressive ? Tuning.BindingChancePerSecondProgressive : 0.0f);
						if (Cell.Random.Bernoulli(Chance * Dt))
						{
							SetPhase(Cell, EGenesisSpermPhase::Bound);
							Cell.AcrosomeTimer = Pick(Tuning.AcrosomeReactionSeconds, Cell.Random.NextFloat());
							Cell.PenetrationDepthUm = 0.0f;
							// Kopf an die Zona setzen und zur Eizelle ausrichten
							Cell.Position = Oocyte.Position - Inward * Oocyte.ZonaOuterRadiusUm;
							Cell.Heading = Inward;
							++BoundCount;
						}
					}
				}
				continue;
			}

			// Gebundene und bohrende Zellen schwimmen nicht mehr frei; die Geißel schlägt weiter (Darstellung)
			Cell.BeatPhase = Cell.BeatPhase + Cell.BeatFrequencyHz * Dt;
			Cell.BeatPhase -= FMath::FloorToDouble(Cell.BeatPhase);

			if (Oocyte.bZonaHardened && Oocyte.FertilizedByCell != Index)
			{
				SetPhase(Cell, EGenesisSpermPhase::Blocked);
				++BlockedCount;
				continue;
			}

			if (Phase == EGenesisSpermPhase::Bound)
			{
				++BoundCount;
				// 4. Akrosomreaktion: erst danach kann die Zelle bohren
				Cell.AcrosomeTimer -= Dt;
				if (Cell.AcrosomeTimer <= 0.0f)
				{
					SetPhase(Cell, EGenesisSpermPhase::Penetrating);
				}
				continue;
			}

			// 5. Durchdringung der Zona
			++BoundCount;
			// Wie kräftig die Zelle bohrt: aus ihrer Veranlagung – oder, beim Spieler, aus seiner Anstrengung
			const float Drive = Cell.Vigor >= 0.0f ? Cell.Vigor : Cell.Individuality;
			const float Speed = Pick(Tuning.PenetrationSpeedUm, Drive) * (0.6f + 0.4f * Cell.Vitality);
			Cell.PenetrationDepthUm += Speed * Dt;
			Cell.Position = Oocyte.Position - Inward * FMath::Max(Oocyte.ZonaInnerRadiusUm, Oocyte.ZonaOuterRadiusUm - Cell.PenetrationDepthUm);

			// Steckenbleiben: Wer schwach schlägt, bleibt eher hängen – ein kräftiger hyperaktivierter
			// Schlag ist genau das, was eine Zelle durch die Zona bringt
			if (Cell.Random.Bernoulli(Tuning.PenetrationFailureRate * (1.0f - 0.7f * FMath::Clamp(Drive, 0.0f, 1.0f)) * Dt))
			{
				// Steckengeblieben: Die Zelle löst sich wieder und schwimmt weiter
				SetPhase(Cell, EGenesisSpermPhase::Swimming);
				Cell.PenetrationDepthUm = 0.0f;
				Cell.Position = Oocyte.Position - Inward * (Oocyte.ZonaOuterRadiusUm + 3.0f);
				Cell.Heading = -Inward;
				continue;
			}

			// 6. Verschmelzung – nur für die erste Zelle
			if (Cell.PenetrationDepthUm >= ZonaThickness)
			{
				if (!Oocyte.IsFertilized())
				{
					SetPhase(Cell, EGenesisSpermPhase::Fused);
					Oocyte.FertilizedByCell = Index;
					Oocyte.SecondsSinceFusion = 0.0f;
					Oocyte.CorticalReaction = 0.0f;
					bFusedThisStep = true;
					OutResult.CellIndex = Index;
					OutResult.Vitality = Cell.Vitality;
					OutResult.SecondsToFusion = Cell.LifeSeconds;
				}
				else
				{
					SetPhase(Cell, EGenesisSpermPhase::Blocked);
					++BlockedCount;
				}
			}
		}

		Oocyte.BoundCells = BoundCount;
		OutResult.CompetingCells = FMath::Max(OutResult.CompetingCells, BoundCount);
		OutResult.BlockedCells = BlockedCount;
		return bFusedThisStep;
	}
}
