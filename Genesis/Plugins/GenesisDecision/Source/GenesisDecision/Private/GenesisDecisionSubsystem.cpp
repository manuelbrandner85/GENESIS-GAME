// GENESIS: Der Kreislauf des Lebens

#include "GenesisDecisionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisDecisionConsiderations.h"
#include "GenesisDecisionEngine.h"
#include "GenesisGameplayTags.h"
#include "GenesisLifeSimulationEngine.h"
#include "GenesisLifeSimulationSubsystem.h"
#include "GenesisLog.h"
#include "GenesisMemorySubsystem.h"
#include "GenesisSoulSubsystem.h"
#include "GenesisWorldClockSubsystem.h"
#include "HAL/IConsoleManager.h"

void UGenesisDecisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UGenesisWorldClockSubsystem>();
	Collection.InitializeDependency<UGenesisMemorySubsystem>();
	Collection.InitializeDependency<UGenesisSoulSubsystem>();
	Collection.InitializeDependency<UGenesisLifeSimulationSubsystem>();

	const UGenesisDecisionSettings* Settings = GetDefault<UGenesisDecisionSettings>();
	Engine = NewObject<UGenesisDecisionEngine>(this);
	Engine->SetTuning(Settings->Tuning);
	Engine->AddDefaultConsiderations();
	for (const TObjectPtr<UGenesisDecisionConsideration>& Consideration : Engine->GetConsiderations())
	{
		if (const float* Override = Settings->ConsiderationWeights.Find(Consideration->GetConsiderationName()))
		{
			Consideration->Weight = FMath::Max(0.0f, *Override);
		}
	}

	RegisterDebugPage();
}

void UGenesisDecisionSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Decision"));
#endif
	OnDecisionMade.Clear();
	Super::Deinitialize();
}

FGenesisDecisionInputs UGenesisDecisionSubsystem::MakeInputs(const FGuid& DeciderId) const
{
	FGenesisDecisionInputs Inputs;
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return Inputs;
	}

	if (const UGenesisLifeSimulationSubsystem* Life = GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>())
	{
		Inputs.State = Life->GetEngine() ? &Life->GetEngine()->GetState() : nullptr;
	}
	if (const UGenesisMemorySubsystem* Memory = GameInstance->GetSubsystem<UGenesisMemorySubsystem>())
	{
		Inputs.Memory = &Memory->GetMemoryWorld();
	}
	if (const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
	{
		Inputs.Now = Clock->GetNow();
	}
	if (const UGenesisSoulSubsystem* Soul = GameInstance->GetSubsystem<UGenesisSoulSubsystem>())
	{
		const FGenesisLifeProfile* Profile = Inputs.State ? Inputs.State->FindProfile(DeciderId) : nullptr;
		if (Profile && Profile->SoulId.IsValid())
		{
			Inputs.DeciderSoul = Soul->FindSoul(Profile->SoulId);
		}
	}
	return Inputs;
}

FGenesisDecisionResult UGenesisDecisionSubsystem::EvaluateSituation(const FGenesisDecisionSituation& Situation) const
{
	return Engine ? Engine->Evaluate(Situation, MakeInputs(Situation.DeciderId)) : FGenesisDecisionResult();
}

FGenesisDecisionResult UGenesisDecisionSubsystem::DecideForNpc(const FGenesisDecisionSituation& Situation)
{
	FGenesisDecisionResult Result = EvaluateSituation(Situation);

	const UGameInstance* GameInstance = GetGameInstance();
	UGenesisLifeSimulationSubsystem* Life = GameInstance ? GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>() : nullptr;
	if (!Engine || !Life || !Life->GetEngine())
	{
		return Result;
	}

	// Der Zufallsstrom der Welt macht NPC-Entscheidungen reproduzierbar
	Engine->ChooseForNpc(Result, Life->GetEngine()->GetState().Rng);
	return Execute(Situation, MoveTemp(Result));
}

FGenesisDecisionResult UGenesisDecisionSubsystem::ResolvePlayerChoice(const FGenesisDecisionSituation& Situation, int32 ChosenIndex)
{
	FGenesisDecisionResult Result = EvaluateSituation(Situation);
	UGenesisDecisionEngine::ApplyPlayerChoice(Result, ChosenIndex);
	return Execute(Situation, MoveTemp(Result));
}

FGenesisDecisionResult UGenesisDecisionSubsystem::Execute(const FGenesisDecisionSituation& Situation, FGenesisDecisionResult Result)
{
	const UGameInstance* GameInstance = GetGameInstance();
	UGenesisLifeSimulationSubsystem* Life = GameInstance ? GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>() : nullptr;

	if (Life && Result.ChosenIndex != INDEX_NONE)
	{
		Result.EventId = Life->ReportAction(UGenesisDecisionEngine::BuildAction(Situation, Result));
	}

	LastSituation = Situation;
	LastResult = Result;
	OnDecisionMade.Broadcast(Situation, Result);
	return Result;
}

