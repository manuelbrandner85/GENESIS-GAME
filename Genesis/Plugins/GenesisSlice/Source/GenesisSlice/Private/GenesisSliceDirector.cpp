// GENESIS: Der Kreislauf des Lebens

#include "GenesisSliceDirector.h"
#include "GenesisFrontendSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GenesisBirthSubsystem.h"
#include "GenesisBirthTypes.h"
#include "GenesisBodyLogic.h"
#include "GenesisBodySubsystem.h"
#include "GenesisBodyTypes.h"
#include "GenesisConceptionSubsystem.h"
#include "GenesisSpermSwarm.h"
#include "EngineUtils.h"
#include "GenesisDebug.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisEmbryoSubsystem.h"
#include "GenesisEmbryoTypes.h"
#include "GenesisLog.h"
#include "GenesisRandom.h"
#include "GenesisSliceLogic.h"
#include "GenesisWorldClockSubsystem.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

namespace
{
	UGenesisSliceDirector* GetDirector(UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UGenesisSliceDirector>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GenesisSliceStartCommand(
		TEXT("genesis.Slice.Start"),
		TEXT("Startet einen Durchlauf von der Befruchtung bis zur ersten Stunde. Optional: Seed."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UGenesisSliceDirector* Director = GetDirector(World))
			{
				Director->StartRun(Args.Num() > 0 ? FCString::Strtoui64(*Args[0], nullptr, 10) : 0);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GenesisSliceAbortCommand(
		TEXT("genesis.Slice.Abort"),
		TEXT("Bricht den laufenden Durchlauf ab."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			if (UGenesisSliceDirector* Director = GetDirector(World))
			{
				Director->AbortRun();
			}
		}));
}
#endif

void UGenesisSliceDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGenesisPersistenceRegistry* Registry = Collection.InitializeDependency<UGenesisPersistenceRegistry>())
	{
		Registry->RegisterSystem(this);
	}

	// Der Rahmen (Startbildschirm, Kapitelkarte) sagt, wann ein Leben beginnt und wann es zurück ins
	// Menü geht. Die Regie hängt sich hier ein und nicht im Spielmodus: Der Spielmodus wird bei jedem
	// Ortswechsel neu gebaut, die Regie nicht – so gibt es jede Verbindung genau einmal.
	if (UGenesisFrontendSubsystem* Frontend = Collection.InitializeDependency<UGenesisFrontendSubsystem>())
	{
		Frontend->OnStartRequested.AddWeakLambda(this, [this]()
		{
			RaceAttempts = 0;
			RaceLostSeconds = 0.0f;
			bForceTravel = true;
			StartRun(0);
		});
		Frontend->OnReturnToMenuRequested.AddWeakLambda(this, [this]()
		{
			ReturnToMenu();
		});
	}

	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UGenesisSliceDirector::Tick));
	RegisterDebugPage();
}

void UGenesisSliceDirector::ReturnToMenu()
{
	State = FGenesisSliceState();
	bCareGiven = false;
	bEndReported = false;
	bUterusTravelTried = false;
	EndedSeconds = 0.0f;
	bForceTravel = true;
	TravelTo(Tuning.ConceptionMap);
}

void UGenesisSliceDirector::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
#if !UE_BUILD_SHIPPING
	GenesisDebug::UnregisterPage(TEXT("Slice"));
#endif
	Super::Deinitialize();
}

bool UGenesisSliceDirector::SaveState(TArray<uint8>& OutPayload) const
{
	return GenesisPersistence::Write(State, OutPayload);
}

bool UGenesisSliceDirector::LoadState(const TArray<uint8>& Payload, int32 SavedSchemaVersion)
{
	FGenesisSliceState Loaded;
	if (!GenesisPersistence::Read(Loaded, Payload))
	{
		return false;
	}
	State = MoveTemp(Loaded);
	return true;
}

void UGenesisSliceDirector::ResetState()
{
	State = FGenesisSliceState();
}

