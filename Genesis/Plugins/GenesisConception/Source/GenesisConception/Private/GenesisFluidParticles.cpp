// GENESIS: Der Kreislauf des Lebens

#include "GenesisFluidParticles.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GenesisRandom.h"
#include "GenesisSpermSwimLogic.h"

namespace
{
	constexpr int32 MaxComponents = 4;
	constexpr uint64 ParticleSalt = 0xF1E1Dull;
}

AGenesisFluidParticles::AGenesisFluidParticles()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	for (int32 Index = 0; Index < MaxComponents; ++Index)
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Particles%d"), Index));
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Winzige Teilchen: keine Schatten, keine Distanzfelder – beides wäre bei dieser Größe unsichtbar und teuer
		Component->SetCastShadow(false);
		Component->bAffectDistanceFieldLighting = false;
		Components.Add(Component);
	}
}

void AGenesisFluidParticles::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildParticles();
}

void AGenesisFluidParticles::BeginPlay()
{
	Super::BeginPlay();
	RebuildParticles();
}

void AGenesisFluidParticles::RebuildParticles()
{
	Random = FGenesisRandomStream(GenesisHash::Combine(static_cast<uint64>(Seed), ParticleSalt));
	Particles.Reset(ParticleCount);
	TransformBuffers.SetNum(Components.Num());

	TArray<int32> UsableComponents;
	for (int32 Index = 0; Index < Components.Num(); ++Index)
	{
		UInstancedStaticMeshComponent* Component = Components[Index];
		if (!Component)
		{
			continue;
		}
		UStaticMesh* Mesh = Meshes.IsValidIndex(Index) ? Meshes[Index].Get() : nullptr;
		Component->ClearInstances();
		Component->SetStaticMesh(Mesh);
		if (Material && Mesh)
		{
			Component->SetMaterial(0, Material);
		}
		if (Mesh)
		{
			UsableComponents.Add(Index);
		}
		TransformBuffers[Index].Reset();
	}
	if (UsableComponents.Num() == 0)
	{
		return;
	}

	const float Limit = FMath::Max(10.0f, Channel.LumenRadiusUm - 5.0f);
	for (int32 Index = 0; Index < ParticleCount; ++Index)
	{
		FParticle& Particle = Particles.AddDefaulted_GetRef();
		const double Radius = Limit * FMath::Sqrt(Random.NextDouble());
		const double Angle = 2.0 * UE_DOUBLE_PI * Random.NextDouble();
		Particle.Position = FVector(Random.NextDouble() * Channel.LengthUm, Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
		Particle.Scale = FMath::Lerp(SizeUm.Min, SizeUm.Max, FMath::Pow(Random.NextFloat(), 2.0f));  // viele kleine, wenige große
		Particle.Rotation = FQuat(FVector(Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f)).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector), Random.FRandRange(0.0f, 2.0f * PI));
		Particle.SpinAxis = FVector(Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f)).GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
		Particle.SpinSpeed = Random.FRandRange(-0.4f, 0.4f);
		Particle.Component = UsableComponents[Random.RandRange(0, UsableComponents.Num() - 1)];
		TransformBuffers[Particle.Component].Add(FTransform(Particle.Rotation, Particle.Position, FVector(Particle.Scale)));
	}

	for (int32 Index : UsableComponents)
	{
		Components[Index]->AddInstances(TransformBuffers[Index], false, false, false);
	}
}

void AGenesisFluidParticles::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Dt = DeltaSeconds * TimeScale;
	if (Dt <= 0.0f || Particles.Num() == 0)
	{
		return;
	}

	for (TArray<FTransform>& Buffer : TransformBuffers)
	{
		Buffer.Reset();
	}

	const double Limit = FMath::Max(10.0f, Channel.LumenRadiusUm - 5.0f);
	for (FParticle& Particle : Particles)
	{
		// Zilienstrom plus schwache Zufallsbewegung
		const FVector Flow = GenesisSpermSwimLogic::FlowAt(Particle.Position, Channel);
		Particle.Drift += FVector(Random.Gaussian(0.0f, BrownianSpeedUm), Random.Gaussian(0.0f, BrownianSpeedUm), Random.Gaussian(0.0f, BrownianSpeedUm)) * Dt;
		Particle.Drift *= FMath::Exp(-Dt / 0.8f);  // Reibung in zäher Flüssigkeit
		Particle.Position += (Flow + Particle.Drift) * Dt;

		const double Radius = FVector2D(Particle.Position.Y, Particle.Position.Z).Size();
		if (Radius > Limit)
		{
			Particle.Position.Y *= Limit / Radius;
			Particle.Position.Z *= Limit / Radius;
		}
		Particle.Position.X = FMath::Fmod(Particle.Position.X, static_cast<double>(Channel.LengthUm));
		if (Particle.Position.X < 0.0)
		{
			Particle.Position.X += Channel.LengthUm;
		}

		Particle.Rotation = FQuat(Particle.SpinAxis, Particle.SpinSpeed * Dt) * Particle.Rotation;
		TransformBuffers[Particle.Component].Add(FTransform(Particle.Rotation, Particle.Position, FVector(Particle.Scale)));
	}

	PushInstances();
}

void AGenesisFluidParticles::PushInstances()
{
	for (int32 Index = 0; Index < Components.Num(); ++Index)
	{
		UInstancedStaticMeshComponent* Component = Components[Index];
		if (Component && TransformBuffers.IsValidIndex(Index) && Component->GetInstanceCount() == TransformBuffers[Index].Num())
		{
			Component->BatchUpdateInstancesTransforms(0, TransformBuffers[Index], false, true, false);
		}
	}
}
