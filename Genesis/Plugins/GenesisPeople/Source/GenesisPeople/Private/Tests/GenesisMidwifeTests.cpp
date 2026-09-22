// GENESIS: Der Kreislauf des Lebens

#include "Misc/AutomationTest.h"
#include "GenesisMidwifeRig.h"
#include "GenesisPeopleRendering.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisMidwifeTaskTest, "Genesis.People.Midwife.FollowsTheBirth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenesisMidwifeTaskTest::RunTest(const FString& Parameters)
{
	using namespace GenesisMidwifeLogic;
	float Progress = 0.0f;
	TestEqual(TEXT("Unter der Geburt am Fußende"), TaskFor(false, false, 0.0f, 7.0f, Progress), EGenesisMidwifeTask::Attending);
	TestEqual(TEXT("Geboren: sie hält das Kind"), TaskFor(true, false, 0.0f, 7.0f, Progress), EGenesisMidwifeTask::Holding);
	TestEqual(TEXT("Beim Auflegen: sie reicht es hinüber"), TaskFor(true, true, 3.5f, 7.0f, Progress), EGenesisMidwifeTask::Handing);
	TestTrue(TEXT("…zur Hälfte"), FMath::IsNearlyEqual(Progress, 0.5f, 0.01f));
	TestEqual(TEXT("Auf der Brust rubbelt sie es trocken"), TaskFor(true, true, 8.0f, 7.0f, Progress, 14.0f, 4.0f), EGenesisMidwifeTask::Drying);
	TestEqual(TEXT("Dann deckt sie es zu"), TaskFor(true, true, 22.0f, 7.0f, Progress, 14.0f, 4.0f), EGenesisMidwifeTask::Covering);
	TestTrue(TEXT("…zur Hälfte"), FMath::IsNearlyEqual(Progress, 0.25f, 0.01f));
	TestEqual(TEXT("Danach sieht sie zu"), TaskFor(true, true, 26.0f, 7.0f, Progress, 14.0f, 4.0f), EGenesisMidwifeTask::Watching);

	// Sie hält das Kind: am Fußende des Bettes, der Mutter zugewandt, das Kind in ihren Händen vor ihr;
	// es schaut zu ihr hoch (ChildForward zeigt vom Kind zu ihrem Gesicht)
	FInputs In;
	In.Task = EGenesisMidwifeTask::Holding;
	In.FootOfBed = FVector(1000.0, 0.0, 0.0);
	In.MotherEye = FVector(-500.0, 0.0, 1000.0);
	In.Bedside = FVector(-500.0, 650.0, 1000.0);
	In.ChildEye = FVector(620.0, 0.0, 1350.0);
	In.ChildForward = FVector(1.0, 0.0, 0.4).GetSafeNormal();
	const FGenesisMidwifeStance Hold = Compute(In, 420.0f);
	TestTrue(TEXT("Sie bleibt am Fußende des Bettes"), FVector::Dist2D(Hold.Feet, In.FootOfBed) < 1.0);
	TestTrue(TEXT("Sie steht auf dem Boden"), FMath::IsNearlyZero(Hold.Feet.Z));
	TestTrue(TEXT("Sie ist der Mutter zugewandt"), FVector::DotProduct(Hold.Facing, FVector(-1.0, 0.0, 0.0)) > 0.99);
	TestTrue(TEXT("Sie schaut das Kind an"), Hold.LookAt.Equals(In.ChildEye, 1.0));
	TestTrue(TEXT("Die Hände sind am Kind"), Hold.HandsOnChild > 0.99f && FVector::Dist(Hold.RightHand, In.ChildEye) < 150.0);
	TestTrue(TEXT("Sie neigt sich dem Kind zu"), Hold.LeanDegPerVertebra > 1.0f && Hold.LeanDegPerVertebra < 5.0f);

	// Nach dem Auflegen: neben dem Bett, Hände frei, Blick beim Kind
	In.Task = EGenesisMidwifeTask::Watching;
	const FGenesisMidwifeStance Watch = Compute(In);
	TestTrue(TEXT("Neben dem Bett"), FVector::Dist2D(Watch.Feet, In.Bedside) < 1.0);
	TestTrue(TEXT("Hände frei"), Watch.HandsOnChild < 0.01f);

	// Beim Abtrocknen: neben dem Bett, beide Hände am Kind, über das Kind gebeugt – und die Hand bewegt sich
	In.Task = EGenesisMidwifeTask::Drying;
	In.Time = 0.0f;
	const FGenesisMidwifeStance DryA = Compute(In);
	In.Time = 0.15f;
	const FGenesisMidwifeStance DryB = Compute(In);
	TestTrue(TEXT("Beim Abtrocknen neben dem Bett"), FVector::Dist2D(DryA.Feet, In.Bedside) < 1.0);
	TestTrue(TEXT("Hände am Kind"), DryA.HandsOnChild > 0.99f && FVector::Dist(DryA.RightHand, In.ChildEye) < 250.0);
	TestTrue(TEXT("Sie rubbelt (die Hand bewegt sich)"), FVector::Dist(DryA.RightHand, DryB.RightHand) > 20.0);
	TestTrue(TEXT("Über das Kind gebeugt"), DryA.LeanDegPerVertebra > Watch.LeanDegPerVertebra + 3.0f);

	// Beim Hinüberreichen lässt sie erst am Ende los
	In.Task = EGenesisMidwifeTask::Handing;
	In.HandingProgress = 0.5f;
	TestTrue(TEXT("In der Mitte hält sie noch"), Compute(In).HandsOnChild > 0.99f);
	In.HandingProgress = 1.0f;
	TestTrue(TEXT("Am Ende hat die Mutter das Kind"), Compute(In).HandsOnChild < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenesisHairVoxelTest, "Genesis.People.HairVoxelsFollowScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenesisHairVoxelTest::RunTest(const FString& Parameters)
{
	// Am Menschen immer 3 mm: 0,3 Einheiten bei 1 cm je Einheit, 3 Einheiten im Kreißsaal (1 mm je Einheit, Figur zehnfach)
	TestTrue(TEXT("Normaler Maßstab"), FMath::IsNearlyEqual(GenesisPeopleRendering::HairVoxelWorldSize(1.0f), 0.3f));
	TestTrue(TEXT("Kreißsaal"), FMath::IsNearlyEqual(GenesisPeopleRendering::HairVoxelWorldSize(10.0f), 3.0f));
	return true;
}

#endif
