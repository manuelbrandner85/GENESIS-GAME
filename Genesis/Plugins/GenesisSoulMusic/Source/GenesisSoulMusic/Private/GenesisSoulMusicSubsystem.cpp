// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoulMusicSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisGameplayTags.h"
#include "GenesisLog.h"
#include "GenesisMemoryTrace.h"
#include "GenesisRandom.h"
#include "GenesisSoulLogic.h"
#include "GenesisSoulMusicLogic.h"
#include "GenesisSoulSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace
{
	/** Salt für Charaktermotive – unabhängig vom Seed der Seele. */
	constexpr uint64 CharacterMotifSalt = 0xC4A2AC7E2ull;
	constexpr uint64 InheritanceSalt = 0x1A4E217ull;
	constexpr uint64 RecallSalt = 0x2EC411ull;

	const FGenesisSoulMusicTuning& Tuning()
	{
		return GetDefault<UGenesisSoulMusicSettings>()->Tuning;
	}

	TPair<FGuid, FGuid> CanonicalPair(const FGuid& A, const FGuid& B)
	{
		return A < B ? TPair<FGuid, FGuid>(A, B) : TPair<FGuid, FGuid>(B, A);
	}

#if !UE_BUILD_SHIPPING
	FAutoConsoleCommandWithWorldAndArgs GenesisMusicScenarioCommand(
		TEXT("genesis.Music.SimulateLife"),
		TEXT("Entwickler: spielt ein ganzes Leben musikalisch durch (Seelenmotiv je Phase, Familienmotiv, Bindung, Erinnerungen, Todeskomposition)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			if (UGenesisSoulMusicSubsystem* Music = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulMusicSubsystem>() : nullptr)
			{
				Music->RunDeveloperScenario();
			}
		}));
#endif
}

bool UGenesisSoulMusicArchive::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisSoulMusicArchive::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisSoulMusicArchiveState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	return true;
}

void UGenesisSoulMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Archive = NewObject<UGenesisSoulMusicArchive>(this);
	if (UGenesisPersistenceRegistry* PersistenceRegistry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		PersistenceRegistry->RegisterSystem(this);
		PersistenceRegistry->RegisterSystem(Archive);
	}
	Collection.InitializeDependency<UGenesisSoulSubsystem>();

	RegisterDebugPage();
}

void UGenesisSoulMusicSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* PersistenceRegistry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			PersistenceRegistry->UnregisterSystem(this);
			PersistenceRegistry->UnregisterSystem(Archive);
		}
	}

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("SoulMusic"));
#endif

	Super::Deinitialize();
}

bool UGenesisSoulMusicSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisSoulMusicSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisSoulMusicWorldState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	RebuildIndices();
	return true;
}

void UGenesisSoulMusicSubsystem::ResetState()
{
	State = FGenesisSoulMusicWorldState();
	RebuildIndices();
}

void UGenesisSoulMusicSubsystem::RebuildIndices()
{
	MotifIndex.Reset();
	SoundtrackIndex.Reset();
	FragmentIndex.Reset();
	for (int32 Index = 0; Index < State.CharacterMotifs.Num(); ++Index)
	{
		MotifIndex.Add(State.CharacterMotifs[Index].EntityId, Index);
	}
	for (int32 Index = 0; Index < State.Soundtracks.Num(); ++Index)
	{
		SoundtrackIndex.Add(State.Soundtracks[Index].EntityId, Index);
	}
	for (int32 Index = 0; Index < State.Fragments.Num(); ++Index)
	{
		FragmentIndex.Add(State.Fragments[Index].FragmentId, Index);
	}
}

const FGenesisSoulSeed* UGenesisSoulMusicSubsystem::GetPlayerSoul() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisSoulSubsystem* Souls = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulSubsystem>() : nullptr;
	return Souls && Souls->HasPlayerSoul() ? &Souls->GetPlayerSoul() : nullptr;
}

bool UGenesisSoulMusicSubsystem::IsPlayerIncarnation(const FGuid& EntityId) const
{
	const FGenesisSoulSeed* Soul = GetPlayerSoul();
	return Soul && EntityId.IsValid() && Soul->Incarnations.Num() > 0 && Soul->Incarnations.Last().EntityId == EntityId;
}

