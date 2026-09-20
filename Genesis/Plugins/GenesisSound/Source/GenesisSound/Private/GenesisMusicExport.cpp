// GENESIS: Der Kreislauf des Lebens

#include "GenesisMusicExport.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisGameplayTags.h"
#include "GenesisLog.h"
#include "GenesisMusicSynth.h"
#include "GenesisSoulLogic.h"
#include "GenesisSoulMusicLogic.h"
#include "GenesisSoulMusicSubsystem.h"
#include "GenesisSoulMusicTypes.h"
#include "GenesisSoulSubsystem.h"
#include "GenesisSoundExport.h"
#include "Misc/Paths.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

namespace GenesisMusicExport
{
	FGameplayTag ParseLifePhase(const FString& Name)
	{
		if (Name.Equals(TEXT("zeugung"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("conception"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Conception;
		}
		if (Name.Equals(TEXT("embryo"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Embryo;
		}
		if (Name.Equals(TEXT("geburt"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("birth"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Birth;
		}
		if (Name.Equals(TEXT("kindheit"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("childhood"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Childhood;
		}
		if (Name.Equals(TEXT("jugend"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("youth"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Youth;
		}
		if (Name.Equals(TEXT("alter"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("elder"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Elder;
		}
		if (Name.Equals(TEXT("tod"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("death"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_Death;
		}
		if (Name.Equals(TEXT("kosmos"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("cosmic"), ESearchCase::IgnoreCase))
		{
			return GenesisTags::LifePhase_CosmicConsciousness;
		}
		return GenesisTags::LifePhase_Adulthood;
	}

	FGenesisMusicPhrase BuildPhrase(uint64 SoulSeed, const FGameplayTag& LifePhase)
	{
		// Das Motiv gehört der Seele, die Instrumentierung der Lebensphase –
		// dieselbe Melodie klingt als Spieldose anders als im Chor.
		const FGenesisSoulMotif Motif = GenesisSoulLogic::GenerateMotifFromSeed(SoulSeed);
		const FGenesisSoulMusicTuning Tuning;
		const FGenesisPhaseArrangement Arrangement = GenesisSoulMusicLogic::ResolveArrangement(LifePhase, Tuning);
		return GenesisSoulMusicLogic::RenderPhrase(Motif, Arrangement);
	}

	void RenderPhraseAudio(const FGenesisMusicPhrase& Phrase, float SampleRate, float Seconds, TArray<float>& OutSamples, bool bLoop)
	{
		const int32 Rate = FMath::Max(8000, FMath::RoundToInt(SampleRate));
		const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));

		FGenesisMusicSynth Synth;
		Synth.Initialize(static_cast<float>(Rate));
		Synth.SetPhrase(Phrase, bLoop);

		OutSamples.SetNumUninitialized(TotalFrames);
		Synth.Render(OutSamples.GetData(), TotalFrames);
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisMusicRenderCommand(
		TEXT("genesis.Music.RenderWav"),
		TEXT("Schreibt das Seelenmotiv als Klangprobe: <zeugung|embryo|geburt|kindheit|jugend|erwachsen|alter|tod|kosmos> [Sekunden]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const FString PhaseName = Args.Num() > 0 ? Args[0] : TEXT("kindheit");
			const FGameplayTag Phase = GenesisMusicExport::ParseLifePhase(PhaseName);
			const float Seconds = FMath::Clamp(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 20.0f, 2.0f, 180.0f);

			// Gibt es eine Spielerseele, klingt ihr Motiv – sonst ein Motiv aus festem Seed
			uint64 Seed = 0x50BA5E11ull;
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			if (const UGenesisSoulSubsystem* Soul = GameInstance ? GameInstance->GetSubsystem<UGenesisSoulSubsystem>() : nullptr)
			{
				if (Soul->HasPlayerSoul())
				{
					Seed = GenesisHash::FromGuid(Soul->GetPlayerSoul().SoulId);
				}
			}

			const FGenesisMusicPhrase Phrase = GenesisMusicExport::BuildPhrase(Seed, Phase);
			TArray<float> Samples;
			// Hörprobe: Das Motiv wiederholt sich, damit man es mehrfach hört
			GenesisMusicExport::RenderPhraseAudio(Phrase, 48000.0f, Seconds, Samples, true);

			const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audio"),
				FString::Printf(TEXT("GENESIS_Motiv_%s.wav"), *PhaseName));
			if (GenesisSoundExport::WriteWav(Path, Samples, 48000))
			{
				UE_LOG(LogGenesis, Display, TEXT("Musik: %s geschrieben (%d Noten, %.1f BPM, Phrase %.1f s, Probe %.1f s)"),
					*Path, Phrase.Notes.Num(), Phrase.TempoBpm, Phrase.GetLengthSeconds(), Seconds);
			}
		}));
}
#endif
