// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisFertilizationTypes.h"
#include "GenesisOocyte.generated.h"

class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
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

	virtual void OnConstruction(const FTransform& Transform) override;
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

	/**
	 * Zweiter Polkörper: Erst die Verschmelzung weckt die Eizelle aus der Metaphase II; sie schnürt die
	 * überzähligen Chromosomen ab, rund 3,5 Stunden danach (Docs/38). Bis dahin unsichtbar, der Keim zeigt ihn.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SecondPolarBody;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Corona;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CumulusMatrix;

	/** Fäden der Gallerte zwischen den Zellen – erst sie machen aus dem Kranz eine Wolke. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CumulusStrands;

	/**
	 * Der äußere Cumulus (GENESIS-047 Teil 2): gut 13.000 Zellen frei in der Gallerte, als Instanzen. Mehrere
	 * Formvarianten, damit keine zwei Nachbarn gleich aussehen. Dieselben Zellen sind Hindernisse für die Spermien.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> CumulusCells;

	UPROPERTY(EditAnywhere, Category = "Cumulus")
	FGenesisCumulusTuning CumulusTuning;

	/** Formvarianten einer Cumuluszelle (Einheitsgröße: Radius 1 µm; die Instanz trägt die Halbachsen). */
	UPROPERTY(EditAnywhere, Category = "Cumulus")
	TArray<TObjectPtr<UStaticMesh>> CumulusCellMeshes;

	UPROPERTY(EditAnywhere, Category = "Cumulus")
	TObjectPtr<UMaterialInterface> CumulusCellMaterial;

	/** Baut Zellfeld und Instanzen neu (deterministisch aus CumulusTuning.Seed). */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Cumulus")
	void RebuildCumulus();

	static constexpr int32 CumulusVariantCount = 4;

private:
	void RegisterDebugPage();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ZonaMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OoplasmMaterial;

	bool bDebugPageRegistered = false;
};
