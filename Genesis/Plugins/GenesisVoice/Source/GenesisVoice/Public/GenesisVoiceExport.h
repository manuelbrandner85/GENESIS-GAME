// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisVoiceTypes.h"

/**
 * Hörproben der Stimme.
 *
 * Eine Stimme kann man nicht ansehen. Deshalb schreibt GENESIS auf Befehl WAV-Dateien aus derselben
 * Synthese, die auch im Spiel läuft – für die Prüfung durch Menschen und für die Messung im Test.
 */
namespace GenesisVoiceExport
{
	/** Ein Mensch an einem Punkt seines Lebens. */
	enum class EPreset : uint8
	{
		/** Der erste Schrei. */
		Newborn,
		/** Säugling mit sechs Monaten: Gurren und Lallen. */
		Infant,
		/** Kind mit sechs Jahren. */
		Child,
		/** Erwachsene Frau. */
		Woman,
		/** Erwachsener Mann. */
		Man,
		/** Mensch mit achtzig Jahren. */
		Elder,
		/** Die Mutter über dem Kind: Beruhigen und Summen. */
		Mother,
		/** Erkältet und heiser. */
		Hoarse
	};

	GENESISVOICE_API EPreset ParsePreset(const FString& Name);
	GENESISVOICE_API FString GetPresetName(EPreset Preset);

	/** Das Stimmprofil der Probe – ohne Ton, für Anzeige und Test. */
	GENESISVOICE_API FGenesisVoiceProfile GetPresetProfile(EPreset Preset);

	/** Rendert die Probe (Mono). Ohne Welt und ohne Audiogerät. */
	GENESISVOICE_API void RenderPreset(EPreset Preset, float SampleRate, float Seconds, TArray<float>& OutSamples);

	/** Rendert einen einzelnen Laut einer Stimme – die Grundlage jeder Messung. */
	GENESISVOICE_API void RenderUtterance(const FGenesisVoiceProfile& Profile, const FGenesisUtterance& Utterance,
		float SampleRate, float Seconds, TArray<float>& OutSamples);
}
