// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisBodySynth.h"

/**
 * Klangproben als Datei – die einzige Art, Ton wirklich zu prüfen.
 *
 * Ein Bild kann man ansehen, einen Klang muss man hören. Deshalb schreibt GENESIS auf Befehl
 * kurze WAV-Dateien aus derselben Synthese, die auch im Spiel läuft.
 */
namespace GenesisSoundExport
{
	/** Welcher Moment des Lebens geklungen werden soll. */
	enum class EPreset : uint8
	{
		/** Im Mutterleib: Herzschlag der Mutter, eigener Herzschlag, tiefes Grundrauschen. */
		Womb,
		/** Unter der Geburt: Wehen als Druckwellen, Sauerstoff fällt und kommt zurück. */
		Labor,
		/** Der Moment der Geburt: der Sprung vom Hören im Wasser zum Hören in der Luft. */
		Birth,
		/** Die ersten Minuten: eigener Atem, eigener Herzschlag, eine Welt ohne Tiefpass. */
		Newborn
	};

	GENESISSOUND_API EPreset ParsePreset(const FString& Name);
	GENESISSOUND_API FString GetPresetName(EPreset Preset);

	/** Rendert die Probe in einen Puffer (Mono). Ohne Welt, ohne Audiogerät – auch im Test nutzbar. */
	GENESISSOUND_API void RenderPreset(EPreset Preset, float SampleRate, float Seconds, TArray<float>& OutSamples);

	/** Schreibt Mono-Abtastwerte als 16-Bit-WAV. Liefert false, wenn die Datei nicht geschrieben werden konnte. */
	GENESISSOUND_API bool WriteWav(const FString& FilePath, const TArray<float>& Samples, int32 SampleRate);
}
