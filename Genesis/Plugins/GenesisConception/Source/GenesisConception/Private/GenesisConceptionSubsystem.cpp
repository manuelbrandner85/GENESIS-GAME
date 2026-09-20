// GENESIS: Der Kreislauf des Lebens

#include "GenesisConceptionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GenesisBodySubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisDebug.h"
#include "GenesisGameplayTags.h"
#include "GenesisGeneticsGameplayTags.h"
#include "GenesisGeneticsSubsystem.h"
#include "GenesisLog.h"
#include "GenesisRandom.h"
#include "GenesisSoulMusicSubsystem.h"
#include "GenesisSoulSubsystem.h"
#include "GenesisSoulTypes.h"
#include "GenesisWorldClockSubsystem.h"

void UGenesisConceptionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	RegisterDebugPage();
}

void UGenesisConceptionSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Conceived"));
#endif
	Super::Deinitialize();
}

bool UGenesisConceptionSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(Record, OutPayload);
}

bool UGenesisConceptionSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisConceptionRecord Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	Record = MoveTemp(Loaded);
	return true;
}

void UGenesisConceptionSubsystem::ResetState()
{
	Record = FGenesisConceptionRecord();
}

const FGenesisConceptionRecord& UGenesisConceptionSubsystem::Conceive(const FGenesisFertilizationResult& Fertilization)
{
	if (Record.bConceived)
	{
		// Eine Welt, eine Zeugung – ein zweiter Aufruf darf das Leben nicht überschreiben
		return Record;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisGeneticsSubsystem* Genetics = GameInstance ? GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
	if (!Genetics)
	{
		UE_LOG(LogGenesis, Error, TEXT("Conception: Ohne Genetik entsteht kein Genom – Zeugung abgebrochen."));
		return Record;
	}

	// Die beiden Elterngenome des Prologs. Später kommen sie aus der Weltbevölkerung, hier sind es Gründer.
	Record.MotherGenomeId = Genetics->CreateFounderGenome();
	Record.FatherGenomeId = Genetics->CreateFounderGenome();
	Record.GenomeId = Genetics->ConceiveChild(Record.MotherGenomeId, Record.FatherGenomeId);
	if (!Record.GenomeId.IsValid())
	{
		UE_LOG(LogGenesis, Error, TEXT("Conception: Kindgenom konnte nicht gezeugt werden."));
		Record = FGenesisConceptionRecord();
		return Record;
	}

	// Die Person selbst – deterministisch aus dem Genom, damit ein Neustart derselben Welt dieselbe Person ergibt
	const uint64 EntityHigh = GenesisHash::Combine(GenesisHash::FromGuid(Record.GenomeId), 0x50455253ull);
	const uint64 EntityLow = GenesisHash::Mix64(EntityHigh);
	Record.EntityId = FGuid(static_cast<uint32>(EntityHigh >> 32), static_cast<uint32>(EntityHigh),
		static_cast<uint32>(EntityLow >> 32), static_cast<uint32>(EntityLow));
	Record.SecondsToFusion = Fertilization.SecondsToFusion;
	Record.CompetingCells = Fertilization.CompetingCells;
	Record.BlockedCells = Fertilization.BlockedCells;

	// Startwerte des ersten Körpers: Lebenskraft aus der erfolgreichen Zelle,
	// Widerstandskraft aus dem geerbten Genom (geringes Herz-Kreislauf-Risiko, kräftiger Stoffwechsel)
	const float CardiovascularRisk = Genetics->ExpressTrait(Record.GenomeId, GenesisGeneticsTags::Trait_Risk_Cardiovascular, 0.5f);
	const float Metabolism = Genetics->ExpressTrait(Record.GenomeId, GenesisGeneticsTags::Trait_Body_Metabolism, 0.5f);
	Record.Vitality = FMath::Clamp(Fertilization.Vitality, 0.0f, 1.0f);
	Record.Resilience = FMath::Clamp(0.6f * (1.0f - CardiovascularRisk) + 0.4f * Metabolism, 0.0f, 1.0f);

	const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
	Record.ConceptionTime = Clock ? Clock->GetNow() : FGenesisTimestamp();

	if (UGenesisBodySubsystem* Body = GameInstance->GetSubsystem<UGenesisBodySubsystem>())
	{
		FGenesisConceptionVitality Vitality;
		Vitality.Vitality = Record.Vitality;
		Vitality.Resilience = Record.Resilience;
		Body->CreateBodyAtConception(Record.EntityId, Record.GenomeId, Vitality, EGenesisSimulationLevel::Full);
	}

	// Die Seele nimmt diesen Körper an. Gibt es noch keine Spielerseele, entsteht sie hier deterministisch aus dem Genom.
	if (UGenesisSoulSubsystem* Soul = GameInstance->GetSubsystem<UGenesisSoulSubsystem>())
	{
		if (!Soul->HasPlayerSoul())
		{
			Soul->CreatePlayerSoul(GenesisHash::FromGuid(Record.GenomeId));
		}
		FGenesisIncarnationRecord Incarnation;
		Incarnation.EntityId = Record.EntityId;
		Incarnation.GenomeId = Record.GenomeId;
		Incarnation.BirthTime = Record.ConceptionTime;
		Soul->BeginIncarnation(Incarnation);
	}

	// Das Leitmotiv erbt von beiden Eltern und erklingt zum ersten Mal in der Phase "Zeugung".
	// Die Eltern des Prologs existieren noch nicht als Personen – ihre Motive leitet die Musik deterministisch
	// aus den Genom-IDs ab, bis GenesisWorld echte Eltern liefert.
	if (UGenesisSoulMusicSubsystem* Music = GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>())
	{
		Music->CreateInheritedMotif(Record.EntityId, Record.MotherGenomeId, Record.FatherGenomeId);
		Music->NotifyLifePhase(Record.EntityId, GenesisTags::LifePhase_Conception, Record.ConceptionTime);
	}

	Record.bConceived = true;
	UE_LOG(LogGenesis, Display, TEXT("Conception: Person %s gezeugt (Genom %s, Lebenskraft %.2f, Widerstandskraft %.2f, %.1f s bis zur Verschmelzung)."),
		*Record.EntityId.ToString(EGuidFormats::Short), *Record.GenomeId.ToString(EGuidFormats::Short),
		Record.Vitality, Record.Resilience, Record.SecondsToFusion);

	OnConceived.Broadcast(Record);
	return Record;
}

void UGenesisConceptionSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisConceptionSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Conceived"),
		TEXT("Conception – gezeugtes Leben"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisConceptionSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			const FGenesisConceptionRecord& Record = Self->Record;
			if (!Record.bConceived)
			{
				OutLines.Add(TEXT("Noch kein Leben gezeugt."));
				return;
			}
			OutLines.Add(FString::Printf(TEXT("Person %s | Genom %s | Eltern %s + %s"),
				*Record.EntityId.ToString(EGuidFormats::Short), *Record.GenomeId.ToString(EGuidFormats::Short),
				*Record.MotherGenomeId.ToString(EGuidFormats::Short), *Record.FatherGenomeId.ToString(EGuidFormats::Short)));
			OutLines.Add(FString::Printf(TEXT("Lebenskraft %.2f | Widerstandskraft %.2f | Weg %.1f s | Mitbewerber %d | abgewiesen %d"),
				Record.Vitality, Record.Resilience, Record.SecondsToFusion, Record.CompetingCells, Record.BlockedCells));
		}
	});
#endif
}
