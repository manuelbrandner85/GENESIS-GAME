// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisBodyGameplayTags.h"
#include "GenesisBodyLogic.h"
#include "GenesisGeneticsLogic.h"
#include "GenesisPersistence.h"
#include "GenesisRandom.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisBodyTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const FGenesisTimestamp Conception = FGenesisTimestamp::FromCalendar(2000, 10, 8);
	constexpr double DaysPerWeek = 7.0;

	FGenesisBodyState NewBody(EGenesisSimulationLevel Level = EGenesisSimulationLevel::Full, const FGenesisBodyGenetics& Genetics = FGenesisBodyGenetics())
	{
		return GenesisBodyLogic::CreateAtConception(FGuid(7, 0, 0, 1), FGuid(7, 0, 0, 2), Genetics, FGenesisConceptionVitality(), Conception, Level);
	}

	FGenesisTimestamp AfterDays(double Days)
	{
		return Conception + FGenesisTimestamp::DaysToSeconds(Days);
	}

	/** Schwangerschaft bis Woche X und Geburt. */
	FGenesisBodyState BornAtWeek(double Weeks, const FGenesisBodyTuning& Tuning, EGenesisSimulationLevel Level = EGenesisSimulationLevel::Full,
		const FGenesisBodyGenetics& Genetics = FGenesisBodyGenetics())
	{
		FGenesisBodyState Body = NewBody(Level, Genetics);
		GenesisBodyLogic::AdvanceDays(Body, AfterDays(Weeks * DaysPerWeek), Weeks * DaysPerWeek, 0.0f, Tuning);
		GenesisBodyLogic::Birth(Body, AfterDays(Weeks * DaysPerWeek), Tuning);
		return Body;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyPrenatalTest, "Genesis.Body.PrenatalDevelopment", GenesisBodyTests::Flags)
bool FGenesisBodyPrenatalTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;
	FGenesisBodyState Body = NewBody();

	TestTrue(TEXT("Zygote"), GenesisBodyLogic::GetStage(Body, Conception) == EGenesisDevelopmentStage::Zygote);

	GenesisBodyLogic::AdvanceDays(Body, AfterDays(8 * DaysPerWeek), 8 * DaysPerWeek, 0.0f, Tuning);
	TestTrue(TEXT("Woche 8: Embryo"), GenesisBodyLogic::GetStage(Body, AfterDays(8 * DaysPerWeek)) == EGenesisDevelopmentStage::Embryo);
	TestTrue(TEXT("Woche 8: Herz weitgehend ausgebildet"), Body.Organ(EGenesisOrgan::Heart).Development > 0.8f);
	TestTrue(TEXT("Woche 8: Lunge noch kaum"), Body.Organ(EGenesisOrgan::Lungs).Development < 0.1f);
	TestEqual(TEXT("Woche 8: noch kein Hören"), Body.Sense(EGenesisBodySense::Hearing).Development, 0.0f);
	TestTrue(TEXT("Woche 8: Tastsinn beginnt"), Body.Sense(EGenesisBodySense::Touch).Development > 0.0f);

	GenesisBodyLogic::AdvanceHours(Body, AfterDays(8 * DaysPerWeek), 1.0, 0.0f);
	TestTrue(TEXT("Fetaler Herzschlag"), Body.Vitals.HeartRate > 110.0f);

	GenesisBodyLogic::AdvanceDays(Body, AfterDays(26 * DaysPerWeek), 18 * DaysPerWeek, 0.0f, Tuning);
	TestTrue(TEXT("Woche 26: Fetus"), GenesisBodyLogic::GetStage(Body, AfterDays(26 * DaysPerWeek)) == EGenesisDevelopmentStage::Fetus);
	TestTrue(TEXT("Woche 26: hört die Welt draußen"), Body.Sense(EGenesisBodySense::Hearing).Development > 0.99f);
	TestTrue(TEXT("Hören im Mutterleib gedämpft"), GenesisBodyLogic::GetSymptomIntensity(GenesisBodyLogic::DeriveSymptoms(Body, AfterDays(26 * DaysPerWeek)), GenesisBodyTags::Symptom_MuffledHearing) > 0.5f);

	// Schwächere Entwicklungsqualität (z. B. Embryo-Minispiel) begrenzt die Ausbildung
	FGenesisBodyState Weaker = NewBody();
	Weaker.Organ(EGenesisOrgan::Heart).DevelopmentQuality = 0.2f;
	GenesisBodyLogic::AdvanceDays(Weaker, AfterDays(20 * DaysPerWeek), 20 * DaysPerWeek, 0.0f, Tuning);
	TestTrue(TEXT("Entwicklungsqualität wirkt"), Weaker.Organ(EGenesisOrgan::Heart).Development < Body.Organ(EGenesisOrgan::Heart).Development);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyBirthTest, "Genesis.Body.BirthAndNewbornPerception", GenesisBodyTests::Flags)