FGenesisSoulMotif UGenesisSoulMusicSubsystem::GetLeitmotif(const FGuid& EntityId) const
{
	if (IsPlayerIncarnation(EntityId))
	{
		return GetPlayerSoul()->Motif;
	}
	if (const FGenesisCharacterMotif* Existing = FindCharacterMotif(EntityId))
	{
		return Existing->Motif;
	}
	return GenesisSoulLogic::GenerateMotifFromSeed(GenesisHash::Combine(GenesisHash::FromGuid(EntityId), CharacterMotifSalt));
}

const FGenesisCharacterMotif* UGenesisSoulMusicSubsystem::FindCharacterMotif(const FGuid& EntityId) const
{
	const int32* Found = MotifIndex.Find(EntityId);
	return Found ? &State.CharacterMotifs[*Found] : nullptr;
}

const FGenesisCharacterMotif& UGenesisSoulMusicSubsystem::GetOrCreateCharacterMotif(const FGuid& EntityId)
{
	if (const int32* Found = MotifIndex.Find(EntityId))
	{
		return State.CharacterMotifs[*Found];
	}
	FGenesisCharacterMotif& Created = State.CharacterMotifs.AddDefaulted_GetRef();
	Created.EntityId = EntityId;
	Created.Motif = GenesisSoulLogic::GenerateMotifFromSeed(GenesisHash::Combine(GenesisHash::FromGuid(EntityId), CharacterMotifSalt));
	MotifIndex.Add(EntityId, State.CharacterMotifs.Num() - 1);
	return Created;
}

const FGenesisCharacterMotif& UGenesisSoulMusicSubsystem::CreateInheritedMotif(const FGuid& ChildId, const FGuid& ParentA, const FGuid& ParentB)
{
	if (const int32* Found = MotifIndex.Find(ChildId))
	{
		return State.CharacterMotifs[*Found];
	}

	// Kopien: GetOrCreate kann das Array vergrößern
	const FGenesisCharacterMotif MotifA = GetOrCreateCharacterMotif(ParentA);
	const FGenesisCharacterMotif MotifB = GetOrCreateCharacterMotif(ParentB);

	FGenesisCharacterMotif& Child = State.CharacterMotifs.AddDefaulted_GetRef();
	Child.EntityId = ChildId;
	Child.ParentA = ParentA;
	Child.ParentB = ParentB;
	Child.Generation = FMath::Max(MotifA.Generation, MotifB.Generation) + 1;
	Child.Motif = GenesisSoulMusicLogic::InheritMotif(MotifA.Motif, MotifB.Motif, GenesisHash::Combine(GenesisHash::FromGuid(ChildId), InheritanceSalt), Tuning());
	MotifIndex.Add(ChildId, State.CharacterMotifs.Num() - 1);
	return Child;
}

FGenesisLifeSoundtrack& UGenesisSoulMusicSubsystem::GetOrAddSoundtrack(const FGuid& EntityId)
{
	if (const int32* Found = SoundtrackIndex.Find(EntityId))
	{
		return State.Soundtracks[*Found];
	}
	FGenesisLifeSoundtrack& Created = State.Soundtracks.AddDefaulted_GetRef();
	Created.EntityId = EntityId;
	SoundtrackIndex.Add(EntityId, State.Soundtracks.Num() - 1);
	return Created;
}

const FGenesisLifeSoundtrack* UGenesisSoulMusicSubsystem::FindSoundtrack(const FGuid& EntityId) const
{
	const int32* Found = SoundtrackIndex.Find(EntityId);
	return Found ? &State.Soundtracks[*Found] : nullptr;
}

void UGenesisSoulMusicSubsystem::RecordCue(const FGuid& EntityId, EGenesisSoundtrackCue Cue, const FGuid& ReferenceId, float Importance, const FGenesisTimestamp& Time)
{
	FGenesisLifeSoundtrack& Soundtrack = GetOrAddSoundtrack(EntityId);
	FGenesisSoundtrackEntry Entry;
	Entry.Time = Time;
	Entry.Cue = Cue;
	Entry.LifePhase = Soundtrack.CurrentPhase;
	Entry.ReferenceId = ReferenceId;
	Entry.Importance = FMath::Clamp(Importance, 0.0f, 1.0f);
	GenesisSoulMusicLogic::RecordCue(Soundtrack, Entry, Tuning());
}

