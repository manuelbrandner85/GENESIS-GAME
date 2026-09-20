// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisEmbryoTypes.h"
#include "GenesisEmbryoActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AGenesisOocyte;

/**
 * Der Keim im Eileiter: Die Zellen der Simulation werden als Instanzen dargestellt.
 *
 * Maßstab wie in der ganzen Mikrowelt: 1 µm = 1 Unreal-Einheit. Die Zellkugel des Grundmesh hat
 * 55 µm Radius, deshalb ist die Skalierung einer Zelle genau ihr Radius geteilt durch 55.
 */
UCLASS()
class GENESISEMBRYO_API AGenesisEmbryo : public AActor
{
	GENERATED_BODY()

public:
	AGenesisEmbryo();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Die Eizelle, aus der der Keim hervorgeht – ihr Zellleib und der Zellkranz verschwinden mit der Teilung. */
	UPROPERTY(EditAnywhere, Category = "Embryo")
	TObjectPtr<AGenesisOocyte> Oocyte;

	/** Kugel mit 55 µm Radius (SM_GEN_OocyteCytoplasm). */
	UPROPERTY(EditAnywhere, Category = "Rendering")
	TObjectPtr<UStaticMesh> CellMesh;

	UPROPERTY(EditAnywhere, Category = "Rendering")
	TObjectPtr<UMaterialInterface> CellMaterial;

	/** Stunden nach der Verschmelzung, über die sich der Zellkranz auflöst. */
	UPROPERTY(EditAnywhere, Category = "Embryo")
	float CoronaDispersalHours = 20.0f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Blastomeres;

private:
	void PushCells(const FGenesisEmbryoState& State);
	void UpdateOocyteRemains(const FGenesisEmbryoState& State);

	TArray<FTransform> TransformBuffer;
	TArray<float> CustomDataBuffer;
	int32 LastCellCount = -1;
};