void UGenesisSliceDirector::StartRun(uint64 Seed)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;

	State = FGenesisSliceState();
	// Ohne Seed: aus der Uhrzeit, damit zwei Durchläufe nicht gleich ausgehen
	State.RunSeed = Seed != 0 ? Seed : GenesisHash::Mix64(static_cast<uint64>(FDateTime::UtcNow().GetTicks()));
	State.StartTime = Clock ? Clock->GetNow() : FGenesisTimestamp();
	bCareGiven = false;
	bUterusTravelTried = false;
	bEndReported = false;
	EndedSeconds = 0.0f;

	UE_LOG(LogGenesis, Display, TEXT("Durchlauf: beginnt (Seed %llu)."), State.RunSeed);
	EnterPhase(EGenesisSlicePhase::Conception, EGenesisSliceEnding::None);
}

void UGenesisSliceDirector::AbortRun()
{
	if (!State.IsRunning())
	{
		return;
	}
	EnterPhase(EGenesisSlicePhase::Ended, EGenesisSliceEnding::Aborted);
}

FGenesisSliceSignals UGenesisSliceDirector::ReadSignals() const
{
	FGenesisSliceSignals Signals;

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return Signals;
	}

	if (const UGenesisConceptionSubsystem* Conception = GameInstance->GetSubsystem<UGenesisConceptionSubsystem>())
	{
		Signals.bConceived = Conception->HasConceived();
	}

	if (const UGenesisEmbryoSubsystem* Embryo = GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>())
	{
		const FGenesisEmbryoState& EmbryoState = Embryo->GetState();
		Signals.bImplanted = EmbryoState.Stage == EGenesisEmbryoStage::Implanted;
		Signals.bEmbryoArrested = EmbryoState.Stage == EGenesisEmbryoStage::Arrested;
	}

	const UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>();
	if (const UGenesisBodySubsystem* Body = GameInstance->GetSubsystem<UGenesisBodySubsystem>())
	{
		if (const FGenesisBodyState* BodyState = State.EntityId.IsValid() ? Body->FindBody(State.EntityId) : nullptr)
		{
			Signals.bAlive = BodyState->bAlive;
			Signals.bBorn = BodyState->bBorn;
			Signals.GestationalWeeks = BodyState->bBorn
				? BodyState->GestationalWeeksAtBirth
				: static_cast<float>(GenesisBodyLogic::GetGestationalWeeks(*BodyState, Clock ? Clock->GetNow() : FGenesisTimestamp()));
		}
	}

	if (const UGenesisBirthSubsystem* Birth = GameInstance->GetSubsystem<UGenesisBirthSubsystem>())
	{
		Signals.bLaborRunning = Birth->HasLabor() && !Birth->GetState().IsBorn();
		Signals.bBorn = Signals.bBorn || (Birth->HasLabor() && Birth->GetState().IsBorn());
	}

	if (const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>())
	{
		if (EarlyLife->HasNewborn())
		{
			const FGenesisNewbornState& Newborn = EarlyLife->GetState();
			Signals.MinutesSinceBirth = static_cast<float>(Newborn.MinutesSinceBirth);
			Signals.bAsleep = Newborn.Stage == EGenesisNewbornStage::FirstSleep;
		}
	}

	return Signals;
}

bool UGenesisSliceDirector::Tick(float DeltaSeconds)
{
	// Der Durchlauf startet nicht mehr von selbst: Seit GENESIS-031 gibt es einen Startbildschirm,
	// und der Spieler entscheidet dort, wann sein Leben beginnt. Der Spielmodus hängt sich in
	// `OnStartRequested` ein. Im Editor und in den Messläufen startet weiterhin `genesis.Slice.Start`.
	if (!State.IsRunning())
	{
		// Ein Leben ist zu Ende: Nicht im selben Augenblick abblenden – der letzte Moment (das Kind
		// schläft ein) soll noch ein paar Sekunden stehen dürfen. Dann Abspann, dann Menü.
		const bool bOver = State.Phase == EGenesisSlicePhase::Complete || State.Phase == EGenesisSlicePhase::Ended;
		if (bOver && !bEndReported)
		{
			EndedSeconds += DeltaSeconds;
			if (EndedSeconds >= Tuning.EndingHoldSeconds)
			{
				bEndReported = true;
				if (UGenesisFrontendSubsystem* Frontend = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGenesisFrontendSubsystem>() : nullptr)
				{
					Frontend->EndLife();
				}
			}
		}
		return true;
	}

	State.RealSeconds += DeltaSeconds;
	State.PhaseRealSeconds += DeltaSeconds;

	// Erst tun, was die Phase verlangt, dann prüfen, ob sie zu Ende ist –
	// sonst würde jede Phase eine Runde zu spät weiterschalten.
	DrivePhase(DeltaSeconds);

	EGenesisSliceEnding Ending = EGenesisSliceEnding::None;
	const EGenesisSlicePhase Next = GenesisSliceLogic::NextPhase(State.Phase, ReadSignals(), Tuning, Ending);
	if (Next != State.Phase)
	{
		EnterPhase(Next, Ending);
	}

	return true;
}

