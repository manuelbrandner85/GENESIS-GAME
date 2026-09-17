// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisGameplayTags.h"
#include "GenesisMemoryTrace.h"
#include "GenesisPersistence.h"
#include "GenesisSoulLogic.h"
#include "GenesisSoulMusicLogic.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSoulMusicTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGenesisSoulMotif MakeMotif(const TArray<int32>& Intervals, const TArray<int32>& Durations, int32 Mode, float Warmth = 0.5f)
	{
		FGenesisSoulMotif Motif;
		Motif.Intervals = Intervals;
		Motif.Durations = Durations;
		Motif.ModeIndex = Mode;
		Motif.Warmth = Warmth;
		return Motif;
	}

	bool HasInstrument(const FGenesisPhaseArrangement& Arrangement, EGenesisInstrument Instrument)
	{
		return Arrangement.Layers.ContainsByPredicate([Instrument](const FGenesisInstrumentLayer& Layer) { return Layer.Instrument == Instrument; });
	}

	/** Tonhöhen der ersten Instrumentenschicht in Spielreihenfolge. */
	TArray<int32> LeadPitches(const FGenesisMusicPhrase& Phrase)
	{
		TArray<int32> Pitches;
		if (Phrase.Notes.Num() > 0)
		{
			for (const FGenesisMusicNote& Note : Phrase.Notes)
			{
				if (Note.Instrument == Phrase.Notes[0].Instrument)
				{
					Pitches.Add(Note.MidiPitch);
				}
			}
		}
		return Pitches;
	}

	FGenesisMemoryTrace MakeTrace(int32 Id, float Intensity, float Valence)
	{
		FGenesisMemoryTrace Trace;
		Trace.TraceId = FGuid(0x7E57, Id, 0, 0);
		Trace.EventId = FGuid(0x7E57, Id, 1, 0);
		Trace.Intensity = Intensity;
		Trace.Valence = Valence;
		Trace.Arousal = 0.5f;
		return Trace;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicArrangementTest, "Genesis.Music.Soul.PhaseArrangements", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicArrangementTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;
	using EI = EGenesisInstrument;

	const FGenesisPhaseArrangement Conception = GetDefaultArrangement(GenesisTags::LifePhase_Conception);
	const FGenesisPhaseArrangement Childhood = GetDefaultArrangement(GenesisTags::LifePhase_Childhood);
	const FGenesisPhaseArrangement Youth = GetDefaultArrangement(GenesisTags::LifePhase_Youth);
	const FGenesisPhaseArrangement Adulthood = GetDefaultArrangement(GenesisTags::LifePhase_Adulthood);
	const FGenesisPhaseArrangement Elder = GetDefaultArrangement(GenesisTags::LifePhase_Elder);
	const FGenesisPhaseArrangement Death = GetDefaultArrangement(GenesisTags::LifePhase_Death);
	const FGenesisPhaseArrangement Afterlife = GetDefaultArrangement(GenesisTags::LifePhase_Afterlife);
	const FGenesisPhaseArrangement Rebirth = GetDefaultArrangement(GenesisTags::LifePhase_Rebirth);

	// Instrumentierung entlang des Lebens
	TestTrue(TEXT("Zeugung: Spieluhr"), HasInstrument(Conception, EI::MusicBox));
	TestTrue(TEXT("Kindheit: Spieluhr + Klavier"), HasInstrument(Childhood, EI::MusicBox) && HasInstrument(Childhood, EI::Piano));
	TestTrue(TEXT("Jugend: Klavier + Gitarre"), HasInstrument(Youth, EI::Piano) && HasInstrument(Youth, EI::Guitar));
	TestTrue(TEXT("Erwachsen: Streicher"), HasInstrument(Adulthood, EI::Strings));
	TestTrue(TEXT("Alter: nur Klavier, reduziert"), Elder.Layers.Num() == 1 && HasInstrument(Elder, EI::Piano) && Elder.NoteDensity < 1.0f);
	TestTrue(TEXT("Tod: Orchester"), HasInstrument(Death, EI::Orchestra));
	TestTrue(TEXT("Jenseits: Chor + kosmisch"), HasInstrument(Afterlife, EI::Choir) && HasInstrument(Afterlife, EI::CosmicPad));
	TestTrue(TEXT("Wiedergeburt: Spieluhr, kaum hörbar"), HasInstrument(Rebirth, EI::MusicBox) && Rebirth.Presence < Conception.Presence && Rebirth.Presence <= 0.05f);
	TestTrue(TEXT("Tod präsenter als Alter"), Death.Presence > Elder.Presence);

	// Jede Phase hat eine Instrumentierung
	const FGameplayTag AllPhases[] = {
		GenesisTags::LifePhase_Conception, GenesisTags::LifePhase_Embryo, GenesisTags::LifePhase_Birth, GenesisTags::LifePhase_Childhood,
		GenesisTags::LifePhase_Youth, GenesisTags::LifePhase_Adulthood, GenesisTags::LifePhase_Elder, GenesisTags::LifePhase_Death,
		GenesisTags::LifePhase_Ghost, GenesisTags::LifePhase_Afterlife, GenesisTags::LifePhase_Rebirth, GenesisTags::LifePhase_CosmicConsciousness,
		GenesisTags::LifePhase_Creation };
	const FGenesisSoulMotif SoulMotif = GenesisSoulLogic::GenerateMotifFromSeed(42);
	for (const FGameplayTag& Phase : AllPhases)
	{
		const FGenesisPhaseArrangement Arrangement = GetDefaultArrangement(Phase);
		const FGenesisMusicPhrase Phrase = RenderPhrase(SoulMotif, Arrangement);
		TestTrue(FString::Printf(TEXT("%s spielt Noten"), *Phase.ToString()), Arrangement.Layers.Num() > 0 && Phrase.Notes.Num() > 0);
	}

	// Reduziertes Alter: weniger Noten, gleiche Phrasenlänge, Identitätskern und Schlussnote bleiben
	FGenesisSoulMotif Long = MakeMotif({ 2, -1, 3, -2, 1, 2 }, { 4, 2, 2, 4, 4, 2, 8 }, 0);
	FGenesisPhaseArrangement ElderFull = Elder;
	ElderFull.NoteDensity = 1.0f;
	const FGenesisMusicPhrase Reduced = RenderPhrase(Long, Elder);
	const FGenesisMusicPhrase Full = RenderPhrase(Long, ElderFull);
	TestTrue(TEXT("Alter spielt weniger Noten"), Reduced.Notes.Num() < Full.Notes.Num());
	TestEqual(TEXT("Phrasenlänge bleibt"), Reduced.LengthBeats, Full.LengthBeats);
	TestEqual(TEXT("Länge = Summe der Dauern"), Full.LengthBeats, 26.0f / 4.0f);
	const TArray<int32> AllPitches = ComputePitches(Long, Elder.TonicMidiNote);
	const TArray<int32> ReducedPitches = LeadPitches(Reduced);
	if (TestTrue(TEXT("Kern + Schluss vorhanden"), ReducedPitches.Num() >= 4))
	{
		for (int32 Index = 0; Index <= FGenesisSoulMotif::IdentityCoreLength; ++Index)
		{
			TestEqual(TEXT("Identitätskern unverändert"), ReducedPitches[Index], AllPitches[Index]);
		}
		TestEqual(TEXT("Schlussnote bleibt"), ReducedPitches.Last(), AllPitches.Last());
	}

	// Oktavlage: Spieluhr eine Oktave über der Kindheits-Tonika
	const FGenesisMusicPhrase ChildPhrase = RenderPhrase(SoulMotif, Childhood);
	TestEqual(TEXT("Spieluhr beginnt eine Oktave über der Tonika"), ChildPhrase.Notes[0].MidiPitch, Childhood.TonicMidiNote + 12);

	// Modus-Raster (Gleichstand → tiefer)
	TestEqual(TEXT("C# in C-Ionisch → C"), QuantizeToMode(61, 60, 0), 60);
	TestEqual(TEXT("F# in C-Ionisch → F"), QuantizeToMode(66, 60, 0), 65);
	TestEqual(TEXT("E in C-Äolisch → Es"), QuantizeToMode(64, 60, 5), 63);
	TestEqual(TEXT("H in C-Äolisch → B (Oktavgrenze)"), QuantizeToMode(59, 60, 5), 58);
	TestEqual(TEXT("Leitton bleibt in C-Ionisch"), QuantizeToMode(71, 60, 0), 71);
	TestEqual(TEXT("Tonika tiefer Oktave"), QuantizeToMode(48, 60, 3), 48);

	// Kontur bleibt beim Einrasten erhalten (auch Halbtonschritte in allen Modi)
	bool bContourKept = true;
	for (uint64 Seed = 0; Seed < 200; ++Seed)
	{
		FGenesisSoulMotif Motif = GenesisSoulLogic::GenerateMotifFromSeed(Seed);
		Motif.ModeIndex = static_cast<int32>(Seed % 7);
		const TArray<int32> Pitches = ComputePitches(Motif, 60);
		for (int32 Index = 1; Index < Pitches.Num(); ++Index)
		{
			bContourKept &= FMath::Sign(Pitches[Index] - Pitches[Index - 1]) == FMath::Sign(Motif.Intervals[Index - 1]);
			bContourKept &= QuantizeToMode(Pitches[Index], 60, Motif.ModeIndex) == Pitches[Index];
		}
	}
	TestTrue(TEXT("Kontur und Modus-Töne über 200 Motive in allen Modi"), bContourKept);
	TestTrue(TEXT("Halbton aufwärts in C-Ionisch wird nicht zur Tonwiederholung"), ComputePitches(MakeMotif({ 1 }, { 4, 4 }, 0), 60) == TArray<int32>({ 60, 62 }));
	TestTrue(TEXT("Halbton abwärts in C-Phrygisch"), ComputePitches(MakeMotif({ -1 }, { 4, 4 }, 2), 60) == TArray<int32>({ 60, 58 }));

	// Überschreibung aus den Einstellungen
	FGenesisSoulMusicTuning Tuning;
	FGenesisPhaseArrangement Custom = Childhood;
	Custom.Layers = { FGenesisInstrumentLayer() };
	Custom.Layers[0].Instrument = EI::Guitar;
	Tuning.ArrangementOverrides.Add(Custom);
	TestTrue(TEXT("Überschreibung greift"), HasInstrument(ResolveArrangement(GenesisTags::LifePhase_Childhood, Tuning), EI::Guitar));
	TestTrue(TEXT("Andere Phasen unverändert"), HasInstrument(ResolveArrangement(GenesisTags::LifePhase_Youth, Tuning), EI::Piano));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicInheritanceTest, "Genesis.Music.Soul.InheritanceAndSimilarity", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicInheritanceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;

	const FGenesisSoulMotif A = GenesisSoulLogic::GenerateMotifFromSeed(7);
	const FGenesisSoulMotif B = GenesisSoulLogic::GenerateMotifFromSeed(8);
	TestEqual(TEXT("Motiv gleicht sich selbst"), MotifSimilarity(A, A), 1.0f);
	TestEqual(TEXT("Ähnlichkeit symmetrisch"), MotifSimilarity(A, B), MotifSimilarity(B, A));

	FGenesisSoulMusicTuning Tuning;
	const int32 Families = 300;
	double ChildToParent = 0.0;
	double ChildToStranger = 0.0;
	double GrandchildToGrandparent = 0.0;
	bool bIntervalsValid = true;
	for (int32 Family = 0; Family < Families; ++Family)
	{
		const FGenesisSoulMotif Mother = GenesisSoulLogic::GenerateMotifFromSeed(1000 + Family);
		const FGenesisSoulMotif Father = GenesisSoulLogic::GenerateMotifFromSeed(5000 + Family);
		const FGenesisSoulMotif Stranger = GenesisSoulLogic::GenerateMotifFromSeed(9000 + Family);
		const FGenesisSoulMotif InLaw = GenesisSoulLogic::GenerateMotifFromSeed(13000 + Family);

		const FGenesisSoulMotif Child = InheritMotif(Mother, Father, 20000 + Family, Tuning);
		const FGenesisSoulMotif Grandchild = InheritMotif(Child, InLaw, 30000 + Family, Tuning);

		ChildToParent += 0.5 * (MotifSimilarity(Child, Mother) + MotifSimilarity(Child, Father));
		ChildToStranger += MotifSimilarity(Child, Stranger);
		GrandchildToGrandparent += 0.5 * (MotifSimilarity(Grandchild, Mother) + MotifSimilarity(Grandchild, Father));

		for (int32 Interval : Child.Intervals)
		{
			bIntervalsValid &= Interval != 0 && FMath::Abs(Interval) <= 7;
		}
		bIntervalsValid &= Child.Durations.Num() == Child.Intervals.Num() + 1;
	}
	ChildToParent /= Families;
	ChildToStranger /= Families;
	GrandchildToGrandparent /= Families;
	AddInfo(FString::Printf(TEXT("Ähnlichkeit Kind~Eltern %.2f, Kind~Fremde %.2f, Enkel~Großeltern %.2f"), ChildToParent, ChildToStranger, GrandchildToGrandparent));

	TestTrue(TEXT("Kinder klingen nach ihren Eltern"), ChildToParent > ChildToStranger + 0.1);
	TestTrue(TEXT("Über Generationen verblasst die Ähnlichkeit"), GrandchildToGrandparent < ChildToParent);
	TestTrue(TEXT("Enkel erinnern noch an die Großeltern"), GrandchildToGrandparent > ChildToStranger);
	TestTrue(TEXT("Intervalle gültig, Dauern passend"), bIntervalsValid);

	const FGenesisSoulMotif First = InheritMotif(A, B, 99, Tuning);
	const FGenesisSoulMotif Second = InheritMotif(A, B, 99, Tuning);
	TestTrue(TEXT("Vererbung deterministisch"), First.Intervals == Second.Intervals && First.Durations == Second.Durations && First.ModeIndex == Second.ModeIndex);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicFusionTest, "Genesis.Music.Soul.FusionAndSeparation", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicFusionTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;

	const FGenesisSoulMotif Own = MakeMotif({ 2, 2, -1, 3 }, { 4, 4, 4, 4, 8 }, 0, 0.5f);
	const FGenesisSoulMotif Other = MakeMotif({ -5, 7, -6, 5, -2 }, { 8, 2, 2, 8, 4, 4 }, 5, 0.5f);
	FGenesisSoulMusicTuning Tuning;

	const FGenesisSoulMotif Untouched = FuseMotifs(Own, Other, 0.0f);
	TestTrue(TEXT("Fusion 0 = eigenes Motiv"), Untouched.Intervals == Own.Intervals && Untouched.Durations == Own.Durations);

	const FGenesisSoulMotif FromOwn = FuseMotifs(Own, Other, 1.0f);
	const FGenesisSoulMotif FromOther = FuseMotifs(Other, Own, 1.0f);
	TestTrue(TEXT("Volle Verschmelzung klingt aus beiden Perspektiven gleich"),
		FromOwn.Intervals == FromOther.Intervals && FromOwn.Durations == FromOther.Durations && FromOwn.ModeIndex == FromOther.ModeIndex);

	const float Before = MotifSimilarity(Own, Other);
	const FGenesisSoulMotif Half = FuseMotifs(Own, Other, 0.5f);
	TestTrue(TEXT("Näher am anderen mit wachsender Fusion"), MotifSimilarity(Half, Other) > Before && MotifSimilarity(FromOwn, Other) >= MotifSimilarity(Half, Other));
	TestEqual(TEXT("Bis zur vollen Fusion bleibt der eigene Modus"), Half.ModeIndex, Own.ModeIndex);

	for (const FGenesisSoulMotif& Motif : { Half, FromOwn })
	{
		TestEqual(TEXT("Dauern passen zu Intervallen"), Motif.Durations.Num(), Motif.Intervals.Num() + 1);
		TestFalse(TEXT("Keine Null-Intervalle"), Motif.Intervals.Contains(0));
	}

	const FGenesisSoulMotif Fused = FuseMotifs(Own, Other, 0.9f);
	float Scar = 0.0f;
	const FGenesisSoulMotif Separated = SeparateMotif(Own, Other, 0.9f, Tuning, Scar);
	TestEqual(TEXT("Narbe = Fusion × Rückhalt"), Scar, 0.9f * Tuning.ScarRetention);
	TestTrue(TEXT("Nach der Trennung wieder näher am eigenen Motiv"), MotifSimilarity(Separated, Own) > MotifSimilarity(Fused, Own));
	TestTrue(TEXT("Aber nicht unverändert – eine Narbe bleibt"), MotifSimilarity(Separated, Own) < 1.0f);
	TestTrue(TEXT("Der andere klingt noch nach"), MotifSimilarity(Separated, Other) > Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicMemoryTest, "Genesis.Music.Soul.MemoryFragmentsAndSoundtrack", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicMemoryTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;

	FGenesisSoulMusicTuning Tuning;
	const FGuid Owner(1, 1, 1, 1);
	const FGenesisTimestamp Time = FGenesisTimestamp::FromCalendar(2010);
	const FGenesisSoulMotif Bright = MakeMotif({ 2, 2, 1, 2, -3 }, { 4, 4, 2, 2, 4, 8 }, 0);
	const FGameplayTag Childhood(GenesisTags::LifePhase_Childhood);

	FGenesisMemoryMusicFragment Fragment;
	TestFalse(TEXT("Schwache Erinnerung bekommt keine Musik"), MakeFragment(Bright, Owner, MakeTrace(1, 0.4f, 0.5f), Childhood, Time, Tuning, Fragment));

	if (TestTrue(TEXT("Intensive Erinnerung bekommt ein Fragment"), MakeFragment(Bright, Owner, MakeTrace(2, 0.9f, 0.8f), Childhood, Time, Tuning, Fragment)))
	{
		TestEqual(TEXT("Sehr intensiv: beginnt beim Identitätskern"), Fragment.SourceNoteIndex, 0);
		TestEqual(TEXT("4 Noten"), Fragment.Snapshot.Durations.Num(), 4);
		TestTrue(TEXT("Intervalle aus dem Motiv"), Fragment.Snapshot.Intervals == TArray<int32>({ 2, 2, 1 }));
		TestEqual(TEXT("Heller Modus bleibt bei positiver Erinnerung"), Fragment.Snapshot.ModeIndex, 0);
		TestTrue(TEXT("Phase festgehalten"), Fragment.LifePhase.MatchesTagExact(Childhood));
	}

	FGenesisMemoryMusicFragment Painful;
	MakeFragment(Bright, Owner, MakeTrace(3, 0.75f, -0.8f), Childhood, Time, Tuning, Painful);
	TestEqual(TEXT("Schmerz färbt hellen Modus äolisch"), Painful.Snapshot.ModeIndex, 5);
	TestEqual(TEXT("Mittlere Intensität: 3 Noten"), Painful.Snapshot.Durations.Num(), 3);

	FGenesisMemoryMusicFragment Tender;
	MakeFragment(MakeMotif({ 1, 2, -1 }, { 4, 4, 4, 4 }, 2), Owner, MakeTrace(4, 0.9f, 0.9f), Childhood, Time, Tuning, Tender);
	TestEqual(TEXT("Zärtlichkeit hellt dunklen Modus auf"), Tender.Snapshot.ModeIndex, 0);

	// Erinnern
	const FGenesisMusicPhrase Original = RenderPhrase(Fragment.Snapshot, ResolveArrangement(Fragment.LifePhase, Tuning));
	const FGenesisMusicPhrase Exact = RecallFragment(Fragment, 1.0f, 123, Tuning);
	TestTrue(TEXT("Genaue Erinnerung klingt wie damals"), LeadPitches(Exact) == LeadPitches(Original) && Exact.Notes.Num() == Original.Notes.Num());

	int32 Changed = 0;
	bool bFirstNoteKept = true;
	bool bNeverEmpty = true;
	for (uint64 Seed = 0; Seed < 50; ++Seed)
	{
		const FGenesisMusicPhrase Blurry = RecallFragment(Fragment, 0.2f, Seed, Tuning);
		bNeverEmpty &= Blurry.Notes.Num() > 0;
		bFirstNoteKept &= Blurry.Notes.Num() > 0 && Blurry.Notes[0].MidiPitch == Original.Notes[0].MidiPitch && Blurry.Notes[0].StartBeat == 0.0f;
		bool bDifferent = Blurry.Notes.Num() != Original.Notes.Num();
		for (int32 Index = 0; !bDifferent && Index < Blurry.Notes.Num(); ++Index)
		{
			bDifferent = Blurry.Notes[Index].MidiPitch != Original.Notes[Index].MidiPitch || Blurry.Notes[Index].StartBeat != Original.Notes[Index].StartBeat;
		}
		Changed += bDifferent ? 1 : 0;
	}
	AddInfo(FString::Printf(TEXT("Ungenaue Erinnerung (0.2) verändert in %d von 50 Fällen"), Changed));
	TestTrue(TEXT("Ungenaue Erinnerungen klingen anders"), Changed >= 25);
	TestTrue(TEXT("Erster Ton bleibt als Anker"), bFirstNoteKept);
	TestTrue(TEXT("Nie ganz verloren"), bNeverEmpty);
	TestTrue(TEXT("Mehr Raum bei ungenauer Erinnerung"), RecallFragment(Fragment, 0.2f, 5, Tuning).Space > Exact.Space);

	const FGenesisMusicPhrase RecallA = RecallFragment(Fragment, 0.4f, 77, Tuning);
	const FGenesisMusicPhrase RecallB = RecallFragment(Fragment, 0.4f, 77, Tuning);
	TestTrue(TEXT("Erinnern reproduzierbar"), LeadPitches(RecallA) == LeadPitches(RecallB));

	// Soundtrack: chronologisch, Verdichtung behält Phasenwechsel
	FGenesisSoulMusicTuning SmallTuning;
	SmallTuning.MaxSoundtrackEntries = 8;
	FGenesisLifeSoundtrack Soundtrack;
	const int32 Order[] = { 5, 1, 9, 3, 11, 7, 0, 10, 2, 8, 4, 6 };
	for (int32 Step : Order)
	{
		FGenesisSoundtrackEntry Entry;
		Entry.Time = FGenesisTimestamp(static_cast<int64>(Step) * 1000);
		Entry.Cue = Step % 4 == 0 ? EGenesisSoundtrackCue::PhaseChange : EGenesisSoundtrackCue::MemoryFragment;
		Entry.Importance = static_cast<float>(Step) / 12.0f;
		RecordCue(Soundtrack, Entry, SmallTuning);
	}
	TestEqual(TEXT("Verdichtet auf Obergrenze"), Soundtrack.Entries.Num(), 8);
	bool bChronological = true;
	int32 PhaseChanges = 0;
	float LowestFragmentImportance = 1.0f;
	for (int32 Index = 0; Index < Soundtrack.Entries.Num(); ++Index)
	{
		bChronological &= Index == 0 || Soundtrack.Entries[Index - 1].Time.Seconds <= Soundtrack.Entries[Index].Time.Seconds;
		PhaseChanges += Soundtrack.Entries[Index].Cue == EGenesisSoundtrackCue::PhaseChange ? 1 : 0;
		if (Soundtrack.Entries[Index].Cue != EGenesisSoundtrackCue::PhaseChange)
		{
			LowestFragmentImportance = FMath::Min(LowestFragmentImportance, Soundtrack.Entries[Index].Importance);
		}
	}
	TestTrue(TEXT("Chronologisch"), bChronological);
	TestEqual(TEXT("Alle Phasenwechsel bleiben (0, 4, 8)"), PhaseChanges, 3);
	TestTrue(TEXT("Die unwichtigsten Momente fielen weg"), LowestFragmentImportance >= 5.0f / 12.0f - UE_KINDA_SMALL_NUMBER);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicDeathTest, "Genesis.Music.Soul.DeathComposition", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicDeathTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;
	using EI = EGenesisInstrument;

	FGenesisSoulMusicTuning Tuning;
	const FGuid Owner(2, 2, 2, 2);
	const FGuid Partner(3, 3, 3, 3);
	const FGenesisSoulMotif Soul = GenesisSoulLogic::GenerateMotifFromSeed(2026);
	const FGenesisTimestamp Birth = FGenesisTimestamp::FromCalendar(1950);

	struct FLifeMoment { double Age; FGameplayTag Phase; float Intensity; float Valence; };
	const FLifeMoment Moments[] = {
		{ 6.0, GenesisTags::LifePhase_Childhood, 0.9f, 0.8f },
		{ 9.0, GenesisTags::LifePhase_Childhood, 0.62f, 0.1f },  // schwach – fällt heraus
		{ 16.0, GenesisTags::LifePhase_Youth, 0.8f, -0.7f },
		{ 30.0, GenesisTags::LifePhase_Adulthood, 0.95f, 0.9f },
		{ 45.0, GenesisTags::LifePhase_Adulthood, 0.61f, 0.0f }, // schwach – fällt heraus
		{ 50.0, GenesisTags::LifePhase_Adulthood, 0.85f, 0.6f },
		{ 76.0, GenesisTags::LifePhase_Elder, 0.9f, -0.9f },
	};

	TArray<FGenesisMemoryMusicFragment> Fragments;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Moments); ++Index)
	{
		FGenesisMemoryMusicFragment& Fragment = Fragments.AddDefaulted_GetRef();
		MakeFragment(Soul, Owner, MakeTrace(100 + Index, Moments[Index].Intensity, Moments[Index].Valence), Moments[Index].Phase,
			Birth + FGenesisTimestamp::YearsToSeconds(Moments[Index].Age), Tuning, Fragment);
	}
	// Zeiger in umgekehrter Reihenfolge – die Komposition ordnet selbst
	TArray<const FGenesisMemoryMusicFragment*> Pointers;
	for (int32 Index = Fragments.Num() - 1; Index >= 0; --Index)
	{
		Pointers.Add(&Fragments[Index]);
	}

	const FGenesisSoulMotif Bond = FuseMotifs(Soul, GenesisSoulLogic::GenerateMotifFromSeed(77), 0.9f);
	const FGenesisDeathComposition Piece = ComposeDeathPiece(Owner, Soul, GenesisTags::LifePhase_Conception, Pointers, &Bond, Partner, Tuning);

	if (!TestEqual(TEXT("Anfang + 5 Erinnerungen + Bindung + Tod + Übergang"), Piece.Sections.Num(), 9))
	{
		return false;
	}
	TestTrue(TEXT("Beginnt mit der Spieluhr"), Piece.Sections[0].Cue == EGenesisSoundtrackCue::SoulMotif && Piece.Sections[0].Phrase.Notes[0].Instrument == EI::MusicBox);

	bool bChronological = true;
	bool bWeakExcluded = true;
	for (int32 Index = 1; Index <= 5; ++Index)
	{
		const FGenesisDeathSection& Section = Piece.Sections[Index];
		TestTrue(TEXT("Erinnerungsabschnitt"), Section.Cue == EGenesisSoundtrackCue::MemoryFragment);
		bWeakExcluded &= Section.ReferenceId != Fragments[1].FragmentId && Section.ReferenceId != Fragments[4].FragmentId;
		if (Index > 1)
		{
			const FGenesisMemoryMusicFragment* Previous = Fragments.FindByPredicate([&](const FGenesisMemoryMusicFragment& F) { return F.FragmentId == Piece.Sections[Index - 1].ReferenceId; });
			const FGenesisMemoryMusicFragment* Current = Fragments.FindByPredicate([&](const FGenesisMemoryMusicFragment& F) { return F.FragmentId == Section.ReferenceId; });
			bChronological &= Previous && Current && Previous->CreatedAt.Seconds < Current->CreatedAt.Seconds;
		}
	}
	TestTrue(TEXT("Erinnerungen in der Reihenfolge des Lebens"), bChronological);
	TestTrue(TEXT("Unbedeutendere Erinnerungen fehlen"), bWeakExcluded);
	TestTrue(TEXT("Kindheitserinnerung klingt nach Spieluhr"), Piece.Sections[1].Phrase.Notes[0].Instrument == EI::MusicBox);

	TestTrue(TEXT("Bindung mit Partner"), Piece.Sections[6].Cue == EGenesisSoundtrackCue::RelationshipFusion && Piece.Sections[6].ReferenceId == Partner);
	TestTrue(TEXT("Tod: Orchester"), Piece.Sections[7].Phrase.Notes.ContainsByPredicate([](const FGenesisMusicNote& Note) { return Note.Instrument == EI::Orchestra; }));
	TestTrue(TEXT("Übergang: Chor"), Piece.Sections[8].Phrase.Notes.ContainsByPredicate([](const FGenesisMusicNote& Note) { return Note.Instrument == EI::Choir; }));

	bool bIncreasing = true;
	for (int32 Index = 1; Index < Piece.Sections.Num(); ++Index)
	{
		bIncreasing &= Piece.Sections[Index].StartSeconds > Piece.Sections[Index - 1].StartSeconds;
	}
	TestTrue(TEXT("Abschnitte folgen nacheinander"), bIncreasing);
	const FGenesisDeathSection& Last = Piece.Sections.Last();
	TestTrue(TEXT("Gesamtdauer stimmt"), FMath::IsNearlyEqual(Piece.TotalSeconds, Last.StartSeconds + Last.Phrase.GetLengthSeconds(), 0.001f));
	AddInfo(FString::Printf(TEXT("Todeskomposition: %d Abschnitte, %.1f s"), Piece.Sections.Num(), Piece.TotalSeconds));

	const FGenesisDeathComposition Lonely = ComposeDeathPiece(Owner, Soul, GenesisTags::LifePhase_Conception, Pointers, nullptr, FGuid(), Tuning);
	TestEqual(TEXT("Ohne Bindung ein Abschnitt weniger"), Lonely.Sections.Num(), 8);

	const FGenesisDeathComposition Again = ComposeDeathPiece(Owner, Soul, GenesisTags::LifePhase_Conception, Pointers, &Bond, Partner, Tuning);
	bool bSame = Again.Sections.Num() == Piece.Sections.Num();
	for (int32 Index = 0; bSame && Index < Piece.Sections.Num(); ++Index)
	{
		bSame = LeadPitches(Again.Sections[Index].Phrase) == LeadPitches(Piece.Sections[Index].Phrase) && Again.Sections[Index].ReferenceId == Piece.Sections[Index].ReferenceId;
	}
	TestTrue(TEXT("Komposition deterministisch"), bSame);

	// Performance: ein volles Leben (64 Fragmente)
	TArray<FGenesisMemoryMusicFragment> ManyFragments;
	for (int32 Index = 0; Index < 64; ++Index)
	{
		FGenesisMemoryMusicFragment& Fragment = ManyFragments.AddDefaulted_GetRef();
		MakeFragment(Soul, Owner, MakeTrace(1000 + Index, 0.6f + 0.4f * static_cast<float>(Index % 7) / 6.0f, (Index % 5) / 2.0f - 1.0f),
			GenesisTags::LifePhase_Adulthood, Birth + static_cast<int64>(Index) * FGenesisTimestamp::SecondsPerYear, Tuning, Fragment);
	}
	TArray<const FGenesisMemoryMusicFragment*> ManyPointers;
	for (const FGenesisMemoryMusicFragment& Fragment : ManyFragments)
	{
		ManyPointers.Add(&Fragment);
	}
	const int32 Runs = 500;
	const double Start = FPlatformTime::Seconds();
	for (int32 Run = 0; Run < Runs; ++Run)
	{
		ComposeDeathPiece(Owner, Soul, GenesisTags::LifePhase_Conception, ManyPointers, &Bond, Partner, Tuning);
	}
	const double AverageMs = (FPlatformTime::Seconds() - Start) * 1000.0 / Runs;
	AddInfo(FString::Printf(TEXT("Performance: %.4f ms pro Todeskomposition (64 Fragmente)"), AverageMs));
	TestTrue(TEXT("Komposition unter 2 ms"), AverageMs < 2.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulMusicPersistenceTest, "Genesis.Music.Soul.PersistenceRoundTrip", GenesisSoulMusicTests::Flags)
bool FGenesisSoulMusicPersistenceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisSoulMusicTests;
	using namespace GenesisSoulMusicLogic;

	FGenesisSoulMusicTuning Tuning;
	FGenesisSoulMusicWorldState World;

	FGenesisCharacterMotif& Character = World.CharacterMotifs.AddDefaulted_GetRef();
	Character.EntityId = FGuid(5, 5, 5, 5);
	Character.ParentA = FGuid(6, 6, 6, 6);
	Character.Generation = 3;
	Character.Motif = GenesisSoulLogic::GenerateMotifFromSeed(55);

	FGenesisRelationshipTheme& Theme = World.Relationships.AddDefaulted_GetRef();
	Theme.EntityA = FGuid(5, 5, 5, 5);
	Theme.EntityB = FGuid(7, 7, 7, 7);
	Theme.Fusion = 0.8f;
	Theme.Scar = 0.28f;
	Theme.bSeparated = true;

	FGenesisMemoryMusicFragment& Fragment = World.Fragments.AddDefaulted_GetRef();
	MakeFragment(Character.Motif, Character.EntityId, MakeTrace(9, 0.9f, -0.6f), GenesisTags::LifePhase_Youth, FGenesisTimestamp::FromCalendar(1990), Tuning, Fragment);

	FGenesisLifeSoundtrack& Soundtrack = World.Soundtracks.AddDefaulted_GetRef();
	Soundtrack.EntityId = Character.EntityId;
	Soundtrack.CurrentPhase = GenesisTags::LifePhase_Youth;
	FGenesisSoundtrackEntry Entry;
	Entry.Cue = EGenesisSoundtrackCue::MemoryFragment;
	Entry.ReferenceId = Fragment.FragmentId;
	Entry.Importance = 0.7f;
	RecordCue(Soundtrack, Entry, Tuning);

	TArray<uint8> Bytes;
	TestTrue(TEXT("Welt schreiben"), GenesisPersistence::Write(World, Bytes));
	FGenesisSoulMusicWorldState Loaded;
	TestTrue(TEXT("Welt lesen"), GenesisPersistence::Read(Loaded, Bytes));

	if (TestEqual(TEXT("Motive"), Loaded.CharacterMotifs.Num(), 1) && TestEqual(TEXT("Fragmente"), Loaded.Fragments.Num(), 1)
		&& TestEqual(TEXT("Beziehungen"), Loaded.Relationships.Num(), 1) && TestEqual(TEXT("Soundtracks"), Loaded.Soundtracks.Num(), 1))
	{
		TestTrue(TEXT("Motiv identisch"), Loaded.CharacterMotifs[0].Motif.Intervals == Character.Motif.Intervals && Loaded.CharacterMotifs[0].Generation == 3);
		TestTrue(TEXT("Narbe erhalten"), Loaded.Relationships[0].bSeparated && FMath::IsNearlyEqual(Loaded.Relationships[0].Scar, 0.28f));
		TestTrue(TEXT("Fragment erhalten"), Loaded.Fragments[0].FragmentId == Fragment.FragmentId && Loaded.Fragments[0].Snapshot.Intervals == Fragment.Snapshot.Intervals
			&& Loaded.Fragments[0].LifePhase.MatchesTagExact(GenesisTags::LifePhase_Youth));
		TestTrue(TEXT("Soundtrack erhalten"), Loaded.Soundtracks[0].Entries.Num() == 1 && Loaded.Soundtracks[0].Entries[0].ReferenceId == Fragment.FragmentId);
		TestTrue(TEXT("Erinnern nach dem Laden gleich"), LeadPitches(RecallFragment(Loaded.Fragments[0], 0.5f, 3, Tuning)) == LeadPitches(RecallFragment(Fragment, 0.5f, 3, Tuning)));
	}

	FGenesisSoulMusicArchiveState Archive;
	FGenesisArchivedLifeMusic& Life = Archive.Lives.AddDefaulted_GetRef();
	Life.EntityId = Character.EntityId;
	Life.IncarnationIndex = 4;
	Life.Soundtrack = Soundtrack;
	Life.KeyFragments.Add(Fragment);
	Life.SoulMotifAtDeath = Character.Motif;
	TArray<uint8> ArchiveBytes;
	FGenesisSoulMusicArchiveState LoadedArchive;
	TestTrue(TEXT("Archiv Rundreise"), GenesisPersistence::Write(Archive, ArchiveBytes) && GenesisPersistence::Read(LoadedArchive, ArchiveBytes));
	TestTrue(TEXT("Archiv erhalten"), LoadedArchive.Lives.Num() == 1 && LoadedArchive.Lives[0].IncarnationIndex == 4 && LoadedArchive.Lives[0].KeyFragments.Num() == 1);
	return true;
}

#endif
