// GENESIS: Der Kreislauf des Lebens

#include "GenesisHearingFilter.h"

void FGenesisHearingFilter::Process(float* Audio, int32 Frames, float SampleRate)
{
	if (!Audio || Frames <= 0)
	{
		return;
	}

	const float Rate = FMath::Max(8000.0f, SampleRate);
	const float Cutoff = FMath::Clamp(CutoffHz, 40.0f, Rate * 0.45f);
	const float Alpha = 1.0f - FMath::Exp(-2.0f * PI * Cutoff / Rate);

	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		Stage1 += (Audio[Frame] - Stage1) * Alpha;
		Stage2 += (Stage1 - Stage2) * Alpha;
		Stage3 += (Stage2 - Stage3) * Alpha;
		Stage4 += (Stage3 - Stage4) * Alpha;
		Audio[Frame] = Stage4 * Gain;
	}
}