void UGenesisDecisionSubsystem::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisDecisionSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Decision"),
		TEXT("Decision Engine (letzte Entscheidung)"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisDecisionSubsystem* Self = WeakThis.Get();
			if (!Self || Self->LastResult.Evaluations.Num() == 0)
			{
				OutLines.Add(TEXT("Noch keine Entscheidung."));
				return;
			}

			const FGenesisDecisionResult& Result = Self->LastResult;
			OutLines.Add(FString::Printf(TEXT("Entscheider %s | Gewählt #%d | Impuls #%d | Kopf #%d | Zögern %.2f | Konflikt %.2f | %s"),
				*Self->LastSituation.DeciderId.ToString(EGuidFormats::Short).Left(8), Result.ChosenIndex, Result.ImpulseIndex,
				Result.ConsciousBestIndex, Result.Hesitation, Result.InnerConflict, Result.bFollowedImpulse ? TEXT("dem Impuls gefolgt") : TEXT("gegen den Impuls")));

			for (const FGenesisOptionEvaluation& Evaluation : Result.Evaluations)
			{
				const FGenesisDecisionOption& Option = Self->LastSituation.Options[Evaluation.OptionIndex];
				FString Breakdown;
				for (const FGenesisConsiderationScore& Score : Evaluation.Breakdown)
				{
					Breakdown += FString::Printf(TEXT("%s%s %+.2f  "), Score.bSubconscious ? TEXT("*") : TEXT(""), *Score.Consideration.ToString(), Score.Score);
				}
				OutLines.Add(FString::Printf(TEXT("  #%d %s | p=%.2f Gesamt %+.2f Kopf %+.2f Bauch %+.2f | %s"),
					Evaluation.OptionIndex, *Option.Label.ToString(), Evaluation.Probability, Evaluation.Total, Evaluation.Conscious, Evaluation.Impulse, *Breakdown));
			}
		}
	});
#endif
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisDecisionDilemmaCommand(
		TEXT("genesis.Decision.SimulateDilemma"),
		TEXT("Entwickler: Zwei Personen mit gegensätzlichem Charakter stehen vor derselben Wahl (Wahrheit gestehen oder lügen). Optional: 1 = unter Zeitdruck."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UGenesisDecisionSubsystem* Decision = GameInstance ? GameInstance->GetSubsystem<UGenesisDecisionSubsystem>() : nullptr;
			UGenesisLifeSimulationSubsystem* Life = GameInstance ? GameInstance->GetSubsystem<UGenesisLifeSimulationSubsystem>() : nullptr;
			if (!Decision || !Life || !Life->GetEngine())
			{
				return;
			}

			FGenesisLifeSimulationState& State = Life->GetEngine()->GetState();
			const FGuid Honest = State.Rng.NewGuid();
			const FGuid Deceiver = State.Rng.NewGuid();
			const FGuid Victim = State.Rng.NewGuid();

			for (const FGuid& Id : { Honest, Deceiver, Victim })
			{
				FGenesisLifeProfile Profile;
				Profile.EntityId = Id;
				Profile.SimulationLevel = EGenesisSimulationLevel::Full;
				Life->RegisterEntity(Profile);
			}
			State.FindProfile(Honest)->Karma.Values.Honesty = 70.0f;
			State.FindProfile(Deceiver)->Karma.Values.Honesty = -60.0f;
			State.FindProfile(Deceiver)->Altruism = -0.5f;

			UGenesisLifeActionDefinition* Confess = NewObject<UGenesisLifeActionDefinition>(GetTransientPackage());
			Confess->ActionType = GenesisTags::Theme_Honesty;
			Confess->Themes.AddTag(GenesisTags::Theme_Honesty);
			Confess->ExpressesValues.AddTag(GenesisTags::Theme_Honesty);
			Confess->KarmaImpulse.Honesty = 6.0f;
			Confess->KarmaImpulse.Courage = 3.0f;
			Confess->CostToSelf = 0.5f;
			Confess->TrustImpact = 0.3f;
			Confess->BaseMagnitude = 0.5f;

			UGenesisLifeActionDefinition* Lie = NewObject<UGenesisLifeActionDefinition>(GetTransientPackage());
			Lie->ActionType = GenesisTags::Theme_Deception;
			Lie->Themes.AddTag(GenesisTags::Theme_Deception);
			Lie->ViolatesValues.AddTag(GenesisTags::Theme_Honesty);
			Lie->KarmaImpulse.Honesty = -8.0f;
			Lie->SelfBenefit = 0.6f;
			Lie->TrustImpact = -0.5f;
			Lie->BaseMagnitude = 0.5f;

			const float TimePressure = Args.Num() > 0 && Args[0] == TEXT("1") ? 1.0f : 0.0f;
			for (const FGuid& Decider : { Honest, Deceiver })
			{
				FGenesisDecisionSituation Situation;
				Situation.DeciderId = Decider;
				Situation.TimePressure = TimePressure;
				Situation.Stakes = 0.8f;

				FGenesisDecisionOption ConfessOption;
				ConfessOption.Label = FText::FromString(TEXT("Gestehen"));
				ConfessOption.Action = Confess;
				ConfessOption.TargetIds.Add(Victim);
				Situation.Options.Add(ConfessOption);

				FGenesisDecisionOption LieOption;
				LieOption.Label = FText::FromString(TEXT("Leugnen"));
				LieOption.Action = Lie;
				LieOption.TargetIds.Add(Victim);
				Situation.Options.Add(LieOption);

				const FGenesisDecisionResult Result = Decision->DecideForNpc(Situation);
				UE_LOG(LogGenesis, Display, TEXT("Dilemma: %s wählt '%s' (p=%.2f, Zögern %.2f)"),
					Decider == Honest ? TEXT("Ehrliche Person") : TEXT("Täuschende Person"),
					Result.ChosenIndex != INDEX_NONE ? *Situation.Options[Result.ChosenIndex].Label.ToString() : TEXT("-"),
					Result.ChosenIndex != INDEX_NONE ? Result.Evaluations[Result.ChosenIndex].Probability : 0.0f, Result.Hesitation);
			}
		}));
}
#endif
