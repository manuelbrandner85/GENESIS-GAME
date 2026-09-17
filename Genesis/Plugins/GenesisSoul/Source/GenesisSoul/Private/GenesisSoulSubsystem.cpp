// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoulSubsystem.h"
#include "Engine/GameInstance.h"
#include "GenesisDebug.h"
#include "GenesisLog.h"
#include "GenesisSoulLogic.h"
#include "GenesisSoulSettings.h"

void UGenesisSoulSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	RegisterDebugPage();
}

void UGenesisSoulSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* Registry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			Registry->UnregisterSystem(this);
		}
	}

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Soul"));
#endif

	OnIncarnationBegan.Clear();
	OnIncarnationClosed.Clear();
	Super::Deinitialize();
}

bool UGenesisSoulSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(Archive, OutPayload);
}

bool UGenesisSoulSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisSoulArchive Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	Archive = MoveTemp(Loaded);
	return true;
}

void UGenesisSoulSubsystem::ResetState()
{
	Archive = FGenesisSoulArchive();
}

bool UGenesisSoulSubsystem::CreatePlayerSoul(uint64 OriginSeed)
{
	if (HasPlayerSoul())
	{
		UE_LOG(LogGenesis, Warning, TEXT("Soul: Spielerseele existiert bereits – wird nicht überschrieben."));
		return false;
	}

	const UGenesisSoulSettings* Settings = GetDefault<UGenesisSoulSettings>();
	Archive.PlayerSoul = GenesisSoulLogic::CreateSoulSeed(OriginSeed, Settings->InnatePatternPool);
	UE_LOG(LogGenesis, Log, TEXT("Soul: neue Seele %s"), *Archive.PlayerSoul.SoulId.ToString());
	return true;
}

bool UGenesisSoulSubsystem::BeginIncarnation(const FGenesisIncarnationRecord& Template)
{
	const FGenesisIncarnationRecord* Record = GenesisSoulLogic::BeginIncarnation(Archive.PlayerSoul, Template);
	if (!Record)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Soul: Inkarnation kann nicht beginnen (keine Seele oder laufende Inkarnation)."));
		return false;
	}
	OnIncarnationBegan.Broadcast(*Record);
	return true;
}

bool UGenesisSoulSubsystem::CloseIncarnation(const FGenesisLifeClosure& Closure)
{
	if (!GenesisSoulLogic::CloseIncarnation(Archive.PlayerSoul, Closure, GetDefault<UGenesisSoulSettings>()->CarryOver))
	{
		UE_LOG(LogGenesis, Warning, TEXT("Soul: keine offene Inkarnation zum Abschließen."));
		return false;
	}
	OnIncarnationClosed.Broadcast(Archive.PlayerSoul.Incarnations.Last());
	return true;
}

void UGenesisSoulSubsystem::ReinforcePlayerResonance(const FGameplayTag& Pattern, float Amount)
{
	if (HasPlayerSoul())
	{
		GenesisSoulLogic::ReinforceResonance(Archive.PlayerSoul, Pattern, Amount, GetDefault<UGenesisSoulSettings>()->CarryOver);
	}
}

float UGenesisSoulSubsystem::GetPlayerResonance(const FGameplayTag& PatternQuery) const
{
	return GenesisSoulLogic::GetResonanceIntensity(Archive.PlayerSoul, PatternQuery);
}

float UGenesisSoulSubsystem::GetPlayerEchoPull(const FGameplayTagContainer& SituationThemes) const
{
	return GenesisSoulLogic::ComputeEchoPull(Archive.PlayerSoul, SituationThemes);
}

float UGenesisSoulSubsystem::GetPlayerRecognitionOf(const FGuid& OtherSoulId, const FGameplayTagContainer& ContextThemes) const
{
	return GenesisSoulLogic::ComputeRecognition(Archive.PlayerSoul, OtherSoulId, ContextThemes);
}

const FGenesisSoulSeed& UGenesisSoulSubsystem::GetOrCreateCompanionSoul(uint64 OriginSeed)
{
	for (const FGenesisSoulSeed& Existing : Archive.CompanionSouls)
	{
		if (Existing.OriginSeed == OriginSeed)
		{
			return Existing;
		}
	}
	return Archive.CompanionSouls.Add_GetRef(GenesisSoulLogic::CreateSoulSeed(OriginSeed, GetDefault<UGenesisSoulSettings>()->InnatePatternPool));
}

const FGenesisSoulSeed* UGenesisSoulSubsystem::FindSoul(const FGuid& SoulId) const
{
	if (Archive.PlayerSoul.SoulId == SoulId)
	{
		return &Archive.PlayerSoul;
	}
	return Archive.CompanionSouls.FindByPredicate([&SoulId](const FGenesisSoulSeed& Soul) { return Soul.SoulId == SoulId; });
}

void UGenesisSoulSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisSoulSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Soul"),
		TEXT("Soul Engine"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisSoulSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->HasPlayerSoul())
			{
				OutLines.Add(TEXT("Keine Spielerseele."));
				return;
			}

			const FGenesisSoulSeed& Soul = Self->Archive.PlayerSoul;
			OutLines.Add(FString::Printf(TEXT("SoulId %s | Seed %llu | Inkarnationen %d%s | Loslassen %.2f"),
				*Soul.SoulId.ToString(EGuidFormats::Short), Soul.OriginSeed, Soul.Incarnations.Num(),
				Soul.IsIncarnated() ? TEXT(" (aktiv)") : TEXT(""), Soul.Detachment));

			TArray<FString> Intervals;
			for (int32 Interval : Soul.Motif.Intervals)
			{
				Intervals.Add(FString::FromInt(Interval));
			}
			OutLines.Add(FString::Printf(TEXT("Motiv [%s] Modus %d Spannung %.2f Wärme %.2f Variation %d"),
				*FString::Join(Intervals, TEXT(",")), Soul.Motif.ModeIndex, Soul.Motif.Tension, Soul.Motif.Warmth, Soul.Motif.Variation));

			for (const FGenesisSoulResonance& Resonance : Soul.Resonances)
			{
				OutLines.Add(FString::Printf(TEXT("  Resonanz %-40s %.2f (x%d)"), *Resonance.Pattern.ToString(), Resonance.Intensity, Resonance.ReinforcementCount));
			}
			for (const FGenesisSoulEcho& Echo : Soul.Echoes)
			{
				OutLines.Add(FString::Printf(TEXT("  Echo %-44s %.2f %s (x%d)"), *Echo.Theme.ToString(), Echo.Weight,
					Echo.State == EGenesisEchoState::Open ? TEXT("offen") : TEXT("integriert"), Echo.ManifestationCount));
			}
			OutLines.Add(FString::Printf(TEXT("  Bindungen: %d | Begleiterseelen: %d"), Soul.Bonds.Num(), Self->Archive.CompanionSouls.Num()));
		}
	});
#endif
}