void UGenesisSoulMusicSubsystem::NotifyLifePhase(const FGuid& EntityId, const FGameplayTag& LifePhase, const FGenesisTimestamp& Time)
{
	FGenesisLifeSoundtrack& Soundtrack = GetOrAddSoundtrack(EntityId);
	if (Soundtrack.CurrentPhase.MatchesTagExact(LifePhase))
	{
		return;
	}
	Soundtrack.CurrentPhase = LifePhase;
	RecordCue(EntityId, EGenesisSoundtrackCue::PhaseChange, FGuid(), 1.0f, Time);
}

FGameplayTag UGenesisSoulMusicSubsystem::GetLifePhase(const FGuid& EntityId) const
{
	const FGenesisLifeSoundtrack* Soundtrack = FindSoundtrack(EntityId);
	return Soundtrack ? Soundtrack->CurrentPhase : FGameplayTag();
}

FGenesisRelationshipTheme* UGenesisSoulMusicSubsystem::FindRelationshipMutable(const FGuid& EntityA, const FGuid& EntityB)
{
	const TPair<FGuid, FGuid> Key = CanonicalPair(EntityA, EntityB);
	return State.Relationships.FindByPredicate([&Key](const FGenesisRelationshipTheme& Theme)
	{
		return Theme.EntityA == Key.Key && Theme.EntityB == Key.Value;
	});
}

const FGenesisRelationshipTheme* UGenesisSoulMusicSubsystem::FindRelationship(const FGuid& EntityA, const FGuid& EntityB) const
{
	return const_cast<UGenesisSoulMusicSubsystem*>(this)->FindRelationshipMutable(EntityA, EntityB);
}

void UGenesisSoulMusicSubsystem::SetRelationshipFusion(const FGuid& EntityA, const FGuid& EntityB, float Fusion, const FGenesisTimestamp& Time)
{
	if (!EntityA.IsValid() || !EntityB.IsValid() || EntityA == EntityB)
	{
		return;
	}

	FGenesisRelationshipTheme* Theme = FindRelationshipMutable(EntityA, EntityB);
	if (!Theme)
	{
		const TPair<FGuid, FGuid> Key = CanonicalPair(EntityA, EntityB);
		Theme = &State.Relationships.AddDefaulted_GetRef();
		Theme->EntityA = Key.Key;
		Theme->EntityB = Key.Value;
	}

	// Eine neue Nähe nach einer Trennung beginnt wieder – die Narbe bleibt im Datensatz
	Theme->bSeparated = false;
	Theme->Fusion = FMath::Clamp(Fusion, 0.0f, 1.0f);

	if (!Theme->bFusionCueRecorded && Theme->Fusion >= Tuning().FusionCueThreshold)
	{
		Theme->bFusionCueRecorded = true;
		const FGuid A = Theme->EntityA;
		const FGuid B = Theme->EntityB;
		const float Importance = Theme->Fusion;
		RecordCue(A, EGenesisSoundtrackCue::RelationshipFusion, B, Importance, Time);
		RecordCue(B, EGenesisSoundtrackCue::RelationshipFusion, A, Importance, Time);
	}
}

void UGenesisSoulMusicSubsystem::SeparateRelationship(const FGuid& EntityA, const FGuid& EntityB, const FGenesisTimestamp& Time)
{
	FGenesisRelationshipTheme* Theme = FindRelationshipMutable(EntityA, EntityB);
	if (!Theme || Theme->bSeparated)
	{
		return;
	}

	Theme->Scar = GenesisSoulMusicLogic::ComputeScar(Theme->Fusion, Tuning());
	Theme->bSeparated = true;

	const FGuid A = Theme->EntityA;
	const FGuid B = Theme->EntityB;
	const float Importance = Theme->Fusion;
	RecordCue(A, EGenesisSoundtrackCue::RelationshipSeparation, B, Importance, Time);
	RecordCue(B, EGenesisSoundtrackCue::RelationshipSeparation, A, Importance, Time);
}

