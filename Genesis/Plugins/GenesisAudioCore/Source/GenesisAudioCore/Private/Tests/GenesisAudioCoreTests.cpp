// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisAudioCoreLogic.h"
#include "GenesisBodyGameplayTags.h"
#include "GenesisBodyLogic.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisAudioCoreTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGenesisTimestamp Conception = FGenesisTimestamp::FromCalendar(2000, 30, 10);

	FGenesisTimestamp Week(double Weeks)
	{
		return Conception + FGenesisTimestamp::DaysToSeconds(Weeks * 7.0);
	}

	FGenesisBodyState Fetus(double Weeks, const FGenesisBodyTuning& BodyTuning)
	{
		FGenesisBodyState Body = GenesisBodyLogic::CreateAtConception(FGuid(8, 0, 0, 1), FGuid(), FGenesisBodyGenetics(), FGenesisConceptionVitality(), Conception, EGenesisSimulationLevel::Full);
		GenesisBodyLogic::AdvanceDays(Body, Week(Weeks), Weeks * 7.0, 0.0f, BodyTuning);
		GenesisBodyLogic::AdvanceHours(Body, Week(Weeks), 1.0, 0.0f);
		return Body;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisAudioWombHearingTest, "Genesis.Audio.Core.WombAndBirthHearing", GenesisAudioCoreTests::Flags)
bool FGenesisAudioWombHearingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisAudioCoreTests;
	FGenesisBodyTuning BodyTuning;
	FGenesisHearingTuning Tuning;

	// Woche 10: Gehör noch nicht ausgebildet – nur Vibrationen des mütterlichen Körpers
	const FGenesisBodyState Early = Fetus(10.0, BodyTuning);
	const FGenesisHearingPerception EarlyHearing = GenesisAudioCoreLogic::ComputeHearing(&Early, Week(10.0), Tuning);
	TestTrue(TEXT("Im Mutterleib"), EarlyHearing.bInWomb);
	TestTrue(FString::Printf(TEXT("Außenwelt kaum hörbar (%.2f)"), EarlyHearing.ExternalAudibility), EarlyHearing.ExternalAudibility < 0.1f);
	TestTrue(TEXT("Nur tiefste Frequenzen"), EarlyHearing.LowPassCutoffHz <= Tuning.WombCutoffMinHz + 1.0f);

	// Woche 30: Gehör ausgebildet – Stimmen gedämpft hörbar, Körper der Mutter dominiert
	FGenesisBodyState Late = Fetus(30.0, BodyTuning);
	const FGenesisHearingPerception LateHearing = GenesisAudioCoreLogic::ComputeHearing(&Late, Week(30.0), Tuning);
	TestTrue(TEXT("Außenwelt gedämpft hörbar"), LateHearing.ExternalAudibility > 0.4f && LateHearing.ExternalAudibility < 0.6f);
	TestTrue(TEXT("Tiefpass wie durch Gewebe"), FMath::IsNearlyEqual(LateHearing.LowPassCutoffHz, Tuning.WombCutoffMaxHz, 1.0f));
	TestTrue(TEXT("Körperklänge dominieren"), LateHearing.BodyAudibility > LateHearing.ExternalAudibility);

	const FGenesisBodyAudioParams WombAudio = GenesisAudioCoreLogic::ComputeBodyAudio(&Late, Week(30.0), LateHearing);
	TestTrue(TEXT("Mütterlicher Herzschlag"), WombAudio.bMaternalSounds && WombAudio.MaternalHeartRateBpm > 60.0f);
	TestTrue(FString::Printf(TEXT("Eigenes Herz schlägt schneller als das der Mutter (%.0f bpm)"), WombAudio.HeartRateBpm), WombAudio.HeartRateBpm > WombAudio.MaternalHeartRateBpm);
	TestEqual(TEXT("Vor der Geburt keine Atmung"), WombAudio.BreathRate, 0.0f);

	// Geburt: Frequenzsprung
	GenesisBodyLogic::AdvanceDays(Late, Week(38.0), 8.0 * 7.0, 0.0f, BodyTuning);
	GenesisBodyLogic::Birth(Late, Week(38.0), BodyTuning);
	const FGenesisHearingPerception Newborn = GenesisAudioCoreLogic::ComputeHearing(&Late, Week(38.0), Tuning);
	TestFalse(TEXT("Nach der Geburt: Luft"), Newborn.bInWomb);
	TestTrue(FString::Printf(TEXT("Frequenzsprung (%.0f Hz → %.0f Hz)"), LateHearing.LowPassCutoffHz, Newborn.LowPassCutoffHz), Newborn.LowPassCutoffHz > LateHearing.LowPassCutoffHz * 10.0f);
	TestTrue(TEXT("Neugeborenes hört noch nicht voll"), Newborn.LowPassCutoffHz < 20000.0f);
	TestTrue(TEXT("Geburtsschock: Körper sehr präsent"), Newborn.FocusNarrowing > 0.3f);

	const FGenesisBodyAudioParams FirstBreath = GenesisAudioCoreLogic::ComputeBodyAudio(&Late, Week(38.0), Newborn);
	TestTrue(TEXT("Erster Atem"), FirstBreath.BreathRate > 30.0f);
	TestFalse(TEXT("Keine Mutter-Herztöne mehr von innen"), FirstBreath.bMaternalSounds);

	// Der Übergang ist schnell, aber nicht abrupt
	const FGenesisHearingPerception Step = GenesisAudioCoreLogic::SmoothPerception(LateHearing, Newborn, 1.0f / 60.0f, Tuning);
	TestTrue(TEXT("Erster Frame: Übergang begonnen"), Step.LowPassCutoffHz > LateHearing.LowPassCutoffHz && Step.LowPassCutoffHz < Newborn.LowPassCutoffHz);
	FGenesisHearingPerception Settled = Step;
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		Settled = GenesisAudioCoreLogic::SmoothPerception(Settled, Newborn, 1.0f / 60.0f, Tuning);
	}
	TestTrue(TEXT("Übergang als laufend markiert"), Step.bInEnvironmentTransition);
	TestTrue(FString::Printf(TEXT("Nach 2 s angekommen (%.0f Hz)"), Settled.LowPassCutoffHz), FMath::IsNearlyEqual(Settled.LowPassCutoffHz, Newborn.LowPassCutoffHz, Newborn.LowPassCutoffHz * 0.02f));
	TestFalse(TEXT("Übergang abgeschlossen"), Settled.bInEnvironmentTransition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisAudioLifeHearingTest, "Genesis.Audio.Core.HearingAcrossLife", GenesisAudioCoreTests::Flags)
bool FGenesisAudioLifeHearingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisAudioCoreTests;
	FGenesisBodyTuning BodyTuning;
	FGenesisHearingTuning Tuning;

	FGenesisBodyState Body = Fetus(38.0, BodyTuning);
	GenesisBodyLogic::Birth(Body, Week(38.0), BodyTuning);
	FGenesisTimestamp Now = Body.BirthTime + FGenesisTimestamp::YearsToSeconds(30.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 30.0 * 365.0, 0.0f, BodyTuning);
	GenesisBodyLogic::AdvanceHours(Body, Now + FGenesisTimestamp::SecondsPerHour, 1.0, 0.0f);

	const FGenesisHearingPerception Adult = GenesisAudioCoreLogic::ComputeHearing(&Body, Now, Tuning);
	TestTrue(TEXT("Erwachsen: volles Frequenzspektrum"), Adult.LowPassCutoffHz > 19000.0f);
	TestEqual(TEXT("Keine Altersabsenkung"), Adult.HighShelfGainDb, 0.0f);
	TestTrue(TEXT("Körper im Alltag kaum hörbar"), Adult.BodyAudibility < 0.2f);

	// Schock: Tunnel-Hören
	GenesisBodyLogic::ApplyAcuteStressor(Body, 1.0f);
	const FGenesisHearingPerception Shock = GenesisAudioCoreLogic::ComputeHearing(&Body, Now, Tuning);
	TestTrue(TEXT("Tunnel-Hören"), Shock.FocusNarrowing > 0.8f);
	TestTrue(TEXT("Umwelt tritt zurück"), Shock.ExternalAudibility < Adult.ExternalAudibility);
	TestTrue(TEXT("Eigener Körper tritt hervor"), Shock.BodyAudibility > 0.5f);
	const FGenesisBodyAudioParams ShockBody = GenesisAudioCoreLogic::ComputeBodyAudio(&Body, Now, Shock);
	TestTrue(TEXT("Herzschlag kräftig"), ShockBody.HeartStrength > 0.7f);

	// Alter: Hochtonverlust – begrenzt
	FGenesisBodyState Elder = Body;
	Elder.Hormones.Adrenaline = 0.0f;
	Elder.BiologicalAgeYears = 110.0;
	const FGenesisHearingPerception Old = GenesisAudioCoreLogic::ComputeHearing(&Elder, Now, Tuning);
	TestTrue(TEXT("Hochtonverlust im Alter"), Old.HighShelfGainDb < -5.0f);
	TestTrue(TEXT("…aber nie unangenehm stark"), Old.HighShelfGainDb >= Tuning.MaxAgeHighShelfLossDb);

	// Kopfverletzung: Tinnitus
	GenesisBodyLogic::ApplyInjury(Elder, GenesisBodyTags::Region_Head, 0.6f, Now);
	TestTrue(TEXT("Tinnitus nach Kopfverletzung"), GenesisAudioCoreLogic::ComputeHearing(&Elder, Now, Tuning).TinnitusLevel > 0.3f);

	// Herzverschleiß: unregelmäßiger Herzschlag
	Elder.Organ(EGenesisOrgan::Heart).Wear = 0.5f;
	TestTrue(TEXT("Unregelmäßiger Herzschlag"), GenesisAudioCoreLogic::ComputeBodyAudio(&Elder, Now, Old).HeartIrregularity > 0.29f);

	// Kein Körper: neutrales Hören
	const FGenesisHearingPerception Neutral = GenesisAudioCoreLogic::ComputeHearing(nullptr, Now, Tuning);
	TestTrue(TEXT("Ohne Körper neutral"), !Neutral.bInWomb && Neutral.LowPassCutoffHz >= 20000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisAudioMixTest, "Genesis.Audio.Core.MixDuckingIsSmooth", GenesisAudioCoreTests::Flags)
