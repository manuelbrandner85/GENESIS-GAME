// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisVoiceTypes.h"

/**
 * Wie eine Stimme entsteht – zustandslos und ohne Welt, damit sie im Test messbar ist.
 *
 * Zwei Dinge machen eine Stimme aus, und beide wachsen mit dem Menschen:
 * die **Stimmlippen** (sie bestimmen die Grundfrequenz) und das **Ansatzrohr** zwischen Kehlkopf
 * und Lippen (es bestimmt die Formanten, also die Klangfarbe). Beim Stimmwechsel wachsen die
 * Stimmlippen der Jungen um etwa 60 %, der Kehlkopf sinkt tiefer – deshalb fällt die Stimme
 * um rund eine Oktave, während sie bei Mädchen nur wenige Halbtöne sinkt.
 */
namespace GenesisVoiceLogic
{
	/** Die Stimme, die dieser Körper in diesem Moment hat. */
	GENESISVOICE_API FGenesisVoiceProfile BuildProfile(const FGenesisVoiceInputs& Inputs, const FGenesisVoiceTuning& Tuning);

	/** Sprechgrundfrequenz für Alter und Geschlecht, ohne Tagesform und ohne individuelle Streuung. */
	GENESISVOICE_API float GetBaseF0Hz(EGenesisBiologicalSex Sex, float AgeYears, const FGenesisVoiceTuning& Tuning);

	/** Länge des Ansatzrohrs (cm) für Alter, Geschlecht und Körpergröße. */
	GENESISVOICE_API float GetVocalTractCm(EGenesisBiologicalSex Sex, float AgeYears, float HeightCm, const FGenesisVoiceTuning& Tuning);

	/** Kann ein Mensch in diesem Alter diesen Laut überhaupt machen? Ein Neugeborenes lacht nicht. */
	GENESISVOICE_API bool CanMake(EGenesisUtterance Utterance, float AgeYears);

	/** Natürliche Dauer eines Lauts (s), bevor Intensität und Tempo sie verschieben. */
	GENESISVOICE_API float GetNaturalDurationSeconds(EGenesisUtterance Utterance);

	/** Formanten eines Vokals beim erwachsenen Mann (Hz). Alles andere ist eine Skalierung davon. */
	GENESISVOICE_API void GetVowelFormants(EGenesisVowel Vowel, float OutFormantsHz[4]);

	GENESISVOICE_API FString GetUtteranceName(EGenesisUtterance Utterance);
	GENESISVOICE_API FString GetVowelName(EGenesisVowel Vowel);

	/** Kurzbeschreibung einer Stimme für HUD und Log ("heller Kinderklang, 268 Hz"). */
	GENESISVOICE_API FString DescribeVoice(const FGenesisVoiceProfile& Profile);
}
