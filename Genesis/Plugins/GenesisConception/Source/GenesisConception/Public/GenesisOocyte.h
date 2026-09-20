// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisOocyte.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Die reife Eizelle im Eileiter: Zellleib, Zona pellucida, Polkörper, Corona radiata und die Gallerte des Cumulus.
 *
 * Der Actor hält den Zustand (Befruchtung, Cortikalreaktion) und gibt ihn an die Materialien weiter –
 * die Zona verändert sich sichtbar, sobald die Cortikalreaktion läuft.
 */
UCLASS()
class GENESISCONCEPTION_API AGenesisOocyte : public AActor
{
	GENERATED_BODY()

public:
	AGenesisOocyte();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FGenesisOocyteState& GetMutableState() { return State; }
	const FGenesisOocyteState& GetState() const { return State; }

	/** Mittelpunkt in Weltkoordinaten (µm = Unreal-Einheiten). */
	FVector GetCenterWorld() const { return GetActorLocation(); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oocyte")
	FGenesisOocyteState State;

	UPROPERTY(EditAnywhere, Category = "Oocyte")
	FGenesisFertilizationTuning Tuning;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Ooplasm;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Zona;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PolarBody;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Corona;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CumulusMatrix;

	/** Fäden der Gallerte zwischen den Zellen – erst sie machen aus dem Kranz eine Wolke. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CumulusStrands;

private:
	void RegisterDebugPage();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ZonaMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OoplasmMaterial;

	bool bDebugPageRegistered = false;
};
