// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "NativeGameplayTags.h"

/**
 * Systemisch genutzte Merkmale und epigenetische Pfade.
 * Weitere Merkmale definiert der Gen-Katalog (Data Asset) unter Genesis.Trait.*.
 */
namespace GenesisGeneticsTags
{
	// Körper
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Body_Height);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Body_MuscleMass);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Body_Metabolism);

	// Aussehen
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Appearance_Pigmentation);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Appearance_LightEyes);

	// Risiken
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Risk_Cardiovascular);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Risk_AnxietySensitivity);

	// Talente & Sinne
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Talent_Musical);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Talent_Spatial);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Sense_SmellAcuity);

	// Epigenetische Pfade
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Epigenetic_StressResponse);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Epigenetic_Metabolism);
	GENESISGENETICS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Epigenetic_PhysicalConditioning);
}