void UGenesisSliceDirector::DrivePhase(float DeltaSeconds)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	switch (State.Phase)
	{
	case EGenesisSlicePhase::Conception:
	{
		// Der Schwarm läuft im Editor in Zeitlupe (Hochgeschwindigkeitsaufnahme). Für einen Durchlauf,
		// den jemand spielt, gilt die echte Geschwindigkeit: Die Zellen schwimmen mit 30–55 µm/s,
		// und die Befruchtung ist nach gut zwanzig Sekunden geschehen statt nach vier Minuten.
		for (TActorIterator<AGenesisSpermSwarm> It(GameInstance->GetWorld()); It; ++It)
		{
			It->TimeScale = Tuning.ConceptionTimeScale;

			// Das Wettrennen: Der Spieler führt eine Zelle (GENESIS-037). Erst jetzt, nicht im Vorspann –
			// und erst, wenn der Ortswechsel durch ist und der Schwarm im frischen Level steht.
			if (Tuning.bConceptionRace && !bTravelPending && !It->IsRacing() && It->GetRaceOutcome() == EGenesisRaceOutcome::None)
			{
				It->StartRace(State.RunSeed);
			}

			// Eine andere war schneller: Dieses Leben beginnt nicht. Einen Moment stehen lassen – man soll
			// sehen, wie sie verschmilzt –, dann ein neues Rennen mit einem neuen Feld.
			if (It->GetRaceOutcome() == EGenesisRaceOutcome::Lost)
			{
				RaceLostSeconds += DeltaSeconds;
				if (RaceLostSeconds >= Tuning.RaceLostHoldSeconds && !bTravelPending)
				{
					RaceLostSeconds = 0.0f;
					++RaceAttempts;
					UE_LOG(LogGenesis, Display, TEXT("Durchlauf: Das Rennen beginnt neu (Versuch %d)."), RaceAttempts + 1);
					bForceTravel = true;
					StartRun(0);
					return;
				}
			}
		}
		break;
	}

	case EGenesisSlicePhase::Embryo:
	{
		// Die erste Woche als Zeitraffer wie im EmbryoScope: gleichmäßig, das Tempo nach der Stufe.
		// Sekundenbruchteile sammeln sich, bis eine ganze Sekunde Keimzeit übersprungen werden kann.
		const UGenesisEmbryoSubsystem* EmbryoSystem = GameInstance->GetSubsystem<UGenesisEmbryoSubsystem>();
		const EGenesisEmbryoStage EmbryoStage = EmbryoSystem && EmbryoSystem->HasEmbryo() ? EmbryoSystem->GetState().Stage : EGenesisEmbryoStage::Zygote;
		// Geschlüpft: Der Ort wechselt in die Gebärmutter, weil der Keim dort ist
		const FName EmbryoMap = GenesisSliceLogic::MapForEmbryoStage(EmbryoStage, Tuning);
		const UWorld* World = GameInstance->GetWorld();
		if (!bTravelPending && !bUterusTravelTried && EmbryoMap == Tuning.ImplantationMap && World && !World->GetMapName().Contains(EmbryoMap.ToString()))
		{
			bUterusTravelTried = true;
			FadeOut(0.6f);
			TravelTo(EmbryoMap);
			break;
		}
		const float HoursPerSecond = GenesisSliceLogic::EmbryoHoursPerSecond(EmbryoStage, Tuning);
		GestationStepTimer += DeltaSeconds * HoursPerSecond * 3600.0f;
		const int64 Whole = static_cast<int64>(GestationStepTimer);
		if (Whole > 0)
		{
			GestationStepTimer -= static_cast<float>(Whole);
			if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
			{
				Clock->SkipTime(Whole);
			}
		}
		break;
	}

	case EGenesisSlicePhase::Gestation:
	{
		// Neun Monate, in denen es für das Kind nichts zu sehen gibt: Die Regie springt in Wochen
		// vorwärts, damit die Körpersimulation jeden Tag tatsächlich rechnet.
		GestationStepTimer += DeltaSeconds;
		if (GestationStepTimer < Tuning.GestationStepSeconds)
		{
			break;
		}
		GestationStepTimer = 0.0f;

		if (UGenesisWorldClockSubsystem* Clock = GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>())
		{
			Clock->SkipTime(static_cast<int64>(Tuning.GestationSkipDays) * 24 * 60 * 60);
		}
		break;
	}

	case EGenesisSlicePhase::Birth:
	{
		if (UGenesisBirthSubsystem* Birth = GameInstance->GetSubsystem<UGenesisBirthSubsystem>())
		{
			if (!Birth->HasLabor() && !bTravelPending)
			{
				UE_LOG(LogGenesis, Display, TEXT("Durchlauf: Die Wehen beginnen."));
				Birth->BeginLabor(State.EntityId);
			}
			// Die Zeit dehnt sich zum Höhepunkt hin: Stunden der Eröffnung im Zeitraffer, die Presswehen fast in Echtzeit
			const EGenesisLaborStage LaborStage = Birth->GetState().Stage;
			Birth->LaborTimeScale = LaborStage == EGenesisLaborStage::Pushing ? Tuning.PushingTimeScale
				: (LaborStage == EGenesisLaborStage::Transition ? Tuning.TransitionTimeScale : Tuning.BirthTimeScale);
		}
		break;
	}

	case EGenesisSlicePhase::FirstHour:
	{
		if (UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>())
		{
			EarlyLife->TimeScale = Tuning.FirstHourTimeScale;

			// Die Hebamme legt das Kind auf die Haut der Mutter. Das ist die übliche Versorgung –
			// aber **wer ruft, wird früher geholt**. Ein Kind, das schreit, bekommt schneller Hilfe
			// als eines, das still daliegt; das ist der einzige Hebel, den ein Neugeborenes hat.
			const float Called = EarlyLife->HasNewborn() ? EarlyLife->GetState().CalledMinutes : 0.0f;
			const float CareAfter = FMath::Max(0.2f, Tuning.SkinContactAfterMinutes - Called * Tuning.CallShortensCare);

			if (!bCareGiven && EarlyLife->HasNewborn()
				&& EarlyLife->GetState().MinutesSinceBirth >= CareAfter)
			{
				bCareGiven = true;
				EarlyLife->SetSkinToSkin(true);
				EarlyLife->SetMotherSpeaking(true);
				UE_LOG(LogGenesis, Display, TEXT("Durchlauf: Das Kind kommt auf die Haut der Mutter."));
			}
		}
		break;
	}

	default:
		break;
	}
}