FGenesisSoulMotif UGenesisSoulMusicSubsystem::GetRelationshipMotif(const FGuid& Perspective, const FGuid& Other) const
{
	const FGenesisSoulMotif Own = GetLeitmotif(Perspective);
	const FGenesisRelationshipTheme* Theme = FindRelationship(Perspective, Other);
	if (!Theme)
	{
		return Own;
	}
	return GenesisSoulMusicLogic::FuseMotifs(Own, GetLeitmotif(Other), Theme->bSeparated ? Theme->Scar : Theme->Fusion);
}

const FGenesisMemoryMusicFragment* UGenesisSoulMusicSubsystem::CaptureMemoryFragment(const FGuid& OwnerId, const FGenesisMemoryTrace& Trace, const FGenesisTimestamp& Time)
{
	FGenesisMemoryMusicFragment Fragment;
	if (!GenesisSoulMusicLogic::MakeFragment(GetLeitmotif(OwnerId), OwnerId, Trace, GetLifePhase(OwnerId), Time, Tuning(), Fragment))
	{
		return nullptr;
	}
	if (const int32* Existing = FragmentIndex.Find(Fragment.FragmentId))
	{
		return &State.Fragments[*Existing];
	}

	const FGuid FragmentId = Fragment.FragmentId;
	const float Importance = GenesisSoulMusicLogic::FragmentSignificance(Fragment);
	State.Fragments.Add(MoveTemp(Fragment));

	// Obergrenze je Person: die am wenigsten bedeutsame Erinnerung verliert ihre Musik
	int32 OwnerCount = 0;
	int32 Weakest = INDEX_NONE;
	for (int32 Index = 0; Index < State.Fragments.Num(); ++Index)
	{
		const FGenesisMemoryMusicFragment& Candidate = State.Fragments[Index];
		if (Candidate.OwnerId != OwnerId)
		{
			continue;
		}
		++OwnerCount;
		if (Weakest == INDEX_NONE || GenesisSoulMusicLogic::FragmentSignificance(Candidate) < GenesisSoulMusicLogic::FragmentSignificance(State.Fragments[Weakest]))
		{
			Weakest = Index;
		}
	}
	if (OwnerCount > Tuning().MaxFragmentsPerEntity && Weakest != INDEX_NONE)
	{
		State.Fragments.RemoveAt(Weakest);
	}
	RebuildIndices();

	if (FindFragment(FragmentId))
	{
		RecordCue(OwnerId, EGenesisSoundtrackCue::MemoryFragment, FragmentId, Importance, Time);
	}
	return FindFragment(FragmentId);
}

const FGenesisMemoryMusicFragment* UGenesisSoulMusicSubsystem::FindFragment(const FGuid& FragmentId) const
{
	const int32* Found = FragmentIndex.Find(FragmentId);
	return Found ? &State.Fragments[*Found] : nullptr;
}

FGenesisMusicPhrase UGenesisSoulMusicSubsystem::RecallFragment(const FGuid& FragmentId, float Accuracy) const
{
	const FGenesisMemoryMusicFragment* Fragment = FindFragment(FragmentId);
	if (!Fragment)
	{
		return FGenesisMusicPhrase();
	}
	// Gleiche Genauigkeit → gleiche Erinnerung (reproduzierbar); veränderte Genauigkeit → andere Fehler
	const uint64 Seed = GenesisHash::Combine(GenesisHash::Combine(GenesisHash::FromGuid(FragmentId), RecallSalt), static_cast<uint64>(FMath::RoundToInt(Accuracy * 1000.0f)));
	return GenesisSoulMusicLogic::RecallFragment(*Fragment, Accuracy, Seed, Tuning());
}

FGenesisMusicPhrase UGenesisSoulMusicSubsystem::RenderLeitmotif(const FGuid& EntityId) const
{
	return GenesisSoulMusicLogic::RenderPhrase(GetLeitmotif(EntityId), GenesisSoulMusicLogic::ResolveArrangement(GetLifePhase(EntityId), Tuning()));
}

