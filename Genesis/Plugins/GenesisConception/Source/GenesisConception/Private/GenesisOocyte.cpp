// GENESIS: Der Kreislauf des Lebens

#include "GenesisOocyte.h"
#include "Components/StaticMeshComponent.h"
#include "GenesisDebug.h"
#include "Materials/MaterialInstanceDynamic.h"

AGenesisOocyte::AGenesisOocyte()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	auto MakePart = [this, Root](const TCHAR* Name, bool bCastShadow)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Root);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(bCastShadow);
		return Component;
	};

	Ooplasm = MakePart(TEXT("Ooplasm"), true);
	Zona = MakePart(TEXT("Zona"), false);
	PolarBody = MakePart(TEXT("PolarBody"), false);
	Corona = MakePart(TEXT("Corona"), false);
	CumulusMatrix = MakePart(TEXT("CumulusMatrix"), false);
	// Die Fäden der Hyaluronsäure-Matrix zwischen den Zellen: Sie halten den expandierten Cumulus
	// zusammen und bremsen die Spermien, bevor diese die Zona erreichen.
	CumulusStrands = MakePart(TEXT("CumulusStrands"), false);
}

void AGenesisOocyte::BeginPlay()
{
	Super::BeginPlay();
	State.Position = GetActorLocation();

	if (Zona && Zona->GetMaterial(0))
	{
		ZonaMaterial = Zona->CreateDynamicMaterialInstance(0);
	}
	if (Ooplasm && Ooplasm->GetMaterial(0))
	{
		OoplasmMaterial = Ooplasm->CreateDynamicMaterialInstance(0);
	}
	RegisterDebugPage();
}

void AGenesisOocyte::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Die Cortikalreaktion ist sichtbar: Die Zona verändert sich, sobald die Eizelle befruchtet ist
	if (ZonaMaterial)
	{
		ZonaMaterial->SetScalarParameterValue(TEXT("CorticalReaction"), State.CorticalReaction);
	}
	if (OoplasmMaterial)
	{
		OoplasmMaterial->SetScalarParameterValue(TEXT("CorticalReaction"), State.CorticalReaction);
	}
}

void AGenesisOocyte::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;
	TWeakObjectPtr<AGenesisOocyte> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Oocyte"),
		TEXT("Eizelle"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const AGenesisOocyte* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			const FGenesisOocyteState& State = Self->State;
			OutLines.Add(FString::Printf(TEXT("Zellleib %.0f µm | Zona %.0f–%.0f µm | Cumulus %.0f µm | gebundene Zellen %d"),
				2.0f * State.OoplasmRadiusUm, State.ZonaInnerRadiusUm, State.ZonaOuterRadiusUm, State.CumulusRadiusUm, State.BoundCells));
			if (State.IsFertilized())
			{
				OutLines.Add(FString::Printf(TEXT("Befruchtet von Zelle %d | seit %.1f s | Cortikalreaktion %.0f %% | Zona %s"),
					State.FertilizedByCell, State.SecondsSinceFusion, 100.0f * State.CorticalReaction,
					State.bZonaHardened ? TEXT("verhärtet (Polyspermie-Block)") : TEXT("noch durchlässig")));
			}
			else
			{
				OutLines.Add(TEXT("Noch unbefruchtet"));
			}
		}
	});
#endif
}
