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

	/**
	 * Zelltrümmer (Fragmente) zwischen den Zellen und der Hülle. Unsaubere Teilungen schnüren kernlose
	 * Zytoplasma-Stücke ab, 2–10 µm groß; Embryologen schätzen daran die Qualität eines Keims.
	 * Vorher nur eine Zahl in der Simulation, jetzt sichtbar (GENESIS-038).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Fragments;

	/** Größte Zahl sichtbarer Fragmente (bei Fragmentierung 1,0). */
	UPROPERTY(EditAnywhere, Category = "Embryo")
	int32 MaxFragments = 60;

	/**
	 * Nach so vielen Stunden sind die übrigen Spermien fort – abgestorben oder weitergetrieben.
	 * Vorher schwammen sie die ganze Woche um den Keim, als wäre nichts geschehen.
	 */
	UPROPERTY(EditAnywhere, Category = "Embryo")
	float SwarmGoneHours = 30.0f;

private:
	void PushCells(const FGenesisEmbryoState& State);
	void PushFragments(const FGenesisEmbryoState& State);
	void UpdateOocyteRemains(const FGenesisEmbryoState& State);
	int32 LastFragmentCount = -1;

	TArray<FTransform> TransformBuffer;
	TArray<float> CustomDataBuffer;
	int32 LastCellCount = -1;
};
