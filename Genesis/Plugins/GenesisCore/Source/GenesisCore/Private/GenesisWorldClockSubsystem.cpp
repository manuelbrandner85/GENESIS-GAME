// GENESIS: Der Kreislauf des Lebens

#include "GenesisWorldClockSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisDebug.h"
#include "GenesisLog.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING
namespace
{
	UGenesisWorldClockSubsystem* GetClock(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisClockSkipCommand(
		TEXT("genesis.Clock.SkipDays"),
		TEXT("Springt die Weltzeit um N Tage vorwärts (Zeitsprung). Beispiel: genesis.Clock.SkipDays 365"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisWorldClockSubsystem* Clock = GetClock(World))
			{
				const double Days = Args.Num() > 0 ? FCString::Atod(*Args[0]) : 1.0;
				Clock->SkipTime(FGenesisTimestamp::DaysToSeconds(Days));
				UE_LOG(LogGenesis, Display, TEXT("Weltzeit: %s"), *Clock->GetNow().ToString());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisClockScaleCommand(
		TEXT("genesis.Clock.TimeScale"),
		TEXT("Setzt Weltsekunden pro Echtzeitsekunde. Beispiel: genesis.Clock.TimeScale 3600"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisWorldClockSubsystem* Clock = GetClock(World))
			{
				if (Args.Num() > 0)
				{
					Clock->SetTimeScale(FCString::Atod(*Args[0]));
				}
				UE_LOG(LogGenesis, Display, TEXT("TimeScale: %.1f"), Clock->GetTimeScale());
			}
		}));
}
#endif

void UGenesisWorldClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	ResetState();
	bInitialized = true;

#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<UGenesisWorldClockSubsystem> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Clock"),
		TEXT("Weltzeit"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			if (const UGenesisWorldClockSubsystem* Self = WeakThis.Get())
			{
				OutLines.Add(FString::Printf(TEXT("%s | TimeScale %.1f | Schritt %lld s | %s"),
					*Self->GetNow().ToString(), Self->GetTimeScale(), Self->GetStepSeconds(), Self->IsPaused() ? TEXT("pausiert") : TEXT("läuft")));
			}
		}
	});
#endif
}

void UGenesisWorldClockSubsystem::Deinitialize()
{
	bInitialized = false;

#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Clock"));
#endif

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGenesisPersistenceRegistry* Registry = GameInstance->GetSubsystem<UGenesisPersistenceRegistry>())
		{
			Registry->UnregisterSystem(this);
		}
	}

	OnSimulationStep.Clear();
	Super::Deinitialize();
}

void UGenesisWorldClockSubsystem::Tick(float DeltaTime)
{
	if (State.TimeScale <= 0.0 || DeltaTime <= 0.0f)
	{
		return;
	}

	AccumulatedWorldSeconds += static_cast<double>(DeltaTime) * State.TimeScale;

	int32 StepsThisFrame = 0;
	while (AccumulatedWorldSeconds >= static_cast<double>(StepSeconds) && StepsThisFrame < MaxStepsPerFrame)
	{
		AccumulatedWorldSeconds -= static_cast<double>(StepSeconds);
		EmitStep(StepSeconds, /*bIsTimeSkip*/ false);
		++StepsThisFrame;
	}

	// Überhang (sehr hoher TimeScale oder Framedrop): als ein großer, regulärer Schritt nachholen,
	// damit die Uhr nie hinterherläuft und keine Frame-Spirale entsteht.
	if (AccumulatedWorldSeconds >= static_cast<double>(StepSeconds))
	{
		const int64 CatchUpSteps = static_cast<int64>(AccumulatedWorldSeconds / static_cast<double>(StepSeconds));
		const int64 CatchUpSeconds = CatchUpSteps * StepSeconds;
		AccumulatedWorldSeconds -= static_cast<double>(CatchUpSeconds);
		EmitStep(CatchUpSeconds, /*bIsTimeSkip*/ false);
	}
}

ETickableTickType UGenesisWorldClockSubsystem::GetTickableTickType() const
{
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UGenesisWorldClockSubsystem::IsTickable() const
{
	return bInitialized && !bPaused;
}

UWorld* UGenesisWorldClockSubsystem::GetTickableGameObjectWorld() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetWorld() : nullptr;
}

TStatId UGenesisWorldClockSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGenesisWorldClockSubsystem, STATGROUP_Tickables);
}

bool UGenesisWorldClockSubsystem::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisWorldClockSubsystem::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisWorldClockState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}

	State = Loaded;
	AccumulatedWorldSeconds = 0.0;
	return true;
}

void UGenesisWorldClockSubsystem::ResetState()
{
	State = FGenesisWorldClockState();
	AccumulatedWorldSeconds = 0.0;
}

void UGenesisWorldClockSubsystem::SetNow(const FGenesisTimestamp& NewNow)
{
	State.Now = NewNow;
	AccumulatedWorldSeconds = 0.0;
}

void UGenesisWorldClockSubsystem::SetTimeScale(double NewTimeScale)
{
	State.TimeScale = FMath::Max(0.0, NewTimeScale);
}

void UGenesisWorldClockSubsystem::SkipTime(int64 DeltaSeconds)
{
	if (DeltaSeconds <= 0)
	{
		return;
	}
	EmitStep(DeltaSeconds, /*bIsTimeSkip*/ true);
}

void UGenesisWorldClockSubsystem::AdvanceImmediately(int64 DeltaSeconds)
{
	int64 Remaining = DeltaSeconds;
	while (Remaining > 0)
	{
		const int64 Step = FMath::Min(Remaining, StepSeconds);
		EmitStep(Step, /*bIsTimeSkip*/ false);
		Remaining -= Step;
	}
}

void UGenesisWorldClockSubsystem::SetStepSeconds(int64 NewStepSeconds)
{
	StepSeconds = FMath::Max<int64>(1, NewStepSeconds);
}

void UGenesisWorldClockSubsystem::EmitStep(int64 DeltaSeconds, bool bIsTimeSkip)
{
	FGenesisSimulationStep Step;
	Step.From = State.Now;
	Step.To = State.Now + DeltaSeconds;
	Step.bIsTimeSkip = bIsTimeSkip;

	// Zeit zuerst setzen: Listener sehen GetNow() == Step.To
	State.Now = Step.To;

	OnSimulationStep.Broadcast(Step);
}
