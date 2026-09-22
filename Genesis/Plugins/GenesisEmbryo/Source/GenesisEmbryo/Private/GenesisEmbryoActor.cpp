// GENESIS: Der Kreislauf des Lebens

#include "GenesisEmbryoActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "GenesisEmbryoSubsystem.h"
#include "GenesisEmbryoLogic.h"
#include "GenesisOocyte.h"
#include "GenesisSpermSwarm.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	/** Radius der Zellkugel des Grundmesh (SM_GEN_OocyteCytoplasm) in µm. */
	constexpr float BaseCellRadiusUm = 55.0f;
	/** Je Zelle: Farbton, Embryoblast, Fragmentierung, Kern sichtbar (0..1), Vorkerne (0/1). */
	constexpr int32 CellDataCount = 5;
}

AGenesisEmbryo::AGenesisEmbryo()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	Blastomeres = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Blastomeres"));
	SetRootComponent(Blastomeres);
	Blastomeres->SetMobility(EComponentMobility::Movable);
	Blastomeres->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blastomeres->SetNumCustomDataFloats(CellDataCount);
	Blastomeres->SetCastShadow(true);
	Blastomeres->bAffectDistanceFieldLighting = false;

	Fragments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Fragments"));
	Fragments->SetupAttachment(Blastomeres);
	Fragments->SetMobility(EComponentMobility::Movable);
	Fragments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Fragments->SetNumCustomDataFloats(CellDataCount);
	Fragments->SetCastShadow(true);
	Fragments->bAffectDistanceFieldLighting = false;
}

void AGenesisEmbryo::BeginPlay()
{
	Super::BeginPlay();

	// Eine dynamische Materialinstanz für beide Komponenten: Die Lage des optischen Schnitts ändert sich je Bild
	SectionMaterial = CellMaterial ? UMaterialInstanceDynamic::Create(CellMaterial, this) : nullptr;
	if (Blastomeres)
	{
		Blastomeres->SetStaticMesh(CellMesh);
		if (SectionMaterial)
		{
			Blastomeres->SetMaterial(0, SectionMaterial);
		}
		Blastomeres->ClearInstances();
		Blastomeres->SetNumCustomDataFloats(CellDataCount);
	}
	if (Fragments)
	{
		Fragments->SetStaticMesh(CellMesh);
		if (SectionMaterial)
		{
			Fragments->SetMaterial(0, SectionMaterial);
		}
		Fragments->ClearInstances();
		Fragments->SetNumCustomDataFloats(CellDataCount);
	}
	LastCellCount = -1;
	LastFragmentCount = -1;
}

void AGenesisEmbryo::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisEmbryoSubsystem* Embryo = GameInstance ? GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>() : nullptr;
	if (!Embryo || !Embryo->HasEmbryo())
	{
		return;
	}

	const FGenesisEmbryoState& State = Embryo->GetState();
	PushCells(State, Embryo->Tuning);
	PushFragments(State);
	UpdateOocyteRemains(State);
	UpdateSection(State);
}

void AGenesisEmbryo::UpdateSection(const FGenesisEmbryoState& State)
{
	if (!SectionMaterial)
	{
		return;
	}
	// Ein Mikroskop zeigt eine dünne Ebene durch die Mitte. Bei den Furchungsstadien sieht das aus wie die
	// Zellen von außen (ihr Umriss ist ihr Äquator). Erst die Blastozyste ist hohl: Dann gehört die vordere
	// Kappe der Hülle ausgeblendet, damit Hohlraum, Trophoblast-Ring und Embryoblast zu sehen sind.
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	float Front = -1.0e6f;
	float Back = 1.0e6f;
	if (CameraManager && State.Cavity > 0.02f && bMicroscopeView)
	{
		const FVector Camera = CameraManager->GetCameraLocation();
		const FVector Forward = CameraManager->GetCameraRotation().Vector();
		const float CenterDepth = static_cast<float>(FVector::DotProduct(GetActorLocation() - Camera, Forward));
		const float Radius = State.GetOuterRadiusUm();
		// Mit wachsendem Hohlraum zieht sich die Schicht von „ganzer Keim" auf eine Scheibe um die Mitte zusammen:
		// vorn und hinten je SectionMarginFraction des Radius. Die hintere Kappe als Mosaik durch den Hohlraum
		// zu sehen, wäre ebenso falsch wie die vordere (gesehen bei 118 h).
		const float Open = FMath::SmoothStep(0.02f, 0.4f, State.Cavity);
		const float Half = FMath::Lerp(1.3f * Radius, SectionMarginFraction * Radius, Open);

		// Den Embryoblast in die Schnittebene drehen, wie Embryologen es am Mikroskop tun („ICM auf 3 Uhr").
		// Lag er zufällig vorn oder hinten, schnitt der optische Schnitt ihn weg – die Blastozyste war
		// dann ein leerer Ring (gesehen bei 118 h).
		FVector Inner = FVector::ZeroVector;
		for (const FGenesisBlastomere& Cell : State.Cells)
		{
			Inner += Cell.bInnerCellMass ? Cell.Position : FVector::ZeroVector;
		}
		if (!Inner.IsNearlyZero())
		{
			const FVector Current = GetActorQuat().RotateVector(Inner.GetSafeNormal());
			const FVector Right = CameraManager->GetCameraRotation().Quaternion().GetRightVector();
			const FQuat Delta = FQuat::FindBetweenNormals(Current, Right);
			const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
			const float Alpha = 1.0f - FMath::Exp(-DeltaSeconds / 2.5f);
			SetActorRotation(FQuat::Slerp(FQuat::Identity, Delta, Alpha) * GetActorQuat());
		}
		Front = CenterDepth - Half;
		Back = CenterDepth + Half;
	}
	SectionMaterial->SetScalarParameterValue(TEXT("SectionFrontUm"), Front);
	SectionMaterial->SetScalarParameterValue(TEXT("SectionBackUm"), Back);
}

