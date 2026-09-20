// GENESIS: Der Kreislauf des Lebens

#include "GenesisWorldSoundActor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBirthSubsystem.h"
#include "EngineUtils.h"
#include "GenesisBirthTypes.h"
#include "GenesisDebug.h"
#include "GenesisWorldSoundComponent.h"
#include "GenesisWorldSoundExport.h"

AGenesisWorldSoundActor::AGenesisWorldSoundActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WorldSound = CreateDefaultSubobject<UGenesisWorldSoundComponent>(TEXT("WorldSound"));
	WorldSound->SetupAttachment(Root);
	// Ein Ort umgibt den Hörer – er kommt nicht aus einer Richtung
	WorldSound->bAllowSpatialization = false;
}

void AGenesisWorldSoundActor::BeginPlay()
{
	Super::BeginPlay();

	if (WorldSound)
	{
		// Der Mutterleib wird von innen gehört, alles andere von außen durch die Ohren des Kindes
		WorldSound->bHeardFromOutside = Params.Place != EGenesisPlace::Womb;
		WorldSound->SetWorldSoundParams(Params);
	}

	RegisterDebugPage();
}

void AGenesisWorldSoundActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFollowBirth || !WorldSound)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBirthSubsystem* Birth = GameInstance ? GameInstance->GetSubsystem<UGenesisBirthSubsystem>() : nullptr;
	if (!Birth || !Birth->HasLabor())
	{
		return;
	}

	const FGenesisBirthState& State = Birth->GetState();

	FGenesisWorldSoundParams Updated = Params;

	// Der Puls der Mutter steigt mit der Anstrengung: In der Eröffnungsphase um 85, unter der
	// Austreibung über 120. Genau dieser Wert taktet den Monitor und das Rauschen des Mutterkuchens.
	const float Effort = FMath::Clamp(State.DilationCm / 10.0f, 0.0f, 1.0f);
	Updated.MaternalHeartRateBpm = FMath::Lerp(82.0f, 126.0f, Effort) + 8.0f * State.ContractionIntensity;

	// Und im Raum geschieht mehr, je näher die Geburt kommt – danach wird es ruhig
	Updated.Activity = FMath::Clamp(0.15f + 0.85f * Effort, 0.0f, 1.0f);

	if (State.IsBorn())
	{
		// Nach der Geburt kommt die Mutter herunter: Der Puls fällt in wenigen Minuten
		// zurück Richtung Ruhe, und im Raum wird es leiser.
		const float Settle = FMath::Clamp(State.SecondsSinceBirth / 300.0f, 0.0f, 1.0f);
		Updated.MaternalHeartRateBpm = FMath::Lerp(Updated.MaternalHeartRateBpm, 88.0f, Settle);
		Updated.Activity = FMath::Lerp(0.75f, 0.35f, Settle);
	}

	Params = Updated;
	WorldSound->SetWorldSoundParams(Updated);
}

void AGenesisWorldSoundActor::RegisterDebugPage()
{
#if !UE_BUILD_SHIPPING
	// Die Seite gehört dem Ort, nicht diesem einen Actor – sie listet alle Orte der Szene auf
	GenesisDebug::RegisterPage({
		TEXT("Place"),
		TEXT("Klang der Orte"),
		[](const UWorld* World, TArray<FString>& OutLines)
		{
			// Jeder Ort trägt sich selbst ein – so steht im HUD, was gerade tatsächlich klingt
			for (TActorIterator<AGenesisWorldSoundActor> It(const_cast<UWorld*>(World)); It; ++It)
			{
				const AGenesisWorldSoundActor* Actor = *It;
				if (!Actor)
				{
					continue;
				}
				const FGenesisWorldSoundParams& Params = Actor->Params;
				OutLines.Add(FString::Printf(TEXT("%s: Puls der Mutter %.0f/min | Betrieb %.2f | Verdauung %.2f | %s"),
					*GenesisWorldSoundExport::GetPlaceName(Params.Place), Params.MaternalHeartRateBpm,
					Params.Activity, Params.Digestion,
					Actor->WorldSound && Actor->WorldSound->bHeardFromOutside ? TEXT("von außen gehört") : TEXT("von innen")));
			}

			if (OutLines.Num() == 0)
			{
				OutLines.Add(TEXT("Kein Ort in dieser Szene."));
			}
		}
	});
#endif
}