FGenesisDeathComposition UGenesisSoulMusicSubsystem::ComposeDeath(const FGuid& EntityId) const
{
	FGameplayTag FirstPhase;
	if (const FGenesisLifeSoundtrack* Soundtrack = FindSoundtrack(EntityId))
	{
		const FGenesisSoundtrackEntry* FirstPhaseChange = Soundtrack->Entries.FindByPredicate([](const FGenesisSoundtrackEntry& Entry)
		{
			return Entry.Cue == EGenesisSoundtrackCue::PhaseChange;
		});
		FirstPhase = FirstPhaseChange ? FirstPhaseChange->LifePhase : FGameplayTag();
	}

	TArray<const FGenesisMemoryMusicFragment*> Fragments;
	for (const FGenesisMemoryMusicFragment& Fragment : State.Fragments)
	{
		if (Fragment.OwnerId == EntityId)
		{
			Fragments.Add(&Fragment);
		}
	}

	const FGenesisRelationshipTheme* StrongestBond = nullptr;
	for (const FGenesisRelationshipTheme& Theme : State.Relationships)
	{
		if ((Theme.EntityA == EntityId || Theme.EntityB == EntityId) && (!StrongestBond || Theme.Fusion > StrongestBond->Fusion))
		{
			StrongestBond = &Theme;
		}
	}

	FGenesisSoulMotif BondMotif;
	FGuid PartnerId;
	if (StrongestBond && StrongestBond->Fusion > 0.0f)
	{
		PartnerId = StrongestBond->EntityA == EntityId ? StrongestBond->EntityB : StrongestBond->EntityA;
		// Zum Abschied erklingt die Bindung in voller Verschmelzung – auch wenn sie im Leben getrennt wurde
		BondMotif = GenesisSoulMusicLogic::FuseMotifs(GetLeitmotif(EntityId), GetLeitmotif(PartnerId), StrongestBond->Fusion);
	}

	return GenesisSoulMusicLogic::ComposeDeathPiece(EntityId, GetLeitmotif(EntityId), FirstPhase, Fragments, PartnerId.IsValid() ? &BondMotif : nullptr, PartnerId, Tuning());
}

bool UGenesisSoulMusicSubsystem::ArchiveLife(const FGuid& EntityId, const FGenesisTimestamp& Time)
{
	if (!FindSoundtrack(EntityId))
	{
		return false;
	}

	const FGenesisDeathComposition Composition = ComposeDeath(EntityId);
	RecordCue(EntityId, EGenesisSoundtrackCue::DeathComposition, FGuid(), 1.0f, Time);

	if (IsPlayerIncarnation(EntityId) && Archive)
	{
		FGenesisArchivedLifeMusic Life;
		Life.EntityId = EntityId;
		Life.IncarnationIndex = GetPlayerSoul()->GetCurrentIncarnationIndex();
		Life.Soundtrack = *FindSoundtrack(EntityId);
		Life.SoulMotifAtDeath = GetLeitmotif(EntityId);
		for (const FGenesisDeathSection& Section : Composition.Sections)
		{
			if (const FGenesisMemoryMusicFragment* Fragment = Section.Cue == EGenesisSoundtrackCue::MemoryFragment ? FindFragment(Section.ReferenceId) : nullptr)
			{
				Life.KeyFragments.Add(*Fragment);
			}
		}
		Archive->AddLife(MoveTemp(Life));
	}

	State.Soundtracks.RemoveAll([&EntityId](const FGenesisLifeSoundtrack& Soundtrack) { return Soundtrack.EntityId == EntityId; });
	State.Fragments.RemoveAll([&EntityId](const FGenesisMemoryMusicFragment& Fragment) { return Fragment.OwnerId == EntityId; });
	RebuildIndices();
	return true;
}

