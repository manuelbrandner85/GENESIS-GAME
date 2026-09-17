// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisSaveGame.h"
#include "GenesisSaveSubsystem.h"
#include "GenesisSaveTestSystem.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GenesisSaveTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	UGenesisSaveTestSystem* MakeSystem(FName Id, int64 NowSeconds, double TimeScale)
	{
		UGenesisSaveTestSystem* System = NewObject<UGenesisSaveTestSystem>();
		System->SystemId = Id;
		System->State.Now = FGenesisTimestamp(NowSeconds);
		System->State.TimeScale = TimeScale;
		return System;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSaveRoundTripTest, "Genesis.Save.RoundTripThroughBytes", GenesisSaveTests::Flags)
bool FGenesisSaveRoundTripTest::RunTest(const FString& Parameters)
{
	UGenesisSaveTestSystem* SystemA = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.A"), 1000, 5.0);
	UGenesisSaveTestSystem* SystemB = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.B"), 2000, 7.0);
	TArray<IGenesisPersistentSystem*> Systems = { SystemA, SystemB };

	EGenesisSaveResult CaptureResult;
	UGenesisSaveGame* Captured = UGenesisSaveSubsystem::CaptureSystems(Systems, EGenesisPersistenceScope::World, CaptureResult);
	TestTrue(TEXT("Capture erfolgreich"), CaptureResult == EGenesisSaveResult::Success);
	TestEqual(TEXT("Zwei Records"), Captured->Records.Num(), 2);

	// Über den echten Engine-Serialisierungspfad (wie beim Schreiben auf Disk)
	TArray<uint8> Bytes;
	TestTrue(TEXT("SaveGameToMemory"), UGameplayStatics::SaveGameToMemory(Captured, Bytes));
	UGenesisSaveGame* Loaded = Cast<UGenesisSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("LoadGameFromMemory"), Loaded))
	{
		return false;
	}

	FString Error;
	TestTrue(TEXT("Validierung"), Loaded->Validate(Error));

	// Zustand verändern, dann Speicherstand anwenden
	SystemA->State.Now = FGenesisTimestamp(1);
	SystemB->State.TimeScale = 99.0;
	TestTrue(TEXT("Apply erfolgreich"), UGenesisSaveSubsystem::ApplyToSystems(*Loaded, Systems) == EGenesisSaveResult::Success);
	TestEqual(TEXT("A wiederhergestellt"), SystemA->State.Now.Seconds, 1000ll);
	TestTrue(TEXT("B wiederhergestellt"), FMath::IsNearlyEqual(SystemB->State.TimeScale, 7.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSaveChecksumTest, "Genesis.Save.DetectsCorruption", GenesisSaveTests::Flags)
bool FGenesisSaveChecksumTest::RunTest(const FString& Parameters)
{
	UGenesisSaveTestSystem* System = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.A"), 42, 1.0);
	TArray<IGenesisPersistentSystem*> Systems = { System };

	EGenesisSaveResult CaptureResult;
	UGenesisSaveGame* Captured = UGenesisSaveSubsystem::CaptureSystems(Systems, EGenesisPersistenceScope::World, CaptureResult);

	FString Error;
	TestTrue(TEXT("Unverändert gültig"), Captured->Validate(Error));

	Captured->Records[0].Payload[Captured->Records[0].Payload.Num() / 2] ^= 0xFF;
	TestFalse(TEXT("Manipulierter Payload wird erkannt"), Captured->Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSaveVersionAndRollbackTest, "Genesis.Save.VersionGuardAndRollback", GenesisSaveTests::Flags)
bool FGenesisSaveVersionAndRollbackTest::RunTest(const FString& Parameters)
{
	// Diese Fehlermeldungen sind das geprüfte Verhalten
	AddExpectedMessagePlain(TEXT("dieser Build unterstützt nur"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("konnte seinen Zustand nicht wiederherstellen"), ELogVerbosity::Error, EAutomationExpectedMessageFlags::Contains, 1);

	UGenesisSaveTestSystem* SystemA = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.A"), 100, 1.0);
	UGenesisSaveTestSystem* SystemB = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.B"), 200, 1.0);
	TArray<IGenesisPersistentSystem*> Systems = { SystemA, SystemB };

	EGenesisSaveResult CaptureResult;
	UGenesisSaveGame* Saved = UGenesisSaveSubsystem::CaptureSystems(Systems, EGenesisPersistenceScope::World, CaptureResult);

	// Neuere Schema-Version im Speicherstand → nichts wird verändert
	Saved->Records[0].SchemaVersion = 5;
	SystemA->State.Now = FGenesisTimestamp(111);
	TestTrue(TEXT("Neuere Version abgelehnt"), UGenesisSaveSubsystem::ApplyToSystems(*Saved, Systems) == EGenesisSaveResult::NewerVersion);
	TestEqual(TEXT("Keine Teiländerung"), SystemA->State.Now.Seconds, 111ll);
	Saved->Records[0].SchemaVersion = 1;

	// Ladefehler in System B → System A wird auf den Zustand vor dem Laden zurückgerollt
	SystemA->State.Now = FGenesisTimestamp(555);
	SystemB->bFailOnLoad = true;
	TestTrue(TEXT("Ladefehler gemeldet"), UGenesisSaveSubsystem::ApplyToSystems(*Saved, Systems) == EGenesisSaveResult::ApplyFailed);
	TestEqual(TEXT("A zurückgerollt"), SystemA->State.Now.Seconds, 555ll);

	// System ohne Record im Speicherstand → Neuzustand
	UGenesisSaveTestSystem* NewSystem = GenesisSaveTests::MakeSystem(TEXT("Genesis.Test.New"), 999, 1.0);
	SystemB->bFailOnLoad = false;
	Systems.Add(NewSystem);
	TestTrue(TEXT("Apply mit neuem System"), UGenesisSaveSubsystem::ApplyToSystems(*Saved, Systems) == EGenesisSaveResult::Success);
	TestEqual(TEXT("Neues System zurückgesetzt"), NewSystem->ResetCount, 1);
	TestEqual(TEXT("A geladen"), SystemA->State.Now.Seconds, 100ll);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisSaveSlotNamingTest, "Genesis.Save.SlotNaming", GenesisSaveTests::Flags)
bool FGenesisSaveSlotNamingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Unsichere Zeichen entfernt"), UGenesisSaveSubsystem::SanitizeProfileId(TEXT("Ma/nu el..\\01")), FString(TEXT("Manuel01")));
	TestEqual(TEXT("Soul-Slot"), UGenesisSaveSubsystem::BuildSlotName(TEXT("P1"), EGenesisPersistenceScope::Soul, 3), FString(TEXT("Genesis_P1_Soul")));
	TestEqual(TEXT("Life-Slot"), UGenesisSaveSubsystem::BuildSlotName(TEXT("P1"), EGenesisPersistenceScope::Life, 3), FString(TEXT("Genesis_P1_Life_3")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
