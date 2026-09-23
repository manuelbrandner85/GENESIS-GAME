// GENESIS: Der Kreislauf des Lebens

#include "GenesisOocyte.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GenesisFertilizationLogic.h"
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
	// Dasselbe Bauteil neben dem ersten, an der Stelle der Spindel (beide liegen am animalen Pol)
	SecondPolarBody = MakePart(TEXT("SecondPolarBody"), false);
	SecondPolarBody->SetRelativeRotation(FRotator(0.0f, 0.0f, 16.0f));
	SecondPolarBody->SetVisibility(false);
	Corona = MakePart(TEXT("Corona"), false);
	CumulusMatrix = MakePart(TEXT("CumulusMatrix"), false);
	// Die Fäden der Hyaluronsäure-Matrix zwischen den Zellen: Sie halten den expandierten Cumulus
	// zusammen und bremsen die Spermien, bevor diese die Zona erreichen.
	CumulusStrands = MakePart(TEXT("CumulusStrands"), false);

	for (int32 Variant = 0; Variant < CumulusVariantCount; ++Variant)
	{
		UInstancedStaticMeshComponent* Cells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("CumulusCells%d"), Variant));
		Cells->SetupAttachment(Root);
		Cells->SetMobility(EComponentMobility::Movable);
		Cells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// Ton je Zelle und Lage im Komplex (0 = an der Corona, 1 = außen)
		Cells->SetNumCustomDataFloats(2);
		// Kein Schatten: Eine 12 µm dünne, fast klare Zelle hält kaum Licht auf. Mit Schatten warfen die Zellen einander
		// auf kurze Distanz fransige schwarze Sicheln (gesehen in GENESIS-047 Teil 2, mitten in der Wolke).
		Cells->SetCastShadow(false);
		Cells->bAffectDistanceFieldLighting = false;
		// Wie beim Schwarm: Dreizehntausend 10-µm-Zellen spiegeln sich in nichts, kosten in der Strahlenszene aber Speicher
		Cells->SetVisibleInRayTracing(false);
		CumulusCells.Add(Cells);
	}
}

void AGenesisOocyte::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildCumulus();
}

void AGenesisOocyte::RebuildCumulus()
{
	State.Cumulus = GenesisFertilizationLogic::BuildCumulus(State, CumulusTuning);
	const int32 Variants = FMath::Min(CumulusCellMeshes.Num(), CumulusCells.Num());
	for (int32 Variant = 0; Variant < CumulusCells.Num(); ++Variant)
	{
		UInstancedStaticMeshComponent* Cells = CumulusCells[Variant];
		if (!Cells)
		{
			continue;
		}
		Cells->ClearInstances();
		Cells->SetStaticMesh(Variant < Variants ? CumulusCellMeshes[Variant].Get() : nullptr);
		if (CumulusCellMaterial)
		{
			Cells->SetMaterial(0, CumulusCellMaterial);
		}
		Cells->SetNumCustomDataFloats(2);
		Cells->SetCastShadow(false);
	}
	if (Variants == 0 || !State.Cumulus.IsValid())
	{
		return;
	}

	// Das Feld liegt in Kanalachsen um die Eizellmitte; der Actor ist nicht gedreht (wie der Schwarm)
	TArray<TArray<FTransform>> Transforms;
	TArray<TArray<float>> Data;
	Transforms.SetNum(Variants);
	Data.SetNum(Variants);
	const float Span = FMath::Max(1.0f, State.CumulusRadiusUm - State.CoronaRadiusUm);
	const TArray<FGenesisCumulusCell>& Field = State.Cumulus->Cells;
	for (int32 Index = 0; Index < Field.Num(); ++Index)
	{
		const FGenesisCumulusCell& Cell = Field[Index];
		const int32 Variant = Index % Variants;
		Transforms[Variant].Emplace(Cell.Rotation, Cell.Center * GenesisMicroScale::UnitsPerMicrometer, Cell.HalfAxes);
		Data[Variant].Add(Cell.Tint);
		Data[Variant].Add(FMath::Clamp((static_cast<float>(Cell.Center.Size()) - State.CoronaRadiusUm) / Span, 0.0f, 1.0f));
	}
	for (int32 Variant = 0; Variant < Variants; ++Variant)
	{
		UInstancedStaticMeshComponent* Cells = CumulusCells[Variant];
		Cells->PreAllocateInstancesMemory(Transforms[Variant].Num());
		Cells->AddInstances(Transforms[Variant], false, false, false);
		for (int32 Instance = 0; Instance < Transforms[Variant].Num(); ++Instance)
		{
			Cells->SetCustomData(Instance, TArrayView<const float>(&Data[Variant][Instance * 2], 2), false);
		}
		Cells->MarkRenderStateDirty();
	}
}

void AGenesisOocyte::BeginPlay()
{
	Super::BeginPlay();
	State.Position = GetActorLocation();
	// Der Zeiger auf das Zellfeld wird nicht gespeichert: Nach dem Laden neu bauen (deterministisch, dieselben Zellen)
	if (!State.Cumulus.IsValid())
	{
		RebuildCumulus();
	}

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

#if !UE_BUILD_SHIPPING
namespace
{
	TAutoConsoleVariable<int32> CVarHideEgg(TEXT("genesis.Conception.HideEgg"), 0,
		TEXT("Prüfhilfe: 1 = Zellleib, Zona und Kranz ausblenden (was dahinter oder darin liegt, wird sichtbar)."));
}
#endif

void AGenesisOocyte::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	// 1 = alles, 2 = nur Zona, 3 = nur Corona, 4 = nur Zellleib, 5 = nur Fäden, 6 = nur Cumuluszellen
	if (const int32 Hide = CVarHideEgg.GetValueOnGameThread(); Hide > 0)
	{
		if (Hide == 1 || Hide == 6)
		{
			for (UInstancedStaticMeshComponent* Variant : CumulusCells)
			{
				if (Variant)
				{
					Variant->SetVisibility(false);
				}
			}
		}
		const TArray<UStaticMeshComponent*> Parts = { Ooplasm.Get(), Zona.Get(), Corona.Get(), CumulusStrands.Get() };
		const int32 Only[] = { -1, -1, 1, 2, 0, 3 };
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			if (Parts[Index] && (Hide == 1 || (Hide < 6 && Only[Hide] == Index)))
			{
				Parts[Index]->SetVisibility(false);
			}
		}
	}
#endif

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
