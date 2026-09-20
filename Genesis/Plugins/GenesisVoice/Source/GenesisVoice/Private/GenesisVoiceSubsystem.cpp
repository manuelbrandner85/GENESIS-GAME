// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBodySubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisDebug.h"
#include "GenesisGeneticsSubsystem.h"
#include "GenesisLog.h"
#include "GenesisRandom.h"
#include "GenesisVoiceLogic.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	UGenesisVoiceSubsystem* GetVoiceSubsystem(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisVoiceSubsystem>() : nullptr;
	}

	EGenesisUtterance ParseUtterance(const FString& Name)
	{
		const FString Lower = Name.ToLower();
		if (Lower == TEXT("schrei") || Lower == TEXT("cry")) return EGenesisUtterance::Cry;
		if (Lower == TEXT("quengeln") || Lower == TEXT("fuss")) return EGenesisUtterance::Fuss;
		if (Lower == TEXT("gurren") || Lower == TEXT("coo")) return EGenesisUtterance::Coo;
		if (Lower == TEXT("lallen") || Lower == TEXT("babble")) return EGenesisUtterance::Babble;
		if (Lower == TEXT("lachen") || Lower == TEXT("laugh")) return EGenesisUtterance::Laugh;
		if (Lower == TEXT("seufzen") || Lower == TEXT("sigh")) return EGenesisUtterance::Sigh;
		if (Lower == TEXT("summen") || Lower == TEXT("hum")) return EGenesisUtterance::Hum;
		if (Lower == TEXT("beruhigen") || Lower == TEXT("soothe")) return EGenesisUtterance::Soothe;
		if (Lower == TEXT("rufen") || Lower == TEXT("call")) return EGenesisUtterance::Call;
		return EGenesisUtterance::Speak;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisVoiceSayCommand(
		TEXT("genesis.Voice.Say"),
		TEXT("Die Mutter macht einen Laut: <sprechen|beruhigen|summen|lachen|seufzen|rufen> [Stärke 0..1]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisVoiceSubsystem* Voice = GetVoiceSubsystem(World))
			{
				FGenesisUtterance Utterance;
				Utterance.Type = ParseUtterance(Args.Num() > 0 ? Args[0] : TEXT("sprechen"));
				Utterance.Intensity = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.5f;
				Utterance.Seed = FMath::Rand();
				Voice->Say(UGenesisVoiceSubsystem::GetMotherId(), Utterance);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisVoiceCryCommand(
		TEXT("genesis.Voice.Cry"),
		TEXT("Das Kind schreit: [Stärke 0..1]. Sonst geschieht das von selbst, wenn es ihm schlecht geht."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGenesisVoiceSubsystem* Voice = GetVoiceSubsystem(World);
			const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
			if (!Voice || !Body)
			{
				return;
			}

			// Das jüngste geborene Kind der Welt – im Vertical Slice ist das der Spieler
			FGuid Child;
			double Youngest = TNumericLimits<double>::Max();
			for (int32 Index = 0; Index < Body->GetBodyCount(); ++Index)
			{
				const FGenesisBodyState* Candidate = Body->GetBodyByIndex(Index);
				if (Candidate && Candidate->bAlive && Candidate->bBorn && Candidate->BiologicalAgeYears < Youngest)
				{
					Youngest = Candidate->BiologicalAgeYears;
					Child = Candidate->EntityId;
				}
			}

			FGenesisUtterance Utterance;
			Utterance.Type = EGenesisUtterance::Cry;
			Utterance.Intensity = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.9f;
			Utterance.Seed = FMath::Rand();
			Voice->Say(Child, Utterance);
		}));
}
#endif

void UGenesisVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisterDebugPage();
}

void UGenesisVoiceSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Voice"));
#endif
	Super::Deinitialize();
}

FGuid UGenesisVoiceSubsystem::GetMotherId()
{
	// Feste Kennung, damit Befehle, Actors und Anzeige dieselbe Platzhalter-Stimme meinen
	return FGuid(0x4D555454, 0x45520001, 0x00000000, 0x00000001);
}

FGenesisVoiceProfile UGenesisVoiceSubsystem::BuildProfile(const FGenesisVoiceInputs& Inputs) const
{
	return GenesisVoiceLogic::BuildProfile(Inputs, Tuning);
}

