// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisSoulMusicTypes.h"

/**
 * Seelenmusik als Klang: Motiv + Lebensphase → Phrase → Schallwellen.
 *
 * Dieselbe Melodie klingt in jeder Lebensphase anders, weil die Instrumentierung wechselt –
 * Spieldose in der Kindheit, Chor im Alter. Das ist der Kern von Soul Music und hier zu hören.
 */
namespace GenesisMusicExport
{
	GENESISSOUND_API FGameplayTag ParseLifePhase(const FString& Name);

	/** Motiv aus einem Seed, Instrumentierung aus der Lebensphase, daraus die spielbare Phrase. */
	GENESISSOUND_API FGenesisMusicPhrase BuildPhrase(uint64 SoulSeed, const FGameplayTag& LifePhase);

	/**
	 * Rendert eine Phrase in einen Puffer (Mono).
	 * bLoop entscheidet, ob die Phrase nach ihrem Ende wieder von vorn beginnt – für eine Hörprobe ja,
	 * für eine Messung des Ausklangs nein. Implizit zu wiederholen hat schon einmal eine Messung verfälscht.
	 */
	GENESISSOUND_API void RenderPhraseAudio(const FGenesisMusicPhrase& Phrase, float SampleRate, float Seconds,
		TArray<float>& OutSamples, bool bLoop = false);
}