bool FGenesisBodyBirthTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;

	FGenesisBodyState FullTerm = BornAtWeek(38.0, Tuning);
	const FGenesisTimestamp BirthTime = FullTerm.BirthTime;
	TestTrue(TEXT("Geboren"), FullTerm.bBorn);
	TestTrue(TEXT("Neugeborenes"), GenesisBodyLogic::GetStage(FullTerm, BirthTime) == EGenesisDevelopmentStage::Newborn);
	TestEqual(TEXT("Termingeburt ohne Atemnot"), FullTerm.Conditions.Num(), 0);
	TestTrue(TEXT("Erster Atemzug: Adrenalin"), FullTerm.Hormones.Adrenaline > 0.5f);
	TestTrue(TEXT("Neugeborenes ~50 cm"), FMath::IsNearlyEqual(FullTerm.HeightCm, 50.0f, 2.0f));

	const TArray<FGenesisSymptom> NewbornSymptoms = GenesisBodyLogic::DeriveSymptoms(FullTerm, BirthTime);
	TestTrue(TEXT("Neugeborene sehen sehr unscharf"), GenesisBodyLogic::GetSymptomIntensity(NewbornSymptoms, GenesisBodyTags::Symptom_BlurredVision) > 0.9f);

	// Frühgeburt in Woche 26: unreife Lunge
	FGenesisBodyState Premature = BornAtWeek(26.0, Tuning);
	TestTrue(TEXT("Frühgeburt: Atemnot"), Premature.Conditions.ContainsByPredicate([](const FGenesisBodyCondition& Condition)
	{
		return Condition.Condition == GenesisBodyTags::Condition_RespiratoryDistress;
	}));
	GenesisBodyLogic::AdvanceHours(Premature, Premature.BirthTime + FGenesisTimestamp::SecondsPerHour, 1.0, 0.0f);
	TestTrue(TEXT("Frühgeborenes atmet schneller"), Premature.Vitals.RespiratoryRate > FullTerm.Vitals.RespiratoryRate);

	// Sehen reift: mit 5 Jahren scharf
	GenesisBodyLogic::AdvanceDays(FullTerm, BirthTime + FGenesisTimestamp::YearsToSeconds(5.0), 5.0 * 365.0, 0.0f, Tuning);
	const TArray<FGenesisSymptom> ChildSymptoms = GenesisBodyLogic::DeriveSymptoms(FullTerm, BirthTime + FGenesisTimestamp::YearsToSeconds(5.0));
	TestTrue(TEXT("Mit 5 Jahren scharf sehen"), GenesisBodyLogic::GetSymptomIntensity(ChildSymptoms, GenesisBodyTags::Symptom_BlurredVision) < 0.1f);
	TestTrue(TEXT("Kind"), GenesisBodyLogic::GetStage(FullTerm, BirthTime + FGenesisTimestamp::YearsToSeconds(5.0)) == EGenesisDevelopmentStage::Child);
	TestTrue(TEXT("Gewachsen"), FullTerm.HeightCm > 100.0f);

	// Frühgeborenes holt die Lungenreife nach
	GenesisBodyLogic::AdvanceDays(Premature, Premature.BirthTime + FGenesisTimestamp::YearsToSeconds(1.0), 365.0, 0.0f, Tuning);
	TestTrue(TEXT("Lunge nachgereift"), Premature.Organ(EGenesisOrgan::Lungs).Development > 0.99f);
	TestEqual(TEXT("Atemnot überwunden"), Premature.Conditions.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyVitalsTest, "Genesis.Body.VitalsSleepAndSymptoms", GenesisBodyTests::Flags)
