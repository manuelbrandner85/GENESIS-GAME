// GENESIS: Der Kreislauf des Lebens

#include "GenesisPeopleRendering.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "GenesisLog.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

FGenesisSkinExertion GenesisPeopleRendering::SkinExertion(float Exertion)
{
	const float E = FMath::Clamp(Exertion, 0.0f, 1.0f);
	FGenesisSkinExertion Look;
	Look.RoughnessMultiply = FMath::Lerp(1.0f, 0.48f, E);
	Look.SpecularMultiply = FMath::Lerp(1.0f, 1.4f, E);
	Look.BaseColorMultiply = FLinearColor(FMath::Lerp(1.0f, 1.08f, E), FMath::Lerp(1.0f, 0.90f, E), FMath::Lerp(1.0f, 0.90f, E), 1.0f);
	return Look;
}

float GenesisPeopleRendering::ExertionAfterBirth(float MinutesSinceBirth)
{
	return FMath::Exp(-FMath::Max(0.0f, MinutesSinceBirth) / 20.0f);
}

void GenesisPeopleRendering::ApplySkinExertion(USkeletalMeshComponent* Face, const FGenesisSkinExertion& Look)
{
	if (!Face)
	{
		return;
	}
	for (int32 Slot = 0; Slot < Face->GetNumMaterials(); ++Slot)
	{
		UMaterialInterface* Material = Face->GetMaterial(Slot);
		// Gesicht (…Face_Skin…) und Körper (…Body_Baked…): Schweiß und Röte gibt es auch an Hals und Brust
		if (!Material || !(Material->GetName().Contains(TEXT("Skin")) || Material->GetName().Contains(TEXT("Body"))))
		{
			continue;
		}
		UMaterialInstanceDynamic* Instance = Cast<UMaterialInstanceDynamic>(Material);
		if (!Instance)
		{
			Instance = Face->CreateAndSetMaterialInstanceDynamic(Slot);
		}
		if (Instance)
		{
			Instance->SetScalarParameterValue(TEXT("Roughness Global Multiply Post-Bake"), Look.RoughnessMultiply);
			Instance->SetScalarParameterValue(TEXT("Specular Global Multiply Post-Bake"), Look.SpecularMultiply);
			Instance->SetVectorParameterValue(TEXT("Basecolor Global Multiply Post-Bake"), Look.BaseColorMultiply);
		}
	}
}
