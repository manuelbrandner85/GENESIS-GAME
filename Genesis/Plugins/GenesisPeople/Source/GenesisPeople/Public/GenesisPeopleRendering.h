// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"

class AActor;

namespace GenesisPeopleRendering
{
	/**
	 * Haare der MetaHumans im Maßstab der Szene rechnen. Die Haar-Voxel (Schatten und Licht in den Strähnen) haben in
	 * Unreal eine feste Weltgröße von 0,3 Einheiten = 3 mm bei 1 cm je Einheit. Im Kreißsaal ist 1 mm eine Einheit und
	 * die Figuren sind zehnfach skaliert – ohne Anpassung wären die Voxel 0,3 mm groß, tausendmal so viele: gemessen
	 * 6 ms nur für die Voxelisierung und Ausreißer bis 43 ms je Bild. Mit der Figur skaliert bleiben es 3 mm am
	 * Menschen – dasselbe Bild für 0,4 ms.
	 */
	GENESISPEOPLE_API float HairVoxelWorldSize(float CharacterScale);

	/** Setzt r.HairStrands.Voxelization.Virtual.VoxelWorldSize passend zur Skalierung der Figur. */
	GENESISPEOPLE_API void ScaleHairVoxelsTo(const AActor* Character);
}
