// GENESIS: Der Kreislauf des Lebens

#include "GenesisSpermSwimLogic.h"

namespace GenesisSpermSwimLogic
{
	namespace
	{
		constexpr uint64 CellStreamSalt = 0x5BE2A11ull;

		float Pick(const FFloatInterval& Range, float Alpha)
		{
			return FMath::Lerp(Range.Min, Range.Max, FMath::Clamp(Alpha, 0.0f, 1.0f));
		}

		FVector RadialDirection(const FVector& Position)
		{
			const FVector Radial(0.0, Position.Y, Position.Z);
			const double Length = Radial.Size();
			return Length > UE_KINDA_SMALL_NUMBER ? Radial / Length : FVector::RightVector;
		}

		double RadialDistance(const FVector& Position)
		{
			return FVector2D(Position.Y, Position.Z).Size();
		}

		FVector SafeNormal(const FVector& Vector, const FVector& Fallback)
		{
			const FVector Normal = Vector.GetSafeNormal();
			return Normal.IsNearlyZero() ? Fallback : Normal;
		}

		/** Schlagebene: senkrecht zur Schwimmrichtung, rollt mit der Zelle um die Längsachse. */
		FVector BeatSide(const FGenesisSpermCell& Cell)
		{
			const FVector Forward = Cell.Heading;
			const FVector Reference = FMath::Abs(Forward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
			const FVector Side = SafeNormal(FVector::CrossProduct(Reference, Forward), FVector::RightVector);
			return FQuat(Forward, 2.0 * UE_DOUBLE_PI * Cell.RollPhase).RotateVector(Side);
		}

		double Wrap01(double Value)
		{
			return Value - FMath::FloorToDouble(Value);
		}
	}

	void ApplyMotility(FGenesisSpermCell& Cell, EGenesisSpermMotility Motility, const FGenesisSpermSwimTuning& Tuning)
	{
		Cell.Motility = Motility;
		const float Alpha = Cell.Individuality;
		const float VitalityScale = 0.6f + 0.4f * FMath::Clamp(Cell.Vitality, 0.0f, 1.0f);

		switch (Motility)
		{
		case EGenesisSpermMotility::Hyperactivated:
			Cell.Speed = Pick(Tuning.HyperSpeedUm, Alpha) * VitalityScale;
			Cell.BeatFrequencyHz = Pick(Tuning.HyperBeatHz, 1.0f - Alpha);
			Cell.HeadAmplitudeUm = Pick(Tuning.HyperHeadAmplitudeUm, Alpha);
			Cell.WavelengthUm = Tuning.HyperWavelengthUm;
			Cell.Asymmetry = 0.6f + 0.3f * Alpha;
			break;
		case EGenesisSpermMotility::Sluggish:
			Cell.Speed = Pick(Tuning.SluggishSpeedUm, Alpha);
			Cell.BeatFrequencyHz = Pick(Tuning.SluggishBeatHz, Alpha);
			Cell.HeadAmplitudeUm = Pick(Tuning.ProgressiveHeadAmplitudeUm, 0.2f * Alpha);
			Cell.WavelengthUm = Tuning.ProgressiveWavelengthUm;
			Cell.Asymmetry = 0.1f;
			break;
		default:
			Cell.Speed = Pick(Tuning.ProgressiveSpeedUm, Alpha) * VitalityScale;
			Cell.BeatFrequencyHz = Pick(Tuning.ProgressiveBeatHz, Alpha);
			Cell.HeadAmplitudeUm = Pick(Tuning.ProgressiveHeadAmplitudeUm, 1.0f - Alpha);
			Cell.WavelengthUm = Tuning.ProgressiveWavelengthUm;
			Cell.Asymmetry = 0.05f;
			break;
		}
	}

	FGenesisSpermCell CreateCell(uint64 Seed, float Vitality, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning)
	{
		FGenesisRandomStream Rng(Seed);
		FGenesisSpermCell Cell;
		Cell.Individuality = Rng.NextFloat();
		Cell.Vitality = FMath::Clamp(Vitality, 0.0f, 1.0f);

		const double Radius = (Channel.LumenRadiusUm - Tuning.WallMarginUm) * FMath::Sqrt(Rng.NextDouble());
		const double Angle = 2.0 * UE_DOUBLE_PI * Rng.NextDouble();
		Cell.Position = FVector(Rng.NextDouble() * Channel.LengthUm, Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));

