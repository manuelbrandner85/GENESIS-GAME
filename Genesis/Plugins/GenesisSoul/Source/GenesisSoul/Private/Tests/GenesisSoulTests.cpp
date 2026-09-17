// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisGameplayTags.h"
#include "GenesisPersistence.h"
#include "GenesisSoulGameplayTags.h"
#include "GenesisSoulLogic.h"
#include "GenesisSoulSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSoulTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGenesisLifeClosure MakeClosure(int64 DeathYear)
	{
		FGenesisLifeClosure Closure;
		Closure.DeathTime = FGenesisTimestamp::FromCalendar(DeathYear);
		Closure.Warmth = 0.6f;
		return Closure;
	}

	FGenesisLifeClosureTheme MakeTheme(const FGameplayTag& Tag, float Intensity, bool bResolved)
	{
		FGenesisLifeClosureTheme Theme;
		Theme.Theme = Tag;
		Theme.Intensity = Intensity;
		Theme.bResolved = bResolved;
		return Theme;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulCreationTest, "Genesis.Soul.CreationIsDeterministic", GenesisSoulTests::Flags)
bool FGenesisSoulCreationTest::RunTest(const FString& Parameters)
{
	const TArray<FGameplayTag> Pool = { GenesisSoulTags::Pattern_Affinity, GenesisSoulTags::Pattern_Fear, GenesisSoulTags::Pattern_Melody };

	const FGenesisSoulSeed A = GenesisSoulLogic::CreateSoulSeed(777, Pool);
	const FGenesisSoulSeed B = GenesisSoulLogic::CreateSoulSeed(777, Pool);
	const FGenesisSoulSeed C = GenesisSoulLogic::CreateSoulSeed(778, Pool);

	TestTrue(TEXT("Seele gültig"), A.IsValid());
	TestEqual(TEXT("Gleicher Seed → gleiche SoulId"), A.SoulId, B.SoulId);
	TestNotEqual(TEXT("Anderer Seed → andere SoulId"), A.SoulId, C.SoulId);
	TestEqual(TEXT("Gleiches Motiv"), A.Motif.Intervals, B.Motif.Intervals);
	TestTrue(TEXT("Motiv hat 3–6 Intervalle"), A.Motif.Intervals.Num() >= 3 && A.Motif.Intervals.Num() <= 6);
	TestEqual(TEXT("Dauer pro Note"), A.Motif.Durations.Num(), A.Motif.Intervals.Num() + 1);
	TestFalse(TEXT("Keine Null-Intervalle"), A.Motif.Intervals.Contains(0));
	TestTrue(TEXT("1–2 angeborene Resonanzen"), A.Resonances.Num() >= 1 && A.Resonances.Num() <= 2);
	TestEqual(TEXT("Noch keine Inkarnation"), A.Incarnations.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulIncarnationCycleTest, "Genesis.Soul.Reincarnation.Cycle", GenesisSoulTests::Flags)
bool FGenesisSoulIncarnationCycleTest::RunTest(const FString& Parameters)
{
	FGenesisSoulCarryOverParams Params;
	FGenesisSoulSeed Soul = GenesisSoulLogic::CreateSoulSeed(1, {});
	const FGuid OriginalSoulId = Soul.SoulId;

	FGenesisIncarnationRecord Template;
	Template.EntityId = FGuid(1, 2, 3, 4);
	Template.GenomeId = FGuid(5, 6, 7, 8);

	TestFalse(TEXT("Abschluss ohne Inkarnation schlägt fehl"), GenesisSoulLogic::CloseIncarnation(Soul, GenesisSoulTests::MakeClosure(80), Params));
	TestNotNull(TEXT("Erste Inkarnation beginnt"), GenesisSoulLogic::BeginIncarnation(Soul, Template));
	TestNull(TEXT("Doppelte Inkarnation abgelehnt"), GenesisSoulLogic::BeginIncarnation(Soul, Template));
	TestTrue(TEXT("Leben 1 abgeschlossen"), GenesisSoulLogic::CloseIncarnation(Soul, GenesisSoulTests::MakeClosure(80), Params));

	// Neuer Körper, neue DNA – dieselbe Seele
	Template.EntityId = FGuid(11, 12, 13, 14);
	Template.GenomeId = FGuid(15, 16, 17, 18);
	const FGenesisIncarnationRecord* Second = GenesisSoulLogic::BeginIncarnation(Soul, Template);
	if (!TestNotNull(TEXT("Zweite Inkarnation beginnt"), Second))
	{
		return false;
	}
	TestEqual(TEXT("Index 1"), Second->Index, 1);
	TestEqual(TEXT("SoulId bleibt"), Soul.SoulId, OriginalSoulId);
	TestNotEqual(TEXT("Genom wechselt"), Soul.Incarnations[0].GenomeId, Soul.Incarnations[1].GenomeId);
	TestTrue(TEXT("Erstes Leben abgeschlossen"), Soul.Incarnations[0].bCompleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulCarryOverTest, "Genesis.Soul.CarryOver.ResonancesAndEchoes", GenesisSoulTests::Flags)
bool FGenesisSoulCarryOverTest::RunTest(const FString& Parameters)
{
	FGenesisSoulCarryOverParams Params;
	FGenesisSoulSeed Soul = GenesisSoulLogic::CreateSoulSeed(42, {});
	const TArray<int32> IdentityCore = { Soul.Motif.Intervals[0], Soul.Motif.Intervals[1] };

	// Leben 1: Meer erlebt (Resonanz), jemanden verlassen (offen), Vergebung (abgeschlossen)
	GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
	FGenesisLifeClosure Life1 = GenesisSoulTests::MakeClosure(70);
	Life1.ExperiencedPatterns.Emplace(GenesisSoulTags::Pattern_Affinity, 1.0f);
	Life1.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Abandonment, 0.8f, false));
	Life1.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Forgiveness, 0.8f, true));
	GenesisSoulLogic::CloseIncarnation(Soul, Life1, Params);

	const float AffinityAfterLife1 = GenesisSoulLogic::GetResonanceIntensity(Soul, GenesisSoulTags::Pattern_Affinity);
	TestTrue(TEXT("Resonanz entstanden"), AffinityAfterLife1 > 0.3f);

	// Leben 2: nichts davon erlebt
	GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
	GenesisSoulLogic::CloseIncarnation(Soul, GenesisSoulTests::MakeClosure(140), Params);

	const float AffinityAfterLife2 = GenesisSoulLogic::GetResonanceIntensity(Soul, GenesisSoulTags::Pattern_Affinity);
	TestTrue(TEXT("Resonanz verblasst"), FMath::IsNearlyEqual(AffinityAfterLife2, AffinityAfterLife1 * Params.ResonancePersistence, 1.0e-4f));

	const FGenesisSoulEcho* Open = GenesisSoulLogic::FindEcho(Soul, GenesisTags::Theme_Abandonment);
	const FGenesisSoulEcho* Integrated = GenesisSoulLogic::FindEcho(Soul, GenesisTags::Theme_Forgiveness);
	if (!TestNotNull(TEXT("Offenes Echo bleibt"), Open) || !TestNotNull(TEXT("Integriertes Echo bleibt"), Integrated))
	{
		return false;
	}
	TestTrue(TEXT("Offene Themen verblassen langsamer"), Open->Weight > Integrated->Weight);

	// Leben 3: Thema kehrt wieder und wird diesmal abgeschlossen
	GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
	FGenesisLifeClosure Life3 = GenesisSoulTests::MakeClosure(210);
	Life3.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Abandonment, 0.6f, true));
	GenesisSoulLogic::CloseIncarnation(Soul, Life3, Params);

	const FGenesisSoulEcho* Returned = GenesisSoulLogic::FindEcho(Soul, GenesisTags::Theme_Abandonment);
	if (!TestNotNull(TEXT("Echo vorhanden"), Returned))
	{
		return false;
	}
	TestEqual(TEXT("Wiederkehr gezählt"), Returned->ManifestationCount, 1);
	TestTrue(TEXT("Jetzt integriert"), Returned->State == EGenesisEchoState::Integrated);

	// Viele Leben später: Identitätskern des Motivs unverändert
	for (int32 Life = 0; Life < 10; ++Life)
	{
		GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
		GenesisSoulLogic::CloseIncarnation(Soul, GenesisSoulTests::MakeClosure(300 + Life * 70), Params);
	}
	TestEqual(TEXT("Motiv-Kern Intervall 0"), Soul.Motif.Intervals[0], IdentityCore[0]);
	TestEqual(TEXT("Motiv-Kern Intervall 1"), Soul.Motif.Intervals[1], IdentityCore[1]);
	TestEqual(TEXT("Motiv-Variationen"), Soul.Motif.Variation, 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulEchoPullTest, "Genesis.Soul.EchoPullAndRecognition", GenesisSoulTests::Flags)