void AGenesisEmbryo::PushFragments(const FGenesisEmbryoState& State)
{
	if (!Fragments || !CellMesh)
	{
		return;
	}
	// Erst nach dem Schlüpfen verlieren sich die Trümmer: Sie bleiben in der Zona zurück
	const bool bInsideZona = State.ZonaThicknessUm > 0.0f;
	const int32 Count = bInsideZona ? FMath::Clamp(FMath::RoundToInt(State.Fragmentation * MaxFragments), 0, MaxFragments) : 0;
	if (Count == LastFragmentCount)
	{
		return;
	}
	LastFragmentCount = Count;
	Fragments->ClearInstances();
	if (Count == 0)
	{
		return;
	}

	// Deterministisch aus dem Keim: dieselben Trümmer an denselben Stellen, Bild für Bild
	FRandomStream Random(static_cast<int32>(State.Seed ^ (State.Seed >> 32)) ^ 0x5F7A11);
	const float Inner = State.GetOuterRadiusUm() * 0.82f;
	const float Outer = 58.0f; // Innenrand der Zona
	TArray<FTransform> Transforms;
	TArray<float> Data;
	Transforms.Reserve(Count);
	Data.Reserve(Count * CellDataCount);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// Fragmente liegen im Spalt zwischen Zellen und Hülle, gehäuft dort, wo schon welche liegen
		const FVector Direction = Random.GetUnitVector();
		const float Radius = FMath::Lerp(Inner, Outer - 2.0f, Random.FRand());
		const float Size = FMath::Lerp(1.5f, 5.0f, FMath::Square(Random.FRand())) / BaseCellRadiusUm;
		Transforms.Emplace(FQuat::Identity, Direction * Radius, FVector(Size * Random.FRandRange(0.8f, 1.2f), Size, Size * Random.FRandRange(0.7f, 1.0f)));
		// Dunkler als lebende Zellen: Trümmer haben keinen Kern und zerfallen
		Data.Add(0.1f);
		Data.Add(0.0f);
		Data.Add(1.0f);
		Data.Add(0.0f); // Trümmer haben keinen Kern
		Data.Add(0.0f);
	}
	Fragments->PreAllocateInstancesMemory(Count);
	Fragments->AddInstances(Transforms, false, false, false);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Fragments->SetCustomData(Index, TArrayView<const float>(&Data[Index * CellDataCount], CellDataCount), false);
	}
	Fragments->MarkRenderStateDirty();
}