		// Gleichverteilte Richtung auf der Kugel
		const double CosTheta = 2.0 * Rng.NextDouble() - 1.0;
		const double SinTheta = FMath::Sqrt(FMath::Max(0.0, 1.0 - CosTheta * CosTheta));
		const double Phi = 2.0 * UE_DOUBLE_PI * Rng.NextDouble();
		Cell.Heading = FVector(CosTheta, SinTheta * FMath::Cos(Phi), SinTheta * FMath::Sin(Phi));

		Cell.BeatPhase = Rng.NextDouble();
		Cell.RollPhase = Rng.NextDouble();
		Cell.Random = Rng.Derive(CellStreamSalt);

		EGenesisSpermMotility Motility = EGenesisSpermMotility::Progressive;
		if (Cell.Vitality < Tuning.SluggishVitalityThreshold)
		{
			Motility = EGenesisSpermMotility::Sluggish;
		}
		else if (Tuning.DeactivationRate + Tuning.HyperactivationRate > 0.0f)
		{
			// Startverteilung im Gleichgewicht der Wechselraten
			const float Equilibrium = Tuning.HyperactivationRate / (Tuning.HyperactivationRate + Tuning.DeactivationRate);
			Motility = Rng.Bernoulli(Equilibrium) ? EGenesisSpermMotility::Hyperactivated : EGenesisSpermMotility::Progressive;
		}
		// Nur kapazitierte Zellen hyperaktivieren (Docs/38). Als letzter Zug aus dem Strom gezogen, damit alle
		// anderen Eigenschaften einer Zelle dieselben bleiben wie vor der Einführung des Zustands.
		Cell.bCapacitated = Cell.Vitality >= Tuning.SluggishVitalityThreshold && Rng.Bernoulli(Tuning.CapacitatedFraction);
		if (!Cell.bCapacitated && Motility == EGenesisSpermMotility::Hyperactivated)
		{
			Motility = EGenesisSpermMotility::Progressive;
		}
		ApplyMotility(Cell, Motility, Tuning);
		return Cell;
	}

	FVector FlowAt(const FVector& Position, const FGenesisOviductChannel& Channel)
	{
		// Im Eileiter strömt es überall Richtung Gebärmutter: Die Zilien der Schleimhaut treiben den
		// Film an der Wand am stärksten, aber auch in der Mitte des Lumens steht die Flüssigkeit nicht.
		// Das ist der Unterschied zwischen einem Kanal und einem Becken – und für die Zellen der
		// einzige Hinweis, wo oben ist: Sie schwimmen gegen den Strom.
		const float Radius = static_cast<float>(RadialDistance(Position));
		const float WallShare = FMath::SmoothStep(0.35f * Channel.LumenRadiusUm, Channel.LumenRadiusUm, Radius);
		const float Factor = FMath::Clamp(Channel.CoreFlowFraction, 0.0f, 1.0f)
			+ (1.0f - FMath::Clamp(Channel.CoreFlowFraction, 0.0f, 1.0f)) * WallShare;
		return FVector(-Channel.WallFlowSpeedUm * Factor, 0.0, 0.0);
	}

	float WallProximity(const FVector& Position, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning)
	{
		const float Distance = Channel.LumenRadiusUm - static_cast<float>(RadialDistance(Position));
		return 1.0f - FMath::SmoothStep(0.0f, FMath::Max(1.0f, Tuning.WallAttractionDistanceUm), Distance);
	}