bool FGenesisBodyVitalsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;
	FGenesisBodyState Body = BornAtWeek(38.0, Tuning);
	FGenesisTimestamp Now = Body.BirthTime + FGenesisTimestamp::YearsToSeconds(25.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 25.0 * 365.0, 0.0f, Tuning);

	TestTrue(TEXT("Neugeborene haben höheren Ruhepuls"), GenesisBodyLogic::GetBaselineHeartRate(0.0) > GenesisBodyLogic::GetBaselineHeartRate(25.0) + 50.0f);

	// Ruhe
	Body.Activity = EGenesisActivity::Rest;
	for (int32 Hour = 0; Hour < 3; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	}
	const float RestingHeartRate = Body.Vitals.HeartRate;
	TestTrue(FString::Printf(TEXT("Ruhepuls Erwachsener plausibel (%.0f)"), RestingHeartRate), RestingHeartRate > 55.0f && RestingHeartRate < 95.0f);

	// Intensive Anstrengung
	Body.Activity = EGenesisActivity::Intense;
	Now += FGenesisTimestamp::SecondsPerHour;
	GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	TestTrue(TEXT("Puls steigt"), Body.Vitals.HeartRate > RestingHeartRate * 1.6f);
	const TArray<FGenesisSymptom> Exerted = GenesisBodyLogic::DeriveSymptoms(Body, Now);
	TestTrue(TEXT("Herzklopfen spürbar"), GenesisBodyLogic::GetSymptomIntensity(Exerted, GenesisBodyTags::Symptom_HeartPounding) > 0.0f);

	// Erholung
	Body.Activity = EGenesisActivity::Rest;
	for (int32 Hour = 0; Hour < 2; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	}
	TestTrue(TEXT("Puls erholt sich"), Body.Vitals.HeartRate < RestingHeartRate * 1.2f);

	// 30 Stunden wach
	for (int32 Hour = 0; Hour < 30; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
		GenesisBodyLogic::Eat(Body, 0.7f);
		GenesisBodyLogic::Drink(Body);
	}
	TestTrue(TEXT("Übermüdung spürbar"), GenesisBodyLogic::GetSymptomIntensity(GenesisBodyLogic::DeriveSymptoms(Body, Now), GenesisBodyTags::Symptom_Fatigue) > 0.5f);

	// 9 Stunden Schlaf
	Body.Activity = EGenesisActivity::Sleep;
	for (int32 Hour = 0; Hour < 9; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	}
	TestTrue(TEXT("Schlaf baut Schlafdruck ab"), Body.Vitals.SleepPressure < 0.2f);
	TestTrue(FString::Printf(TEXT("Geschlafen bis in die Nacht (Stunde %d)"), Now.GetHourOfDay()), Now.GetHourOfDay() < 6);
	TestTrue(TEXT("Nachts Melatonin"), Body.Hormones.Melatonin > 0.3f);

	// Hunger und Durst ohne Essen
	Body.Activity = EGenesisActivity::Light;
	for (int32 Hour = 0; Hour < 8; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	}
	const TArray<FGenesisSymptom> Needs = GenesisBodyLogic::DeriveSymptoms(Body, Now);
	TestTrue(TEXT("Hunger"), GenesisBodyLogic::GetSymptomIntensity(Needs, GenesisBodyTags::Symptom_Hunger) > 0.5f);
	TestTrue(TEXT("Durst"), GenesisBodyLogic::GetSymptomIntensity(Needs, GenesisBodyTags::Symptom_Thirst) > 0.0f);

	// Psychischer Stress erhöht Cortisol
	const float CortisolBefore = Body.Hormones.Cortisol;
	for (int32 Hour = 0; Hour < 4; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 1.0f);
	}
	TestTrue(TEXT("Stress → Cortisol"), Body.Hormones.Cortisol > CortisolBefore + 0.2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyAgingTest, "Genesis.Body.LifestyleShapesAging", GenesisBodyTests::Flags)