void AGenesisEmbryo::PushCells(const FGenesisEmbryoState& State, const FGenesisEmbryoTuning& Tuning)
{
	if (!Blastomeres || !CellMesh)
	{
		return;
	}

	const int32 Count = State.Cells.Num();
	TransformBuffer.SetNum(Count);
	CustomDataBuffer.SetNum(Count * CellDataCount);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FGenesisBlastomere& Cell = State.Cells[Index];
		const float Scale = Cell.RadiusUm / BaseCellRadiusUm;
		// Die Zellen sind bei der Kompaktierung nicht mehr kugelrund – sie drücken sich gegenseitig flach
		const float Squash = 1.0f - 0.12f * State.Compaction;
		// Keine zwei Zellen sind exakt gleich groß – ohne Streuung sieht der Keim aus wie Luftpolsterfolie
		const float Individual = 0.88f + 0.24f * Cell.Tint;
		// Der Trophoblast ist ein Deckgewebe: Seine Zellen liegen flach an der Hülle und schließen dicht ab.
		// Als Kugeln blieben Lücken, durch die man in den Keim sähe.
		if (State.Cavity > 0.05f && !Cell.bInnerCellMass)
		{
			const FVector Outward = Cell.Position.GetSafeNormal(UE_DOUBLE_SMALL_NUMBER, FVector::UpVector);
			const float Flatten = FMath::Lerp(1.0f, 0.40f, State.Cavity);
			const float Spread = FMath::Sqrt(1.0f / Flatten); // Volumen bleibt erhalten
			TransformBuffer[Index] = FTransform(FRotationMatrix::MakeFromZ(Outward).ToQuat(), Cell.Position,
				FVector(Scale * Spread * Individual, Scale * Spread / Individual, Scale * Flatten));
		}
		else
		{
			// Volumen bleibt erhalten: Was in einer Achse länger ist, ist in der anderen schmaler
			TransformBuffer[Index] = FTransform(FQuat::Identity, Cell.Position,
				FVector(Scale * Individual, Scale / Individual, Scale * Squash));
		}

		CustomDataBuffer[Index * CellDataCount + 0] = Cell.Tint;
		CustomDataBuffer[Index * CellDataCount + 1] = Cell.bInnerCellMass ? 1.0f : 0.0f;
		CustomDataBuffer[Index * CellDataCount + 2] = State.Fragmentation;
		float NucleusVisibility = 0.0f;
		bool bPronuclei = false;
		GenesisEmbryoLogic::GetNucleusDisplay(State, Index, Tuning, NucleusVisibility, bPronuclei);
		CustomDataBuffer[Index * CellDataCount + 3] = NucleusVisibility;
		CustomDataBuffer[Index * CellDataCount + 4] = bPronuclei ? 1.0f : 0.0f;
	}

	if (LastCellCount != Count)
	{
		// Zellzahl ändert sich nur bei einer Teilung – dann wird die Instanzliste neu aufgebaut
		Blastomeres->ClearInstances();
		Blastomeres->PreAllocateInstancesMemory(Count);
		Blastomeres->AddInstances(TransformBuffer, false, false, false);
		LastCellCount = Count;
	}
	else
	{
		Blastomeres->BatchUpdateInstancesTransforms(0, TransformBuffer, false, false, false);
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		Blastomeres->SetCustomData(Index, TArrayView<const float>(&CustomDataBuffer[Index * CellDataCount], CellDataCount), false);
	}
	Blastomeres->MarkRenderStateDirty();
}

void AGenesisEmbryo::UpdateOocyteRemains(const FGenesisEmbryoState& State)
{
	if (!Oocyte)
	{
		return;
	}

	// Der Zellleib der Eizelle ist jetzt der Keim selbst – er wird vom Actor dargestellt
	if (Oocyte->Ooplasm)
	{
		Oocyte->Ooplasm->SetVisibility(false);
	}

	// Der Cumulus löst sich in den Stunden nach der Befruchtung auf – Zellkranz, Gallerte und ihre Fäden.
	// Vorher blieben Gallerte und Fäden die ganze Woche stehen und verdeckten den Keim (gesehen bei 90 hpi).
	const float Dispersal = FMath::Clamp(static_cast<float>(State.HoursSinceFusion) / FMath::Max(1.0f, CoronaDispersalHours), 0.0f, 1.0f);
	for (UStaticMeshComponent* Part : { Oocyte->Corona.Get(), Oocyte->CumulusMatrix.Get(), Oocyte->CumulusStrands.Get() })
	{
		if (Part)
		{
			Part->SetVisibility(Dispersal < 1.0f);
			// Die Zellen treiben nach außen weg, bevor sie verschwinden
			Part->SetRelativeScale3D(FVector(1.0f + 0.35f * Dispersal));
		}
	}

	// Die Zona bleibt, bis der Keim schlüpft
	if (Oocyte->Zona)
	{
		Oocyte->Zona->SetVisibility(State.ZonaThicknessUm > 0.0f);
		// Die Zona dehnt sich mit dem Keim, bis sie reißt
		const float Expansion = 1.0f + 0.35f * State.Cavity;
		Oocyte->Zona->SetRelativeScale3D(FVector(Expansion));
	}

	// Die übrigen Spermien: Nach gut einem Tag sind sie abgestorben oder weitergetrieben
	if (State.HoursSinceFusion > SwarmGoneHours)
	{
		for (TActorIterator<AGenesisSpermSwarm> It(GetWorld()); It; ++It)
		{
			if (!It->IsHidden())
			{
				It->SetActorHiddenInGame(true);
				It->SetActorTickEnabled(false);
			}
		}
	}

	if (Oocyte->PolarBody)
	{
		// Der Polkörper zerfällt in den ersten Tagen
		Oocyte->PolarBody->SetVisibility(State.HoursSinceFusion < 72.0);
	}
}
