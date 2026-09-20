// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisRandom.h"
#include "GenesisSoulMusicTypes.h"

/**
 * Eine klingende Note. Jede Note der Phrase wird zu einer Stimme, die ihren eigenen Verlauf hat –
 * Anschlag, Ausklingen, Vibrato. Sie endet nicht abrupt, sondern hallt aus wie ein echtes Instrument.
 */
struct FGenesisMusicVoice
{
	bool bActive = false;
	float Frequency = 440.0f;
	float Velocity = 0.7f;
	float StartSeconds = 0.0f;
	float HoldSeconds = 0.5f;
	float AgeSeconds = 0.0f;
	bool bReleased = false;
	float ReleaseLevel = 0.0f;
	EGenesisInstrument Instrument = EGenesisInstrument::Piano;

	float PartialPhases[8] = { 0.0f };
	float VibratoPhase = 0.0f;
	float Envelope = 0.0f;

	/** Gezupfte Saite (Karplus-Strong): ein Rauschimpuls, der im Kreis läuft und dabei weich wird. */
	TArray<float> StringBuffer;
	int32 StringIndex = 0;
	float StringLast = 0.0f;
};

/**
 * Der Klangkörper der Seelenmusik.
 *
 * Soul Music (GENESIS-009) rechnet aus, **welche** Töne ein Mensch gerade hat – seine Intervalle,
 * seine Tonart, seine Instrumentierung je Lebensphase. Dieser Synthesizer macht daraus Schall:
 * acht Instrumente aus Obertönen, Hüllkurven und (bei der Gitarre) einer schwingenden Saite,
 * dazu ein Nachhall, dessen Größe aus der Phrase selbst kommt.
 *
 * Frei von Unreal-Objekten, damit er im Test ohne Welt und ohne Audiogerät läuft.
 */
struct GENESISSOUND_API FGenesisMusicSynth
{
	explicit FGenesisMusicSynth(uint64 Seed = 0x4D555331ull);

	void Initialize(float InSampleRate);

	/** Legt eine Phrase auf. Läuft die alte noch, wird sie ausgeblendet. */
	void SetPhrase(const FGenesisMusicPhrase& Phrase, bool bInLoop = false);

	void Render(float* OutAudio, int32 Frames);

	/** true, wenn alle Töne verklungen sind. */
	bool IsFinished() const;

	/** Gesamtlautstärke (Lebensphasen mischen sich sonst gegenseitig weg). */
	float MasterGain = 0.55f;

	int32 GetActiveVoiceCount() const;

private:
	void StartDueNotes();
	float RenderVoice(FGenesisMusicVoice& Voice);
	float RenderReverb(float Input);

	FGenesisMusicPhrase CurrentPhrase;
	TArray<FGenesisMusicVoice> Voices;
	TArray<bool> NoteStarted;
	FGenesisRandomStream Rng;

	float SampleRate = 48000.0f;
	float PlayheadSeconds = 0.0f;
	bool bLoop = false;

	/** Nachhall nach Schroeder: vier Kammfilter parallel, zwei Allpässe in Reihe. */
	TArray<float> CombBuffers[4];
	int32 CombIndices[4] = { 0, 0, 0, 0 };
	float CombFeedback[4] = { 0.805f, 0.827f, 0.783f, 0.764f };
	TArray<float> AllpassBuffers[2];
	int32 AllpassIndices[2] = { 0, 0 };
};