void UGenesisSliceDirector::EnterPhase(EGenesisSlicePhase NewPhase, EGenesisSliceEnding Ending)
{
	const EGenesisSlicePhase Previous = State.Phase;
	State.Phase = NewPhase;
	State.Ending = Ending;
	State.PhaseRealSeconds = 0.0f;
	GestationStepTimer = 0.0f;

	UGameInstance* GameInstance = GetGameInstance();

	// Die Kennung des Kindes steht fest, sobald es gezeugt ist
	if (!State.EntityId.IsValid() && GameInstance)
	{
		if (const UGenesisConceptionSubsystem* Conception = GameInstance->GetSubsystem<UGenesisConceptionSubsystem>())
		{
			if (Conception->HasConceived())
			{
				State.EntityId = Conception->GetRecord().EntityId;
			}
		}
	}

	UE_LOG(LogGenesis, Display, TEXT("Durchlauf: %s → %s (%.1f s) %s"),
		*GenesisSliceLogic::GetPhaseName(Previous), *GenesisSliceLogic::GetPhaseName(NewPhase),
		State.RealSeconds, *GenesisSliceLogic::GetPhaseDescription(NewPhase));

	// Die Weltuhr je Phase: In den Szenen, die man wirklich erlebt, läuft sie in Echtzeit.
	// Sonst liefen zwei Zeitraffer übereinander – die Uhr und die Phase selbst –, und die erste
	// Stunde wäre in sieben Sekunden vorbei, ohne dass irgendetwas davon zu sehen war.
	if (UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr)
	{
		const bool bScene = NewPhase == EGenesisSlicePhase::Birth || NewPhase == EGenesisSlicePhase::FirstHour
			|| NewPhase == EGenesisSlicePhase::Conception;
		Clock->SetTimeScale(bScene ? Tuning.SceneClockTimeScale : Tuning.DefaultClockTimeScale);
	}

	switch (NewPhase)
	{
	case EGenesisSlicePhase::Conception:
		TravelTo(Tuning.ConceptionMap);
		break;

	case EGenesisSlicePhase::Birth:
		// Der Ort wechselt, weil das Kind ihn wechselt
		FadeOut(0.6f);
		TravelTo(Tuning.BirthMap);
		break;

	case EGenesisSlicePhase::Complete:
	case EGenesisSlicePhase::Ended:
		UE_LOG(LogGenesis, Display, TEXT("Durchlauf: %s nach %.1f s."),
			*GenesisSliceLogic::GetEndingName(Ending), State.RealSeconds);
		break;

	default:
		break;
	}

	OnPhaseChanged.Broadcast(State, Previous);
}

