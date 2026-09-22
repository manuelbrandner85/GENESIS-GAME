// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"

class AActor;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Wie die Haut nach körperlicher Anstrengung aussieht (Geburt): Schweiß macht sie glatter und glänzender, die Durchblutung
 * rötet sie. Werte für die MetaHuman-Hautmaterialien („… Global Multiply Post-Bake").
 */
struct GENESISPEOPLE_API FGenesisSkinExertion
{
	float RoughnessMultiply = 1.0f;
	float SpecularMultiply = 1.0f;
	FLinearColor BaseColorMultiply = FLinearColor::White;
};

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

namespace GenesisPeopleRendering
{
	/**
	 * Haut bei Anstrengung 0..1. Bei 1 (gerade geboren, nach den Presswehen) glänzt sie vor Schweiß (Rauheit ×0,48, Glanz
	 * ×1,4) und ist gerötet (Rot ×1,08, Grün/Blau ×0,90); bei 0 ist sie wie im Ruhezustand gebaut.
	 */
	GENESISPEOPLE_API FGenesisSkinExertion SkinExertion(float Exertion);

	/** Anstrengung nach der Geburt: Schweiß verdunstet, die Röte klingt ab – Halbwertszeit gut 14 min. */
	GENESISPEOPLE_API float ExertionAfterBirth(float MinutesSinceBirth);

	/** Setzt die Werte auf alle Hautmaterialien des Gesichts (legt dynamische Instanzen einmalig an). */
	GENESISPEOPLE_API void ApplySkinExertion(USkeletalMeshComponent* Face, const FGenesisSkinExertion& Look);
}