FGenesisVoiceInputs UGenesisVoiceSubsystem::MakeInputsFromBody(const FGenesisBodyState& Body) const
{
	FGenesisVoiceInputs Inputs;
	Inputs.AgeYears = static_cast<float>(Body.BiologicalAgeYears);
	Inputs.HeightCm = Body.HeightCm > 10.0f ? Body.HeightCm : 50.0f;

	// Die Stimme hängt an Kehlkopf und Lunge – im Körpermodell ist die Lunge das, was es davon gibt
	Inputs.RespiratoryHealth = FMath::Clamp(Body.Organ(EGenesisOrgan::Lungs).GetHealth(), 0.05f, 1.0f);

	// Eine akute Erkrankung der Atemwege macht heiser; andere Krankheiten zählen halb
	float Illness = 0.0f;
	for (const FGenesisBodyCondition& Condition : Body.Conditions)
	{
		const float Weight = Condition.AffectedOrgan == EGenesisOrgan::Lungs ? 1.0f : 0.4f;
		Illness = FMath::Max(Illness, Condition.Severity * Weight);
	}
	Inputs.Illness = FMath::Clamp(Illness, 0.0f, 1.0f);

	// Schlafmangel hört man: Nach zwölf Stunden Defizit ist die Stimme deutlich matter
	Inputs.Exhaustion = FMath::Clamp(Body.SleepDebtHours / 12.0f + Body.ChronicStress * 0.3f, 0.0f, 1.0f);

	// Geschlecht und individuelle Streuung stehen im Genom
	Inputs.IndividualSeed = GenesisHash::FromGuid(Body.GenomeId);
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UGenesisGeneticsSubsystem* Genetics = GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>())
		{
			if (const FGenesisGenome* Genome = Genetics->FindGenome(Body.GenomeId))
			{
				Inputs.Sex = Genome->Sex;
			}
		}
	}

	return Inputs;
}

const FGenesisVoiceProfile& UGenesisVoiceSubsystem::GetProfile(const FGuid& EntityId)
{
	if (EntityId == GetMotherId())
	{
		return GetMotherVoice();
	}

	if (const FGenesisVoiceProfile* Existing = Profiles.Find(EntityId))
	{
		return *Existing;
	}

	FGenesisVoiceInputs Inputs;
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	if (const FGenesisBodyState* BodyState = Body && EntityId.IsValid() ? Body->FindBody(EntityId) : nullptr)
	{
		Inputs = MakeInputsFromBody(*BodyState);
	}
	else
	{
		// Ohne Körper gibt es keine Stimme – für Befehle und Platzhalter bleibt ein neutrales Profil
		Inputs.IndividualSeed = GenesisHash::FromGuid(EntityId);
	}

	return Profiles.Add(EntityId, BuildProfile(Inputs));
}

const FGenesisVoiceProfile& UGenesisVoiceSubsystem::GetMotherVoice()
{
	if (!bMotherVoiceBuilt)
	{
		// PLATZHALTER: Die Mutter ist noch keine Person der Simulation. Sobald sie eine ist,
		// kommt ihre Stimme aus ihrem Körper wie bei jedem anderen Menschen auch.
		FGenesisVoiceInputs Inputs;
		Inputs.Sex = EGenesisBiologicalSex::Female;
		Inputs.AgeYears = 30.0f;
		Inputs.HeightCm = 168.0f;
		Inputs.Exhaustion = 0.45f;
		Inputs.Arousal = 0.2f;
		Inputs.IndividualSeed = GenesisHash::FromGuid(GetMotherId());
		MotherVoice = BuildProfile(Inputs);
		bMotherVoiceBuilt = true;
	}
	return MotherVoice;
}

void UGenesisVoiceSubsystem::Say(const FGuid& Speaker, const FGenesisUtterance& Utterance)
{
	const FGenesisVoiceProfile& Profile = GetProfile(Speaker);
	if (!GenesisVoiceLogic::CanMake(Utterance.Type, Profile.AgeYears))
	{
		// Kein Fehler, sondern Entwicklung: Ein Säugling kann diesen Laut noch nicht bilden
		UE_LOG(LogGenesis, Verbose, TEXT("Stimme: %s kann mit %.1f Jahren noch nicht %s."),
			*Speaker.ToString(EGuidFormats::Short), Profile.AgeYears, *GenesisVoiceLogic::GetUtteranceName(Utterance.Type));
		return;
	}

	LastUtterance = FString::Printf(TEXT("%s (%s, Stärke %.2f)"),
		*GenesisVoiceLogic::GetUtteranceName(Utterance.Type), *GenesisVoiceLogic::DescribeVoice(Profile), Utterance.Intensity);

	OnUtterance.Broadcast(Speaker, Utterance, Profile);
}

void UGenesisVoiceSubsystem::InvalidateProfile(const FGuid& EntityId)
{
	Profiles.Remove(EntityId);
}

void UGenesisVoiceSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;

	TWeakObjectPtr<UGenesisVoiceSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Voice"),
		TEXT("Stimmen"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			UGenesisVoiceSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FGenesisVoiceProfile& Mother = Self->GetMotherVoice();
			OutLines.Add(FString::Printf(TEXT("Mutter (Platzhalter): %s | Kraft %.2f | Tempo %.2f"),
				*GenesisVoiceLogic::DescribeVoice(Mother), Mother.Strength, Mother.TempoScale));

			for (const TPair<FGuid, FGenesisVoiceProfile>& Pair : Self->Profiles)
			{
				const FGenesisVoiceProfile& Profile = Pair.Value;
				OutLines.Add(FString::Printf(TEXT("%s: %.1f Jahre, %s | Behauchtheit %.2f, Rauigkeit %.2f"),
					*Pair.Key.ToString(EGuidFormats::Short), Profile.AgeYears,
					*GenesisVoiceLogic::DescribeVoice(Profile), Profile.Breathiness, Profile.Roughness));
			}

			if (!Self->LastUtterance.IsEmpty())
			{
				OutLines.Add(FString::Printf(TEXT("Zuletzt: %s"), *Self->LastUtterance));
			}
		}
	});
#endif
}
