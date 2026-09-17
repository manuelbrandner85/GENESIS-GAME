// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisSoulMusicTypes.h"

struct FGenesisMemoryTrace;

/**
 * Zustandslose, deterministische Logik der Seelenmusik.
 * Motive (FGenesisSoulMotif: Intervalle in Halbtönen, Dauern in Sechzehnteln, Modus) werden hier gespielt, vererbt,
 * verschmolzen, getrennt und erinnert. Keine Klangerzeugung – das Ergebnis sind Phrasen aus Noten.
 */
namespace GenesisSoulMusicLogic
{
	/** Eingebaute Instrumentierung je Lebensphase (unbekannte Phasen: Erwachsenenleben). */
	GENESISSOULMUSIC_API FGenesisPhaseArrangement GetDefaultArrangement(const FGameplayTag& LifePhase);

	/** Überschreibung aus den Einstellungen, sonst eingebaute Instrumentierung. */
	GENESISSOULMUSIC_API FGenesisPhaseArrangement ResolveArrangement(const FGameplayTag& LifePhase, const FGenesisSoulMusicTuning& Tuning);

	/** Rastet eine MIDI-Note auf den nächstliegenden Ton des Modus (0 = Ionisch … 6 = Lokrisch) ein; bei Gleichstand nach unten. */
	GENESISSOULMUSIC_API int32 QuantizeToMode(int32 MidiPitch, int32 TonicMidiNote, int32 ModeIndex);

	/** Absolute Tonhöhen des Motivs ab der Tonika, im Modus eingerastet. */
	GENESISSOULMUSIC_API TArray<int32> ComputePitches(const FGenesisSoulMotif& Motif, int32 TonicMidiNote);

	/** Spielt ein Motiv in einer Phasen-Instrumentierung. Ausgelassene Noten verlängern die vorherige (Phrasenlänge bleibt). */
	GENESISSOULMUSIC_API FGenesisMusicPhrase RenderPhrase(const FGenesisSoulMotif& Motif, const FGenesisPhaseArrangement& Arrangement);

	/** Wiedererkennbarkeit zweier Motive 0..1 (Kontur und Intervalle 60 %, Rhythmus 30 %, Modus 10 %). Symmetrisch. */
	GENESISSOULMUSIC_API float MotifSimilarity(const FGenesisSoulMotif& A, const FGenesisSoulMotif& B);

	/** Familienmotiv eines Kindes: Melodie-Abschnitte beider Eltern, Rhythmus eines Elternteils, leichte Mutation. */
	GENESISSOULMUSIC_API FGenesisSoulMotif InheritMotif(const FGenesisSoulMotif& ParentA, const FGenesisSoulMotif& ParentB, uint64 Seed, const FGenesisSoulMusicTuning& Tuning);

	/**
	 * Beziehungsmotiv aus Sicht von Own. Fusion 0 = das eigene Motiv, 1 = beide Motive treffen sich in der Mitte
	 * (aus beiden Perspektiven identisch – zwei Stimmen sind eine geworden).
	 */
	GENESISSOULMUSIC_API FGenesisSoulMotif FuseMotifs(const FGenesisSoulMotif& Own, const FGenesisSoulMotif& Other, float Fusion);

	/** Wie viel einer Verschmelzung nach einer Trennung bleibt (0..1). */
	GENESISSOULMUSIC_API float ComputeScar(float FusionAtSeparation, const FGenesisSoulMusicTuning& Tuning);

	/** Trennung (Bruch, Tod): Das eigene Motiv kehrt zurück, ein Rest des anderen bleibt als Narbe. */
	GENESISSOULMUSIC_API FGenesisSoulMotif SeparateMotif(const FGenesisSoulMotif& Own, const FGenesisSoulMotif& Other, float FusionAtSeparation, const FGenesisSoulMusicTuning& Tuning, float& OutScar);

	/**
	 * Hält eine intensive Erinnerung musikalisch fest. Schlägt fehl unter der Intensitätsschwelle.
	 * Sehr intensive Momente enthalten den Identitätskern; starke Valenz färbt den Modus.
	 */
	GENESISSOULMUSIC_API bool MakeFragment(const FGenesisSoulMotif& SourceMotif, const FGuid& OwnerId, const FGenesisMemoryTrace& Trace, const FGameplayTag& LifePhase,
		const FGenesisTimestamp& Time, const FGenesisSoulMusicTuning& Tuning, FGenesisMemoryMusicFragment& OutFragment);

	/**
	 * Erinnern: Das Fragment erklingt in der Instrumentierung seiner Lebensphase.
	 * Geringe Genauigkeit erzeugt falsche Töne, Lücken, Zeitversatz und mehr Raum – der erste Ton bleibt immer.
	 */
	GENESISSOULMUSIC_API FGenesisMusicPhrase RecallFragment(const FGenesisMemoryMusicFragment& Fragment, float Accuracy, uint64 RecallSeed, const FGenesisSoulMusicTuning& Tuning);

	/** Fügt einen Soundtrack-Moment chronologisch ein; bei Platzmangel fällt der unwichtigste (Phasenwechsel und Tod bleiben). */
	GENESISSOULMUSIC_API void RecordCue(FGenesisLifeSoundtrack& Soundtrack, const FGenesisSoundtrackEntry& Entry, const FGenesisSoulMusicTuning& Tuning);

	/** Gewicht eines Fragments für die Todeskomposition. */
	GENESISSOULMUSIC_API float FragmentSignificance(const FGenesisMemoryMusicFragment& Fragment);

	/**
	 * Todeskomposition: Seelenmotiv wie am Anfang (Spieluhr) → prägende Erinnerungen chronologisch in ihren Phasenklängen →
	 * wichtigste Bindung → Seelenmotiv im Orchester → Übergang in Chor und kosmische Flächen.
	 */
	GENESISSOULMUSIC_API FGenesisDeathComposition ComposeDeathPiece(const FGuid& EntityId, const FGenesisSoulMotif& SoulMotif, const FGameplayTag& FirstPhase,
		const TArray<const FGenesisMemoryMusicFragment*>& Fragments, const FGenesisSoulMotif* BondMotif, const FGuid& BondPartnerId, const FGenesisSoulMusicTuning& Tuning);

	/** Notenname für Debug-Ausgaben (z. B. "C#5"). */
	GENESISSOULMUSIC_API FString MidiNoteName(int32 MidiPitch);
}
