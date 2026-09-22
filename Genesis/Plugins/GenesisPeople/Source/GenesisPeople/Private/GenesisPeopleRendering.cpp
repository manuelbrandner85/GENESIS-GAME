// GENESIS: Der Kreislauf des Lebens

#include "GenesisPeopleRendering.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "GenesisLog.h"

float GenesisPeopleRendering::HairVoxelWorldSize(float CharacterScale)
{
	// Voreinstellung der Engine: 0,3 Einheiten bei Maßstab 1 (3 mm)
	return 0.3f * FMath::Max(0.01f, CharacterScale);
}

void GenesisPeopleRendering::ScaleHairVoxelsTo(const AActor* Character)
{
	if (!Character)
	{
		return;
	}
	const float Scale = Character->GetActorScale3D().GetAbsMax();
	if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HairStrands.Voxelization.Virtual.VoxelWorldSize")))
	{
		const float Size = HairVoxelWorldSize(Scale);
		if (!FMath::IsNearlyEqual(Variable->GetFloat(), Size))
		{
			Variable->Set(Size, ECVF_SetByCode);
			UE_LOG(LogGenesis, Display, TEXT("Haar-Voxel: %.2f Einheiten (Figur %.1f-fach skaliert, am Menschen 3 mm)"), Size, Scale);
		}
	}
}