	void Step(FGenesisSpermCell& Cell, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning, float DeltaSeconds)
	{
		const float Dt = FMath::Max(0.0f, DeltaSeconds);
		if (Dt <= 0.0f)
		{
			return;
		}
		FGenesisRandomStream& Rng = Cell.Random;

		// 1. Wechsel der Bewegungsart – hyperaktivieren kann nur eine kapazitierte Zelle
		if (Cell.Motility == EGenesisSpermMotility::Progressive && Cell.bCapacitated && Rng.Bernoulli(Tuning.HyperactivationRate * Dt))
		{
			ApplyMotility(Cell, EGenesisSpermMotility::Hyperactivated, Tuning);
		}
		else if (Cell.Motility == EGenesisSpermMotility::Hyperactivated && Rng.Bernoulli(Tuning.DeactivationRate * Dt))
		{
			ApplyMotility(Cell, EGenesisSpermMotility::Progressive, Tuning);
		}
		const bool bHyper = Cell.Motility == EGenesisSpermMotility::Hyperactivated;

		// 2. Rotationsdiffusion
		const float RotationalDiffusion = bHyper ? Tuning.HyperRotationalDiffusion
			: (Cell.Motility == EGenesisSpermMotility::Sluggish ? 2.0f * Tuning.ProgressiveRotationalDiffusion : Tuning.ProgressiveRotationalDiffusion);
		const float Angle = Rng.Gaussian(0.0f, FMath::Sqrt(2.0f * RotationalDiffusion * Dt));
		if (Angle != 0.0f)
		{
			const FVector RandomVector(Rng.FRandRange(-1.0f, 1.0f), Rng.FRandRange(-1.0f, 1.0f), Rng.FRandRange(-1.0f, 1.0f));
			const FVector Axis = SafeNormal(FVector::CrossProduct(Cell.Heading, RandomVector), FVector::UpVector);
			Cell.Heading = FQuat(Axis, Angle).RotateVector(Cell.Heading);
		}

		// 3. Rheotaxis: an der Wand gegen den Strom drehen
		const FVector Flow = FlowAt(Cell.Position, Channel);
		const double FlowSpeed = Flow.Size();
		if (FlowSpeed > UE_KINDA_SMALL_NUMBER && Channel.WallFlowSpeedUm > 0.0f)
		{
			const FVector Upstream = -Flow / FlowSpeed;
			const double Shear = FMath::Clamp(FlowSpeed / Channel.WallFlowSpeedUm, 0.0, 1.0);
			const double MaxTurn = Tuning.RheotaxisTurnRate * Shear * Dt;
			const double Between = FMath::Acos(FMath::Clamp(FVector::DotProduct(Cell.Heading, Upstream), -1.0, 1.0));
			if (Between > UE_KINDA_SMALL_NUMBER && MaxTurn > 0.0)
			{
				const FQuat Full = FQuat::FindBetweenNormals(Cell.Heading, Upstream);
				Cell.Heading = FQuat::Slerp(FQuat::Identity, Full, FMath::Min(1.0, MaxTurn / Between)).RotateVector(Cell.Heading);
			}
		}

		// 3b. Lenken (Spieler): Drehung auf die gewünschte Richtung zu, höchstens mit der Drehrate, die ein
		// asymmetrischer Geißelschlag hergibt. Zufall, Strömung und Wand wirken weiter.
		if (!Cell.SteerDirection.IsNearlyZero())
		{
			const FVector Desired = SafeNormal(Cell.SteerDirection, Cell.Heading);
			const double Rate = Tuning.SteerTurnRate * (bHyper ? Tuning.SteerTurnRateHyperFactor : 1.0f)
				* (Cell.Motility == EGenesisSpermMotility::Sluggish ? 0.5f : 1.0f);
			const double Between = FMath::Acos(FMath::Clamp(FVector::DotProduct(Cell.Heading, Desired), -1.0, 1.0));
			if (Between > UE_KINDA_SMALL_NUMBER)
			{
				const FQuat Full = FQuat::FindBetweenNormals(Cell.Heading, Desired);
				Cell.Heading = FQuat::Slerp(FQuat::Identity, Full, FMath::Min(1.0, Rate * Dt / Between)).RotateVector(Cell.Heading);
			}
		}

		// 4. Wandbindung: zur Wand hin ausrichten lassen, Ablösen erschweren (hyperaktivierte lösen sich leichter)
		const float Proximity = WallProximity(Cell.Position, Channel, Tuning);
		const FVector Radial = RadialDirection(Cell.Position);
		if (Proximity > 0.0f)
		{
			const float HyperFactor = bHyper ? 0.35f : 1.0f;
			const float Trap = Tuning.WallTrapStrength * Proximity * HyperFactor;
			// Zielneigung leicht zur Wand: Zellen, die auf die Wand treffen, richten sich schnell parallel aus, abwandernde werden zurückgelenkt
			const double TargetRadial = FMath::Sin(FMath::DegreesToRadians(Tuning.WallTiltDegrees)) * HyperFactor;
			const double RadialComponent = FVector::DotProduct(Cell.Heading, Radial);
			const double Alignment = RadialComponent > TargetRadial ? FMath::Clamp(Trap * 8.0f * Dt, 0.0f, 1.0f) : FMath::Clamp(Trap * 4.0f * Dt, 0.0f, 1.0f);
			Cell.Heading += Radial * (TargetRadial - RadialComponent) * Alignment;
		}
		Cell.Heading = SafeNormal(Cell.Heading, FVector::ForwardVector);

		// 5. Bewegung
		Cell.Position += (Cell.Heading * Cell.Speed + Flow) * Dt;

		// 6. Die Wand ist undurchdringlich
		const double Limit = Channel.LumenRadiusUm - Tuning.WallMarginUm;
		const double Radius = RadialDistance(Cell.Position);
		if (Radius > Limit)
		{
			const FVector Outward = RadialDirection(Cell.Position);
			Cell.Position.Y *= Limit / Radius;
			Cell.Position.Z *= Limit / Radius;
			const double Outgoing = FVector::DotProduct(Cell.Heading, Outward);
			if (Outgoing > 0.0)
			{
				Cell.Heading = SafeNormal(Cell.Heading - Outward * Outgoing, FVector::ForwardVector);
			}
		}

		// 7. Endloser Abschnitt
		Cell.Position.X = FMath::Fmod(Cell.Position.X, static_cast<double>(Channel.LengthUm));
		if (Cell.Position.X < 0.0)
		{
			Cell.Position.X += Channel.LengthUm;
		}

		// 8. Schlag und Rollen
		Cell.BeatPhase = Wrap01(Cell.BeatPhase + Cell.BeatFrequencyHz * Dt);
		Cell.RollPhase = Wrap01(Cell.RollPhase + Cell.BeatFrequencyHz * Tuning.RollPerBeat * Dt);
	}