bool FGenesisBodyAgingTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;

	FGenesisBodyState Base = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Reduced);
	FGenesisTimestamp Now = Base.BirthTime + FGenesisTimestamp::YearsToSeconds(30.0);
	GenesisBodyLogic::AdvanceDays(Base, Now, 30.0 * 365.0, 0.0f, Tuning);

	FGenesisBodyState Stressed = Base;
	FGenesisBodyState Active = Base;

	// 30 weitere Jahre, Monat für Monat: dauerhafter Druck vs. Bewegung und Ruhe
	for (int32 Month = 0; Month < 360; ++Month)
	{
		const FGenesisTimestamp MonthEnd = Now + FGenesisTimestamp::DaysToSeconds(30.0 * (Month + 1));
		GenesisBodyLogic::AdvanceDays(Stressed, MonthEnd, 30.0, 0.9f, Tuning);
		GenesisBodyLogic::RecordExercise(Active, 12.0f, 0.8f);
		GenesisBodyLogic::AdvanceDays(Active, MonthEnd, 30.0, 0.1f, Tuning);
	}

	AddInfo(FString::Printf(TEXT("Mit 60: biologisches Alter gestresst %.1f / aktiv %.1f | Herzgesundheit %.2f / %.2f | Herzverschleiß %.3f / %.3f"),
		Stressed.BiologicalAgeYears, Active.BiologicalAgeYears, Stressed.Organ(EGenesisOrgan::Heart).GetHealth(), Active.Organ(EGenesisOrgan::Heart).GetHealth(),
		Stressed.Organ(EGenesisOrgan::Heart).Wear, Active.Organ(EGenesisOrgan::Heart).Wear));
	TestTrue(FString::Printf(TEXT("Stress altert schneller (biologisch %.1f vs %.1f)"), Stressed.BiologicalAgeYears, Active.BiologicalAgeYears),
		Stressed.BiologicalAgeYears > Active.BiologicalAgeYears + 5.0);
	TestTrue(TEXT("Herz: Stress hinterlässt Verschleiß"), Stressed.Organ(EGenesisOrgan::Heart).Wear > Active.Organ(EGenesisOrgan::Heart).Wear);
	TestTrue(TEXT("Herz: aktiver Mensch gesünder"), Active.Organ(EGenesisOrgan::Heart).GetHealth() > Stressed.Organ(EGenesisOrgan::Heart).GetHealth());
	TestTrue(TEXT("Fitness"), Active.Fitness > 0.7f && Stressed.Fitness < 0.3f);
	TestTrue(TEXT("Beide Erwachsene über 55 sind 'Elder'"), GenesisBodyLogic::GetStage(Active, Now + FGenesisTimestamp::YearsToSeconds(30.0)) == EGenesisDevelopmentStage::Elder);

	// Genetisches Herz-Risiko beschleunigt den Rückgang
	FGenesisBodyGenetics RiskGenetics;
	RiskGenetics.CardioRisk = 0.8f;
	FGenesisBodyState Risk = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Reduced, RiskGenetics);
	FGenesisBodyState NoRisk = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Reduced);
	const FGenesisTimestamp Sixty = Risk.BirthTime + FGenesisTimestamp::YearsToSeconds(60.0);
	GenesisBodyLogic::AdvanceDays(Risk, Sixty, 60.0 * 365.0, 0.3f, Tuning);
	GenesisBodyLogic::AdvanceDays(NoRisk, Sixty, 60.0 * 365.0, 0.3f, Tuning);
	TestTrue(TEXT("Genetisches Risiko: schwächeres Herz mit 60"), Risk.Organ(EGenesisOrgan::Heart).GetHealth() < NoRisk.Organ(EGenesisOrgan::Heart).GetHealth());

	// Sinne im Alter
	TestTrue(TEXT("Mit 60 nachlassende Nahsicht"), NoRisk.Sense(EGenesisBodySense::Sight).Acuity < 0.95f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyConditionsTest, "Genesis.Body.InjuryIllnessAndScars", GenesisBodyTests::Flags)