#if !UE_BUILD_SHIPPING
void UGenesisSoulMusicSubsystem::RunDeveloperScenario()
{
	const double ScenarioStart = FPlatformTime::Seconds();
	ScenarioReport.Reset();

	UGameInstance* GameInstance = GetGameInstance();
	UGenesisSoulSubsystem* Souls = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulSubsystem>() : nullptr;
	if (!Souls)
	{
		return;
	}
	if (!Souls->HasPlayerSoul())
	{
		Souls->CreatePlayerSoul(0x5EED5EEDull);
	}

	FGuid PlayerId = FGuid(0x9E0E515, 1, 0, 0);
	if (Souls->GetPlayerSoul().IsIncarnated())
	{
		PlayerId = Souls->GetPlayerSoul().Incarnations.Last().EntityId;
	}
	else
	{
		FGenesisIncarnationRecord Template;
		Template.EntityId = PlayerId;
		Souls->BeginIncarnation(Template);
	}

	const FGuid MotherId(0x9E0E515, 2, 0, 0);
	const FGuid FatherId(0x9E0E515, 3, 0, 0);
	const FGuid PartnerId(0x9E0E515, 4, 0, 0);

	// Wiederholbar: Musikdaten dieses Lebens aus früheren Läufen entfernen
	State.Soundtracks.RemoveAll([&PlayerId](const FGenesisLifeSoundtrack& Soundtrack) { return Soundtrack.EntityId == PlayerId; });
	State.Fragments.RemoveAll([&PlayerId](const FGenesisMemoryMusicFragment& Fragment) { return Fragment.OwnerId == PlayerId; });
	State.Relationships.RemoveAll([&PlayerId](const FGenesisRelationshipTheme& Theme) { return Theme.EntityA == PlayerId || Theme.EntityB == PlayerId; });
	RebuildIndices();
	const FGenesisTimestamp Conception = FGenesisTimestamp::FromCalendar(2000);
	auto AtAge = [&Conception](double Years) { return Conception + FGenesisTimestamp::DaysToSeconds(280.0) + FGenesisTimestamp::YearsToSeconds(Years); };
	auto PhaseName = [](const FGameplayTag& Tag)
	{
		FString Name = Tag.GetTagName().ToString();
		int32 Dot;
		return Name.FindLastChar(TEXT('.'), Dot) ? Name.Mid(Dot + 1) : Name;
	};
	auto InstrumentNames = [](const FGenesisPhaseArrangement& Arrangement)
	{
		TArray<FString> Names;
		for (const FGenesisInstrumentLayer& InstrumentLayer : Arrangement.Layers)
		{
			Names.Add(StaticEnum<EGenesisInstrument>()->GetNameStringByValue(static_cast<int64>(InstrumentLayer.Instrument)));
		}
		return FString::Join(Names, TEXT("+"));
	};
	auto FirstLayerNotes = [](const FGenesisMusicPhrase& Phrase)
	{
		TArray<FString> Names;
		if (Phrase.Notes.Num() > 0)
		{
			const EGenesisInstrument Lead = Phrase.Notes[0].Instrument;
			for (const FGenesisMusicNote& Note : Phrase.Notes)
			{
				if (Note.Instrument == Lead)
				{
					Names.Add(GenesisSoulMusicLogic::MidiNoteName(Note.MidiPitch));
				}
			}
		}
		return FString::Join(Names, TEXT(" "));
	};

	// Seelenmotiv durch alle Lebensphasen
	const FGenesisSoulMotif SoulMotif = Souls->GetPlayerSoul().Motif;
	ScenarioReport.Add(FString::Printf(TEXT("Seelenmotiv: %d Noten, Modus %d, Variation %d (Identitätskern: erste %d Intervalle)"),
		SoulMotif.Durations.Num(), SoulMotif.ModeIndex, SoulMotif.Variation, FGenesisSoulMotif::IdentityCoreLength));
	const FGameplayTag Phases[] = {
		GenesisTags::LifePhase_Conception, GenesisTags::LifePhase_Childhood, GenesisTags::LifePhase_Youth, GenesisTags::LifePhase_Adulthood,
		GenesisTags::LifePhase_Elder, GenesisTags::LifePhase_Death, GenesisTags::LifePhase_Afterlife, GenesisTags::LifePhase_Rebirth };
	for (const FGameplayTag& Phase : Phases)
	{
		const FGenesisPhaseArrangement Arrangement = GenesisSoulMusicLogic::ResolveArrangement(Phase, Tuning());
		const FGenesisMusicPhrase Phrase = GenesisSoulMusicLogic::RenderPhrase(SoulMotif, Arrangement);
		ScenarioReport.Add(FString::Printf(TEXT("  %-12s %-26s %3.0f bpm  Präsenz %.2f  %.1f s  | %s"),
			*PhaseName(Phase), *InstrumentNames(Arrangement), Phrase.TempoBpm, Phrase.Presence, Phrase.GetLengthSeconds(), *FirstLayerNotes(Phrase)));
	}

	// Familienmotiv (Abstammung) – getrennt vom Seelenmotiv
	const FGenesisSoulMotif Family = CreateInheritedMotif(PlayerId, MotherId, FatherId).Motif;
	ScenarioReport.Add(FString::Printf(TEXT("Familienmotiv ~ Mutter %.2f | ~ Vater %.2f | ~ Seelenmotiv %.2f"),
		GenesisSoulMusicLogic::MotifSimilarity(Family, GetLeitmotif(MotherId)), GenesisSoulMusicLogic::MotifSimilarity(Family, GetLeitmotif(FatherId)),
		GenesisSoulMusicLogic::MotifSimilarity(Family, SoulMotif)));

	// Ein Leben
	const struct FScenarioMemory
	{
		double Age;
		const TCHAR* Label;
		float Intensity;
		float Valence;
	} Memories[] = {
		{ 5.0, TEXT("Meer zum ersten Mal"), 0.9f, 0.8f },
		{ 8.0, TEXT("Beiläufiger Schultag"), 0.35f, 0.1f },
		{ 15.0, TEXT("Verrat durch Freund"), 0.75f, -0.7f },
		{ 28.0, TEXT("Hochzeit"), 0.95f, 0.9f },
		{ 40.0, TEXT("Kind geboren"), 0.8f, 0.85f },
		{ 74.0, TEXT("Tod des Partners"), 0.92f, -0.9f },
	};

	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Conception, Conception);
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Embryo, Conception + FGenesisTimestamp::DaysToSeconds(56.0));
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Birth, AtAge(0.0));
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Childhood, AtAge(1.0));

	int32 MemoryIndex = 0;
	TArray<FString> FragmentLines;
	TArray<FGuid> FragmentIds;
	auto Remember = [&](const FScenarioMemory& Memory)
	{
		FGenesisMemoryTrace Trace;
		Trace.TraceId = FGuid(0x9E0E515, 100, MemoryIndex, 0);
		Trace.EventId = FGuid(0x9E0E515, 200, MemoryIndex, 0);
		Trace.Intensity = Memory.Intensity;
		Trace.Valence = Memory.Valence;
		Trace.Arousal = FMath::Abs(Memory.Valence) * 0.8f;
		++MemoryIndex;
		const FGenesisMemoryMusicFragment* Fragment = CaptureMemoryFragment(PlayerId, Trace, AtAge(Memory.Age));
		if (!Fragment)
		{
			FragmentLines.Add(FString::Printf(TEXT("  %2.0f J. %-22s Intensität %.2f → keine Musik (unter Schwelle)"), Memory.Age, Memory.Label, Memory.Intensity));
			return;
		}
		FragmentIds.Add(Fragment->FragmentId);
		const FGenesisPhaseArrangement Arrangement = GenesisSoulMusicLogic::ResolveArrangement(Fragment->LifePhase, Tuning());
		FragmentLines.Add(FString::Printf(TEXT("  %2.0f J. %-22s %-10s Modus %d  %s | %s"), Memory.Age, Memory.Label, *PhaseName(Fragment->LifePhase),
			Fragment->Snapshot.ModeIndex, *InstrumentNames(Arrangement), *FirstLayerNotes(GenesisSoulMusicLogic::RenderPhrase(Fragment->Snapshot, Arrangement))));
	};

	Remember(Memories[0]);
	Remember(Memories[1]);
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Youth, AtAge(13.0));
	Remember(Memories[2]);
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Adulthood, AtAge(20.0));
	SetRelationshipFusion(PlayerId, PartnerId, 0.4f, AtAge(25.0));
	Remember(Memories[3]);
	SetRelationshipFusion(PlayerId, PartnerId, 0.9f, AtAge(30.0));

	const FGenesisSoulMotif Fused = GetRelationshipMotif(PlayerId, PartnerId);
	const FGenesisSoulMotif FusedFromPartner = GetRelationshipMotif(PartnerId, PlayerId);
	ScenarioReport.Add(FString::Printf(TEXT("Bindung Fusion 0.90: ~ eigenes %.2f | ~ Partner %.2f | eigenes ~ Partner %.2f | Perspektiven gleich %s"),
		GenesisSoulMusicLogic::MotifSimilarity(Fused, SoulMotif), GenesisSoulMusicLogic::MotifSimilarity(Fused, GetLeitmotif(PartnerId)),
		GenesisSoulMusicLogic::MotifSimilarity(SoulMotif, GetLeitmotif(PartnerId)),
		GenesisSoulMusicLogic::MotifSimilarity(Fused, FusedFromPartner) > 0.9f ? TEXT("annähernd ja") : TEXT("nein")));

	Remember(Memories[4]);
	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Elder, AtAge(65.0));
	Remember(Memories[5]);
	SeparateRelationship(PlayerId, PartnerId, AtAge(74.0));
	const FGenesisSoulMotif Scarred = GetRelationshipMotif(PlayerId, PartnerId);
	ScenarioReport.Add(FString::Printf(TEXT("Nach dem Tod des Partners: Narbe %.2f, ~ eigenes %.2f (vorher %.2f)"),
		FindRelationship(PlayerId, PartnerId)->Scar, GenesisSoulMusicLogic::MotifSimilarity(Scarred, SoulMotif), GenesisSoulMusicLogic::MotifSimilarity(Fused, SoulMotif)));

	ScenarioReport.Add(TEXT("Erinnerungsfragmente:"));
	ScenarioReport.Append(FragmentLines);

	if (FragmentIds.Num() > 0)
	{
		ScenarioReport.Add(FString::Printf(TEXT("Erinnern \"Meer\": genau %s | ungenau (0.3) %s"),
			*FirstLayerNotes(RecallFragment(FragmentIds[0], 1.0f)), *FirstLayerNotes(RecallFragment(FragmentIds[0], 0.3f))));
	}

	NotifyLifePhase(PlayerId, GenesisTags::LifePhase_Death, AtAge(82.0));
	const double ComposeStart = FPlatformTime::Seconds();
	const FGenesisDeathComposition Composition = ComposeDeath(PlayerId);
	const double ComposeMs = (FPlatformTime::Seconds() - ComposeStart) * 1000.0;

	ScenarioReport.Add(FString::Printf(TEXT("Todeskomposition: %d Abschnitte, %.0f s"), Composition.Sections.Num(), Composition.TotalSeconds));
	for (const FGenesisDeathSection& Section : Composition.Sections)
	{
		const FGenesisPhaseArrangement Arrangement = GenesisSoulMusicLogic::ResolveArrangement(Section.LifePhase, Tuning());
		ScenarioReport.Add(FString::Printf(TEXT("  %5.1f s  %-22s %-10s %-26s %s"), Section.StartSeconds,
			*StaticEnum<EGenesisSoundtrackCue>()->GetNameStringByValue(static_cast<int64>(Section.Cue)), *PhaseName(Section.LifePhase),
			*InstrumentNames(Arrangement), *FirstLayerNotes(Section.Phrase)));
	}

	const FGenesisLifeSoundtrack* Soundtrack = FindSoundtrack(PlayerId);
	ScenarioReport.Add(FString::Printf(TEXT("Soundtrack-Momente %d | Szenario %.2f ms | Komposition %.3f ms"),
		Soundtrack ? Soundtrack->Entries.Num() : 0, (FPlatformTime::Seconds() - ScenarioStart) * 1000.0, ComposeMs));

	UE_LOG(LogGenesis, Log, TEXT("SoulMusic: Entwickler-Szenario abgeschlossen (%d Fragmente, %d Abschnitte)."), FragmentIds.Num(), Composition.Sections.Num());
}
#endif

void UGenesisSoulMusicSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisSoulMusicSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("SoulMusic"),
		TEXT("Soul Music"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisSoulMusicSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			OutLines.Add(FString::Printf(TEXT("Charaktermotive %d | Beziehungsthemen %d | Fragmente %d | Soundtracks %d | archivierte Leben %d"),
				Self->State.CharacterMotifs.Num(), Self->State.Relationships.Num(), Self->State.Fragments.Num(), Self->State.Soundtracks.Num(),
				Self->Archive ? Self->Archive->GetState().Lives.Num() : 0));
			OutLines.Append(Self->ScenarioReport);
		}
	});
#endif
}