	int32 Advance(FGenesisSpermCell& Cell, const FGenesisOviductChannel& Channel, const FGenesisSpermSwimTuning& Tuning, float DeltaSeconds)
	{
		const float StepSeconds = FMath::Max(0.001f, Tuning.FixedStepSeconds);
		int32 Steps = 0;
		float Remaining = DeltaSeconds;
		while (Remaining > UE_KINDA_SMALL_NUMBER)
		{
			const float Slice = FMath::Min(StepSeconds, Remaining);
			Step(Cell, Channel, Tuning, Slice);
			Remaining -= Slice;
			++Steps;
		}
		return Steps;
	}

	void ComputeBeatFrame(const FGenesisSpermCell& Cell, FVector& OutSide, FVector& OutNormal)
	{
		OutSide = BeatSide(Cell);
		OutNormal = SafeNormal(FVector::CrossProduct(Cell.Heading, OutSide), FVector::UpVector);
	}

	FVector ComputeHeadPosition(const FGenesisSpermCell& Cell)
	{
		return Cell.Position + BeatSide(Cell) * (0.5f * Cell.HeadAmplitudeUm * FMath::Sin(2.0 * UE_DOUBLE_PI * Cell.BeatPhase));
	}

	FTransform ComputeVisualTransform(const FGenesisSpermCell& Cell)
	{
		const FVector Side = BeatSide(Cell);
		const FVector Normal = SafeNormal(FVector::CrossProduct(Cell.Heading, Side), FVector::UpVector);

		// Der Kopf dreht sich nur als Gegenbewegung zur Geißel – er pendelt nicht selbst. Gemessen: progressiv
		// ±3–6°, hyperaktiviert ±25–40° (Docs/26). Früher drehte sich hier die ganze Zelle um bis zu 55° im
		// Schlagtakt; die Geißel schwang dadurch als starrer Stab mit, und genau das sah nach Wackeln aus.
		const double YawAmplitude = HeadYawAmplitude(Cell);
		const double Yaw = YawAmplitude * FMath::Cos(2.0 * UE_DOUBLE_PI * Cell.BeatPhase) + 0.1 * Cell.Asymmetry;
		const FQuat YawRotation(Normal, Yaw);
		const FVector Forward = YawRotation.RotateVector(Cell.Heading);
		const FVector YawedSide = YawRotation.RotateVector(Side);

		const FMatrix Basis = FRotationMatrix::MakeFromXY(Forward, YawedSide);
		return FTransform(Basis.ToQuat(), ComputeHeadPosition(Cell) * GenesisMicroScale::UnitsPerMicrometer);
	}

	double HeadYawAmplitude(const FGenesisSpermCell& Cell)
	{
		return Cell.HeadAmplitudeUm * (0.019 + 0.03 * FMath::Clamp(Cell.Asymmetry, 0.0f, 1.0f));
	}

	float FlagellumTipAngle(const FGenesisSpermCell& Cell)
	{
		// Auslenkungswinkel der Geißel an der Spitze (rad): progressiv ~0,8, hyperaktiviert bis 1,3
		const float Asym = FMath::Clamp(Cell.Asymmetry, 0.0f, 1.0f);
		return FMath::Clamp(0.55f + 0.08f * Cell.HeadAmplitudeUm * (0.5f + Asym), 0.6f, 1.35f);
	}

	void ComputeMaterialData(const FGenesisSpermCell& Cell, float OutData[4])
	{
		OutData[0] = static_cast<float>(Cell.BeatPhase);
		OutData[1] = FlagellumTipAngle(Cell);
		OutData[2] = Cell.Asymmetry;
		OutData[3] = Cell.WavelengthUm;
	}
}