bool FGenesisBodyConditionsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;
	FGenesisBodyState Body = BornAtWeek(38.0, Tuning);
	FGenesisTimestamp Now = Body.BirthTime + FGenesisTimestamp::YearsToSeconds(20.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 20.0 * 365.0, 0.0f, Tuning);

	// Schwere Armverletzung
	GenesisBodyLogic::ApplyInjury(Body, GenesisBodyTags::Region_Arm, 0.8f, Now);
	GenesisBodyLogic::AdvanceHours(Body, Now + FGenesisTimestamp::SecondsPerHour, 1.0, 0.0f);
	TestTrue(TEXT("Schmerz"), GenesisBodyLogic::GetSymptomIntensity(GenesisBodyLogic::DeriveSymptoms(Body, Now), GenesisBodyTags::Symptom_Pain) > 0.7f);

	Now += FGenesisTimestamp::DaysToSeconds(90.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 90.0, 0.0f, Tuning);
	TestEqual(TEXT("Verletzung verheilt"), Body.Conditions.Num(), 0);
	TestEqual(TEXT("Sichtbare Narbe bleibt"), Body.Scars.Num(), 1);
	TestTrue(TEXT("Narbe am Arm"), Body.Scars.Num() == 1 && Body.Scars[0].Region == GenesisBodyTags::Region_Arm);

	// Infektion mit Fieber
	GenesisBodyLogic::ApplyIllness(Body, GenesisBodyTags::Condition_Infection, 0.7f, 0.0f, 0.1f, false, true, EGenesisOrgan::Immune, Now);
	Now += FGenesisTimestamp::SecondsPerHour;
	GenesisBodyLogic::AdvanceHours(Body, Now, 1.0, 0.0f);
	const TArray<FGenesisSymptom> Sick = GenesisBodyLogic::DeriveSymptoms(Body, Now);
	TestTrue(TEXT("Fieber"), GenesisBodyLogic::GetSymptomIntensity(Sick, GenesisBodyTags::Symptom_Fever) > 0.3f);
	TestTrue(TEXT("Übelkeit"), GenesisBodyLogic::GetSymptomIntensity(Sick, GenesisBodyTags::Symptom_Nausea) > 0.2f);

	Now += FGenesisTimestamp::DaysToSeconds(14.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 14.0, 0.0f, Tuning);
	TestEqual(TEXT("Infektion überstanden"), Body.Conditions.Num(), 0);
	TestEqual(TEXT("Infektionen hinterlassen keine Narbe"), Body.Scars.Num(), 1);

	// Chronische Erkrankung bleibt
	GenesisBodyLogic::ApplyIllness(Body, GenesisBodyTags::Condition_Cardiac, 0.2f, 0.0f, 0.5f, true, false, EGenesisOrgan::Heart, Now);
	GenesisBodyLogic::AdvanceDays(Body, Now + FGenesisTimestamp::DaysToSeconds(60.0), 60.0, 0.0f, Tuning);
	TestEqual(TEXT("Chronisch bleibt"), Body.Conditions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyVitalFailureTest, "Genesis.Body.VitalFailureAndNaturalEnd", GenesisBodyTests::Flags)
bool FGenesisBodyVitalFailureTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;

	// Akutes Organversagen
	FGenesisBodyState Body = BornAtWeek(38.0, Tuning);
	FGenesisTimestamp Now = Body.BirthTime + FGenesisTimestamp::YearsToSeconds(40.0);
	GenesisBodyLogic::AdvanceDays(Body, Now, 40.0 * 365.0, 0.0f, Tuning);
	TestTrue(TEXT("Lebt"), Body.bAlive);
	Body.Organ(EGenesisOrgan::Heart).Wear = 0.99f;
	const bool bFailed = GenesisBodyLogic::AdvanceDays(Body, Now + FGenesisTimestamp::DaysToSeconds(1.0), 1.0, 0.0f, Tuning);
	TestTrue(TEXT("Versagen gemeldet"), bFailed);
	TestFalse(TEXT("Nicht mehr am Leben"), Body.bAlive);
	TestTrue(TEXT("Stufe verstorben"), GenesisBodyLogic::GetStage(Body, Now) == EGenesisDevelopmentStage::Deceased);
	TestFalse(TEXT("Keine weitere Simulation"), GenesisBodyLogic::AdvanceDays(Body, Now + FGenesisTimestamp::DaysToSeconds(10.0), 9.0, 0.0f, Tuning));

	// Natürliches Lebensende entsteht aus Alterung – ohne festes Todesdatum
	FGenesisBodyState Elder = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Reduced);
	GenesisBodyLogic::AdvanceDays(Elder, Elder.BirthTime + FGenesisTimestamp::YearsToSeconds(150.0), 150.0 * 365.0, 0.2f, Tuning);
	TestFalse(TEXT("Niemand wird 150"), Elder.bAlive);
	const double AgeAtDeath = FGenesisTimestamp::YearsBetween(Elder.BirthTime, Elder.DeathTime);
	AddInfo(FString::Printf(TEXT("Natürliches Lebensende bei leichtem Dauerstress: %.1f Jahre"), AgeAtDeath));
	TestTrue(FString::Printf(TEXT("Natürliches Ende in plausiblem Alter (%.0f)"), AgeAtDeath), AgeAtDeath > 70.0 && AgeAtDeath < 125.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyGeneticsTest, "Genesis.Body.GeneticsAndPersistence", GenesisBodyTests::Flags)
bool FGenesisBodyGeneticsTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;

	const TArray<FGenesisTraitDefinition> Traits = GenesisGeneticsLogic::MakeDefaultTraitDefinitions();
	FGenesisRandomStream Rng(17);
	const FGenesisGenome Genome = GenesisGeneticsLogic::CreateFounderGenome(Traits, Rng);
	const FGenesisBodyGenetics Genetics = GenesisBodyLogic::MakeGenetics(&Genome, Traits);
	TestTrue(TEXT("Erwachsenengröße aus Genom"), Genetics.AdultHeightCm >= 150.0f && Genetics.AdultHeightCm <= 195.0f);

	FGenesisBodyState Body = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Full, Genetics);
	const FGenesisTimestamp Adult = Body.BirthTime + FGenesisTimestamp::YearsToSeconds(25.0);
	GenesisBodyLogic::AdvanceDays(Body, Adult, 25.0 * 365.0, 0.0f, Tuning);
	TestTrue(TEXT("Erwachsen erreicht genetische Größe"), FMath::IsNearlyEqual(Body.HeightCm, Genetics.AdultHeightCm, 1.0f));

	TArray<uint8> Bytes;
	TestTrue(TEXT("Schreiben"), GenesisPersistence::Write(Body, Bytes));
	FGenesisBodyState Loaded;
	TestTrue(TEXT("Lesen"), GenesisPersistence::Read(Loaded, Bytes));
	TestEqual(TEXT("Organe"), Loaded.Organs.Num(), GenesisOrganCount);
	TestEqual(TEXT("Biologisches Alter"), Loaded.BiologicalAgeYears, Body.BiologicalAgeYears);
	TestEqual(TEXT("Größe"), Loaded.HeightCm, Body.HeightCm);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisBodyPerformanceTest, "Genesis.Body.Performance.Population", GenesisBodyTests::Flags)
bool FGenesisBodyPerformanceTest::RunTest(const FString& Parameters)
{
	using namespace GenesisBodyTests;
	FGenesisBodyTuning Tuning;

	TArray<FGenesisBodyState> Bodies;
	const FGenesisBodyState Template = BornAtWeek(38.0, Tuning, EGenesisSimulationLevel::Reduced);
	FGenesisTimestamp Now = Template.BirthTime;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		Bodies.Add(Template);
	}
	for (int32 Index = 0; Index < 50; ++Index)
	{
		Bodies[Index].SimulationLevel = EGenesisSimulationLevel::Full;
	}

	const double HourStart = FPlatformTime::Seconds();
	for (int32 Hour = 0; Hour < 24; ++Hour)
	{
		Now += FGenesisTimestamp::SecondsPerHour;
		for (int32 Index = 0; Index < 50; ++Index)
		{
			GenesisBodyLogic::AdvanceHours(Bodies[Index], Now, 1.0, 0.2f);
		}
	}
	const double HourMs = (FPlatformTime::Seconds() - HourStart) * 1000.0 / 24.0;

	const double DayStart = FPlatformTime::Seconds();
	for (FGenesisBodyState& Body : Bodies)
	{
		GenesisBodyLogic::AdvanceDays(Body, Now, 1.0, 0.2f, Tuning);
	}
	const double DayMs = (FPlatformTime::Seconds() - DayStart) * 1000.0;

	AddInfo(FString::Printf(TEXT("Stundenschritt 50 Level-1-Körper: %.3f ms | Tagesschritt 1000 Körper: %.3f ms"), HourMs, DayMs));
	TestTrue(FString::Printf(TEXT("Stundenschritt < 1 ms (%.3f)"), HourMs), HourMs < 1.0);
	TestTrue(FString::Printf(TEXT("Tagesschritt < 10 ms (%.3f)"), DayMs), DayMs < 10.0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