bool FGenesisSoulEchoPullTest::RunTest(const FString& Parameters)
{
	FGenesisSoulCarryOverParams Params;
	FGenesisSoulSeed Soul = GenesisSoulLogic::CreateSoulSeed(9, {});
	const FGuid Beloved(100, 200, 300, 400);
	const FGuid Stranger(1, 1, 1, 1);

	GenesisSoulLogic::BeginIncarnation(Soul, FGenesisIncarnationRecord());
	FGenesisLifeClosure Closure = GenesisSoulTests::MakeClosure(60);
	Closure.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Betrayal, 0.7f, false));
	Closure.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Care, 0.7f, true));
	FGenesisBondExperience Bond;
	Bond.OtherSoulId = Beloved;
	Bond.Intensity = 1.0f;
	Bond.Themes.AddTag(GenesisTags::Theme_Love);
	Closure.Bonds.Add(Bond);
	GenesisSoulLogic::CloseIncarnation(Soul, Closure, Params);

	const float OpenPull = GenesisSoulLogic::ComputeEchoPull(Soul, FGameplayTagContainer(GenesisTags::Theme_Betrayal));
	const float IntegratedPull = GenesisSoulLogic::ComputeEchoPull(Soul, FGameplayTagContainer(GenesisTags::Theme_Care));
	const float NoPull = GenesisSoulLogic::ComputeEchoPull(Soul, FGameplayTagContainer(GenesisTags::Theme_Power));
	TestTrue(TEXT("Offenes Thema zieht stärker"), OpenPull > IntegratedPull);
	TestTrue(TEXT("Integriertes Thema zieht"), IntegratedPull > 0.0f);
	TestEqual(TEXT("Fremdes Thema zieht nicht"), NoPull, 0.0f);

	const float WithTheme = GenesisSoulLogic::ComputeRecognition(Soul, Beloved, FGameplayTagContainer(GenesisTags::Theme_Love));
	const float WithoutTheme = GenesisSoulLogic::ComputeRecognition(Soul, Beloved, FGameplayTagContainer(GenesisTags::Theme_Power));
	TestTrue(TEXT("Wiedererkennung im passenden Kontext stärker"), WithTheme > WithoutTheme);
	TestTrue(TEXT("Wiedererkennung vorhanden"), WithoutTheme > 0.0f);
	TestEqual(TEXT("Fremde Seele unbekannt"), GenesisSoulLogic::ComputeRecognition(Soul, Stranger, FGameplayTagContainer()), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSoulPersistenceTest, "Genesis.Soul.Persistence", GenesisSoulTests::Flags)