void UGenesisSliceDirector::TravelTo(FName MapName)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || MapName.IsNone())
	{
		return;
	}

	// Schon da? Dann ist nichts zu tun – ein unnötiger Ladevorgang würde die Simulation nur anhalten.
	// Außer, ein frischer Ort ist ausdrücklich verlangt (Lebensbeginn aus dem Menü, Rückkehr ins Menü).
	const bool bForce = bForceTravel;
	bForceTravel = false;
	if (!bForce && World->GetMapName().Contains(MapName.ToString()))
	{
		bTravelPending = false;
		return;
	}

	bTravelPending = true;
	UE_LOG(LogGenesis, Display, TEXT("Durchlauf: Ortswechsel nach %s."), *MapName.ToString());
	UGameplayStatics::OpenLevel(World, MapName);
	bTravelPending = false;
}

void UGenesisSliceDirector::FadeOut(float Seconds)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->PlayerCameraManager)
	{
		// Zwischen zwei Orten liegt Dunkelheit – kein Schnitt mitten ins Bild
		Controller->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, Seconds, FLinearColor::Black, false, true);
	}
}

void UGenesisSliceDirector::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	if (bDebugPageRegistered)
	{
		return;
	}
	bDebugPageRegistered = true;

	TWeakObjectPtr<UGenesisSliceDirector> WeakThis(this);
	GenesisDebug::RegisterPage({
		TEXT("Slice"),
		TEXT("Durchlauf"),
		[WeakThis](const UWorld*, TArray<FString>& OutLines)
		{
			const UGenesisSliceDirector* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}

			const FGenesisSliceState& State = Self->GetState();
			if (State.Phase == EGenesisSlicePhase::Idle)
			{
				OutLines.Add(TEXT("Kein Durchlauf (genesis.Slice.Start)."));
				return;
			}

			const FGenesisSliceSignals Signals = Self->ReadSignals();
			OutLines.Add(FString::Printf(TEXT("%s – %s"),
				*GenesisSliceLogic::GetPhaseName(State.Phase), *GenesisSliceLogic::GetPhaseDescription(State.Phase)));
			OutLines.Add(FString::Printf(TEXT("Seit Beginn %.0f s, in dieser Phase %.0f s | Seed %llu"),
				State.RealSeconds, State.PhaseRealSeconds, State.RunSeed));
			OutLines.Add(FString::Printf(TEXT("gezeugt %s | eingenistet %s | Woche %.1f | geboren %s | %.0f min alt"),
				Signals.bConceived ? TEXT("ja") : TEXT("nein"),
				Signals.bImplanted ? TEXT("ja") : TEXT("nein"),
				Signals.GestationalWeeks,
				Signals.bBorn ? TEXT("ja") : TEXT("nein"),
				Signals.MinutesSinceBirth));

			if (State.Ending != EGenesisSliceEnding::None)
			{
				OutLines.Add(FString::Printf(TEXT("Ende: %s"), *GenesisSliceLogic::GetEndingName(State.Ending)));
			}
		}
	});
#endif
}
