// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisSpermSwimTypes.h"
#include "GenesisFluidParticles.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * Schwebeteilchen der Eileiterflüssigkeit: Zelltrümmer, abgelöste Epithelzellen, Sekretflocken (0,3–2 µm).
 *
 * Sie treiben mit dem Zilienstrom Richtung Gebärmutter und zittern leicht (Brownsche Bewegung; bei dieser Größe
 * in einer zähen Flüssigkeit nur schwach). Sie sind der wichtigste Hinweis darauf, dass die Szene in Flüssigkeit spielt.
 */
UCLASS()
class GENESISCONCEPTION_API AGenesisFluidParticles : public AActor
{
	GENERATED_BODY()

public:
	AGenesisFluidParticles();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return false; }

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Genesis|Conception")
	void RebuildParticles();

	/** Bis zu vier verschiedene Formen, damit keine erkennbaren Wiederholungen entstehen. */
	UPROPERTY(EditAnywhere, Category = "Particles")
	TArray<TObjectPtr<UStaticMesh>> Meshes;

	UPROPERTY(EditAnywhere, Category = "Particles")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Particles", meta = (ClampMin = "0", ClampMax = "20000"))
	int32 ParticleCount = 600;

	UPROPERTY(EditAnywhere, Category = "Particles")
	int32 Seed = 3;

	UPROPERTY(EditAnywhere, Category = "Particles")
	FGenesisOviductChannel Channel;

	/** Größenbereich der Teilchen in µm (Skalierung der Meshes, die ~1 µm groß sind). */
	UPROPERTY(EditAnywhere, Category = "Particles")
	FFloatInterval SizeUm = FFloatInterval(0.3f, 2.0f);

	/** Zufallsbewegung in der Flüssigkeit (µm/s); bei dieser Teilchengröße klein. */
	UPROPERTY(EditAnywhere, Category = "Particles", meta = (ClampMin = "0"))
	float BrownianSpeedUm = 1.5f;

	/** Simulationszeit je Echtzeitsekunde – wie beim Schwarm (Hochgeschwindigkeitsaufnahme). */
	UPROPERTY(EditAnywhere, Category = "Particles", meta = (ClampMin = "0", ClampMax = "4"))
	float TimeScale = 0.25f;

private:
	struct FParticle
	{
		FVector Position = FVector::ZeroVector;
		FVector Drift = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector SpinAxis = FVector::UpVector;
		float SpinSpeed = 0.0f;
		float Scale = 1.0f;
		int32 Component = 0;
	};

	void PushInstances();

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;

	TArray<FParticle> Particles;
	TArray<TArray<FTransform>> TransformBuffers;
	FGenesisRandomStream Random;
};
