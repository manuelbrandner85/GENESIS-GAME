// GENESIS: Der Kreislauf des Lebens

#include "GenesisSpermRace.h"
#include "GenesisFertilizationLogic.h"

float GenesisSpermRace::UpdateVigor(float Vigor, int32 Presses, float RealDeltaSeconds, const FGenesisRaceTuning& Tuning)
{
	const float Decayed = FMath::Max(0.0f, Vigor) - Tuning.VigorDecayPerSecond * FMath::Max(0.0f, RealDeltaSeconds);
	return FMath::Clamp(Decayed + Tuning.VigorPerPress * FMath::Max(0, Presses), 0.0f, 1.0f);
}

FVector GenesisSpermRace::SteerFromInput(const FVector& Heading, const FVector2D& Input, const FGenesisRaceTuning& Tuning)
{
	const FVector2D Clamped(FMath::Clamp(Input.X, -1.0, 1.0), FMath::Clamp(Input.Y, -1.0, 1.0));
	if (Clamped.IsNearlyZero(0.05))
	{
		return FVector::ZeroVector;
	}
	const FVector Forward = Heading.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// „Oben" ist die Hochachse des Kanals, solange die Zelle nicht senkrecht schwimmt
	const FVector Reference = FMath::Abs(Forward.Z) < 0.95 ? FVector::UpVector : FVector::RightVector;
	const FVector Right = FVector::CrossProduct(Reference, Forward).GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	FVector Desired = FQuat(Up, FMath::DegreesToRadians(Tuning.SteerYawDegrees * Clamped.X)).RotateVector(Forward);
	Desired = FQuat(Right, FMath::DegreesToRadians(-Tuning.SteerPitchDegrees * Clamped.Y)).RotateVector(Desired);
	return Desired.GetSafeNormal();
}

int32 GenesisSpermRace::CountCellsAhead(const TArray<FGenesisSpermCell>& Cells, int32 PlayerIndex, const FGenesisOocyteState& Oocyte)
{
	if (!Cells.IsValidIndex(PlayerIndex))
	{
		return 0;
	}
	auto Remaining = [&Oocyte](const FGenesisSpermCell& Cell)
	{
		switch (GenesisFertilizationLogic::GetPhase(Cell))
		{
		case EGenesisSpermPhase::Fused:
			return -1000.0f;
		case EGenesisSpermPhase::Bound:
		case EGenesisSpermPhase::Penetrating:
			// Wer in der Zona steckt, ist vorn – je tiefer, desto weiter
			return -Cell.PenetrationDepthUm;
		case EGenesisSpermPhase::Blocked:
			return TNumericLimits<float>::Max();
		default:
			return FMath::Max(0.0f, GenesisFertilizationLogic::DistanceToZona(Cell, Oocyte));
		}
	};
	const float Mine = Remaining(Cells[PlayerIndex]);
	int32 Ahead = 0;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		if (Index != PlayerIndex && Remaining(Cells[Index]) < Mine)
		{
			++Ahead;
		}
	}
	return Ahead;
}

EGenesisRaceOutcome GenesisSpermRace::OutcomeAfterFusion(int32 PlayerIndex, int32 FusedIndex)
{
	if (PlayerIndex == INDEX_NONE)
	{
		return EGenesisRaceOutcome::None;
	}
	if (FusedIndex == INDEX_NONE)
	{
		return EGenesisRaceOutcome::Running;
	}
	return FusedIndex == PlayerIndex ? EGenesisRaceOutcome::Won : EGenesisRaceOutcome::Lost;
}