bool FGenesisAudioMixTest::RunTest(const FString& Parameters)
{
	FGenesisAudioMixTuning Tuning;
	FGenesisMixState State;
	const float FrameSeconds = 1.0f / 60.0f;

	FGenesisMixRequest Dialogue;
	Dialogue.Bus = EGenesisAudioBus::Dialogue;
	Dialogue.Importance = 1.0f;
	const TArray<FGenesisMixRequest> Speaking = { Dialogue };
	const TArray<FGenesisMixRequest> Silent;

	float LargestStep = 0.0f;
	float Previous = State.GetGainDb(EGenesisAudioBus::Music);
	for (int32 Frame = 0; Frame < 180; ++Frame)
	{
		GenesisAudioCoreLogic::UpdateMix(State, Speaking, Tuning, FrameSeconds);
		const float Current = State.GetGainDb(EGenesisAudioBus::Music);
		LargestStep = FMath::Max(LargestStep, FMath::Abs(Current - Previous));
		Previous = Current;
	}
	TestTrue(FString::Printf(TEXT("Musik unter Dialog abgesenkt (%.2f dB)"), State.GetGainDb(EGenesisAudioBus::Music)), FMath::IsNearlyEqual(State.GetGainDb(EGenesisAudioBus::Music), -8.0f, 0.1f));
	TestTrue(TEXT("Umgebung ebenfalls, aber weniger"), State.GetGainDb(EGenesisAudioBus::Ambient) < -4.5f && State.GetGainDb(EGenesisAudioBus::Ambient) > -5.5f);
	TestEqual(TEXT("Dialog selbst unverändert"), State.GetGainDb(EGenesisAudioBus::Dialogue), 0.0f);
	TestTrue(FString::Printf(TEXT("Keine hörbaren Sprünge (größter Schritt %.3f dB/Frame)"), LargestStep), LargestStep < 0.5f);

	// Rückkehr langsamer als Absenkung
	int32 FramesToRecover = 0;
	while (State.GetGainDb(EGenesisAudioBus::Music) < -0.5f && FramesToRecover < 1000)
	{
		GenesisAudioCoreLogic::UpdateMix(State, Silent, Tuning, FrameSeconds);
		++FramesToRecover;
	}
	TestTrue(FString::Printf(TEXT("Sanfte Rückkehr (%d Frames)"), FramesToRecover), FramesToRecover > 60 && FramesToRecover < 600);

	// Beiläufiger Satz duckt weniger
	FGenesisMixState Casual;
	Dialogue.Importance = 0.3f;
	for (int32 Frame = 0; Frame < 300; ++Frame)
	{
		GenesisAudioCoreLogic::UpdateMix(Casual, { Dialogue }, Tuning, FrameSeconds);
	}
	TestTrue(TEXT("Wichtigkeit skaliert Ducking"), Casual.GetGainDb(EGenesisAudioBus::Music) > -3.0f && Casual.GetGainDb(EGenesisAudioBus::Music) < -2.0f);

	// Mehrere Auslöser addieren sich, aber gedeckelt
	FGenesisMixState Crowded;
	FGenesisMixRequest Vital;
	Vital.Bus = EGenesisAudioBus::VitalSignal;
	FGenesisMixRequest Body;
	Body.Bus = EGenesisAudioBus::Body;
	Dialogue.Importance = 1.0f;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		GenesisAudioCoreLogic::UpdateMix(Crowded, { Dialogue, Vital, Body }, Tuning, FrameSeconds);
	}
	TestTrue(FString::Printf(TEXT("Gemeinsames Ducking gedeckelt (%.1f dB)"), Crowded.GetGainDb(EGenesisAudioBus::Music)), Crowded.GetGainDb(EGenesisAudioBus::Music) >= Tuning.MaxDuckDb - 0.01f);
	TestTrue(TEXT("Stärker als einzeln"), Crowded.GetGainDb(EGenesisAudioBus::Music) < -12.0f);

	TestTrue(TEXT("dB → linear"), FMath::IsNearlyEqual(GenesisAudioCoreLogic::DbToLinear(-6.0206f), 0.5f, 1.0e-3f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
