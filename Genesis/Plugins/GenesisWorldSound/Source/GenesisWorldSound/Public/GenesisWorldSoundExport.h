// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisWorldSoundTypes.h"

/** Hörproben der Orte – die einzige Art, einen Raum wirklich zu prüfen. */
namespace GenesisWorldSoundExport
{
	GENESISWORLDSOUND_API EGenesisPlace ParsePlace(const FString& Name);
	GENESISWORLDSOUND_API FString GetPlaceName(EGenesisPlace Place);

	/** Die Werte, mit denen ein Ort in der Hörprobe klingt. */
	GENESISWORLDSOUND_API FGenesisWorldSoundParams GetPresetParams(EGenesisPlace Place);

	/** Rendert die Probe (Mono). Ohne Welt, ohne Audiogerät. */
	GENESISWORLDSOUND_API void RenderPlace(EGenesisPlace Place, float SampleRate, float Seconds, TArray<float>& OutSamples);
}