bool FGenesisSoulPersistenceTest::RunTest(const FString& Parameters)
{
	FGenesisSoulCarryOverParams Params;
	FGenesisSoulArchive Archive;
	Archive.PlayerSoul = GenesisSoulLogic::CreateSoulSeed(2026, { GenesisSoulTags::Pattern_Melody });
	GenesisSoulLogic::BeginIncarnation(Archive.PlayerSoul, FGenesisIncarnationRecord());
	FGenesisLifeClosure Closure = GenesisSoulTests::MakeClosure(90);
	Closure.CourtThemes.Add(GenesisSoulTests::MakeTheme(GenesisTags::Theme_Loss, 0.9f, false));
	GenesisSoulLogic::CloseIncarnation(Archive.PlayerSoul, Closure, Params);
	Archive.CompanionSouls.Add(GenesisSoulLogic::CreateSoulSeed(5, {}));

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben"), GenesisPersistence::Write(Archive, Bytes));

	FGenesisSoulArchive Loaded;
	TestTrue(TEXT("Lesen"), GenesisPersistence::Read(Loaded, Bytes));
	TestEqual(TEXT("SoulId"), Loaded.PlayerSoul.SoulId, Archive.PlayerSoul.SoulId);
	TestEqual(TEXT("Inkarnationen"), Loaded.PlayerSoul.Incarnations.Num(), 1);
	TestEqual(TEXT("Echos"), Loaded.PlayerSoul.Echoes.Num(), Archive.PlayerSoul.Echoes.Num());
	TestTrue(TEXT("Echo-Thema"), Loaded.PlayerSoul.Echoes.Num() > 0 && Loaded.PlayerSoul.Echoes[0].Theme == GenesisTags::Theme_Loss);
	TestEqual(TEXT("Motiv"), Loaded.PlayerSoul.Motif.Intervals, Archive.PlayerSoul.Motif.Intervals);
	TestEqual(TEXT("Begleiterseelen"), Loaded.CompanionSouls.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
