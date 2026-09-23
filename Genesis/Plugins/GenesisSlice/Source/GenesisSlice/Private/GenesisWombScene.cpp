// GENESIS: Der Kreislauf des Lebens

#include "GenesisWombScene.h"
#include "CineCameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GenesisAudioCoreLogic.h"
#include "GenesisAudioSubsystem.h"
#include "GenesisBodySubsystem.h"
#include "GenesisFetalLogic.h"
#include "GenesisLog.h"
#include "GenesisSliceDirector.h"
#include "GenesisWorldClockSubsystem.h"
#include "GenesisWorldSoundActor.h"
#include "GenesisWorldSoundComponent.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	/** Entwickler: Woche und Uhrzeit der Prüfansicht setzen – genesis.Womb.Preview <SSW> [Uhrzeit]. */
	FAutoConsoleCommandWithWorldAndArgs GenesisWombPreviewCommand(
		TEXT("genesis.Womb.Preview"),
		TEXT("Mutterleib aus Sicht des Kindes ohne Durchlauf: genesis.Womb.Preview <SSW> [Uhrzeit] [lx im Mutterleib]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<AGenesisWombScene> It(World); It; ++It)
			{
				if (Args.Num() > 0) { It->PreviewWeeks = FCString::Atof(*Args[0]); }
				if (Args.Num() > 1) { It->PreviewHour = FCString::Atof(*Args[1]); }
				if (Args.Num() > 2) { It->PreviewWombLux = FCString::Atof(*Args[2]); }
			}
		}));

}

namespace GenesisWombPerception
{
	float CavityRadiusCm(float Weeks)
	{
		// Stützpunkte (SSW, Innenradius cm) – Näherung, siehe Deklaration
		static const FVector2D Points[] = {
			FVector2D(8.0f, 1.5f), FVector2D(12.0f, 3.5f), FVector2D(16.0f, 5.5f), FVector2D(20.0f, 8.0f), FVector2D(24.0f, 9.5f),
			FVector2D(28.0f, 11.0f), FVector2D(32.0f, 12.5f), FVector2D(36.0f, 13.5f), FVector2D(40.0f, 14.5f) };
		if (Weeks <= Points[0].X) { return Points[0].Y; }
		for (int32 Index = 1; Index < UE_ARRAY_COUNT(Points); ++Index)
		{
			if (Weeks <= Points[Index].X)
			{
				const float Alpha = (Weeks - Points[Index - 1].X) / (Points[Index].X - Points[Index - 1].X);
				return FMath::Lerp(Points[Index - 1].Y, Points[Index].Y, Alpha);
			}
		}
		return Points[UE_ARRAY_COUNT(Points) - 1].Y;
	}

	FGenesisWombPerception Compute(const FGenesisFetalView& Fetal, float WombLux)
	{
		FGenesisWombPerception Result;
		Result.Presence = Fetal.ConsciousAccess;
		Result.EyesOpen = Fetal.EyesOpen;
		// Auch mit offenen Augen sieht ein Fetus nur Helligkeit und diffuse Flächen
		Result.Blur = 1.0f - 0.5f * Fetal.EyesOpen;
		const float Light = WombLux <= 0.05f ? 0.0f
			: FMath::Clamp(FMath::LogX(10.0f, WombLux / 0.05f) / FMath::LogX(10.0f, 50.0f / 0.05f), 0.0f, 1.0f);
		Result.Brightness = Light * Fetal.LightPerception;
		Result.CavityRadiusCm = CavityRadiusCm(Fetal.GestationalWeeks);
		return Result;
	}
}

AGenesisWombScene::AGenesisWombScene()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Cavity = CreateDefaultSubobject<USceneComponent>(TEXT("Cavity"));
	Cavity->SetupAttachment(Root);
	Cavity->SetMobility(EComponentMobility::Movable);

	auto MakePart = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Cavity);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	Wall = MakePart(TEXT("Wall"));
	Placenta = MakePart(TEXT("Placenta"));
	PlacentaVessels = MakePart(TEXT("PlacentaVessels"));
	Cord = MakePart(TEXT("Cord"));
	CordVessels = MakePart(TEXT("CordVessels"));

	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	MotherVoice = CreateDefaultSubobject<UAudioComponent>(TEXT("MotherVoice"));
	MotherVoice->SetupAttachment(Root);
	MotherVoice->bAutoActivate = false;
	MotherVoice->bAllowSpatialization = false;
}

void AGenesisWombScene::BeginPlay()
{
	Super::BeginPlay();
	Random.Initialize(1234);
	// Alles in der Höhle hängt am Licht durch den Bauch: Die Wand leuchtet, das Gewebe davor lässt es durch
	for (UStaticMeshComponent* Part : { Wall.Get(), Placenta.Get(), PlacentaVessels.Get(), Cord.Get(), CordVessels.Get() })
	{
		if (Part && Part->GetMaterial(0))
		{
			LitMaterials.Add(Part->CreateAndSetMaterialInstanceDynamic(0));
		}
	}
	Camera->Filmback.SensorWidth = 36.0f;
	Camera->Filmback.SensorHeight = 20.25f;
	Camera->SetCurrentFocalLength(20.0f);
	Camera->bOverride_CustomNearClippingPlane = true;
	Camera->CustomNearClippingPlane = 0.1f * WorldScale;
	// Die Blende wird mit dem Maßstab kleiner gerechnet (siehe WorldScale)
	Camera->LensSettings.MinFStop = 0.05f;
	Camera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Camera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Camera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.FocusMethod = ECameraFocusMethod::Manual;
	Focus.ManualFocusDistance = 4.0f * WorldScale;       // wenige Zentimeter vor dem Gesicht
	Camera->SetFocusSettings(Focus);
}

void AGenesisWombScene::UpdateTime()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisSliceDirector* Director = GameInstance ? GameInstance->GetSubsystem<UGenesisSliceDirector>() : nullptr;
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	const UGenesisWorldClockSubsystem* Clock = GameInstance ? GameInstance->GetSubsystem<UGenesisWorldClockSubsystem>() : nullptr;
	const FGenesisBodyState* BodyState = Director && Body && Director->GetState().EntityId.IsValid() ? Body->FindBody(Director->GetState().EntityId) : nullptr;
	if (Director && Clock && BodyState && !BodyState->bBorn && Director->GetState().Phase == EGenesisSlicePhase::Gestation)
	{
		const double Hours = static_cast<double>(Clock->GetNow() - BodyState->ConceptionTime) / FGenesisTimestamp::SecondsPerHour;
		Weeks = static_cast<float>(Hours / (7.0 * 24.0) + GenesisFetalLogic::WeeksFromConceptionToGestational);
		HourOfDay = FMath::Fmod(Hours, 24.0);
		DayIndex = FMath::FloorToInt32(Hours / 24.0);
		Seed = static_cast<int32>(Director->GetState().RunSeed & 0x7fffffff);
		const int32 MomentIndex = Director->GetGestationPlanPoint().MomentIndex;
		if (MomentIndex != LastMomentIndex && MomentIndex >= 0)
		{
			// Ein neuer Moment: Sie spricht bald, wenn sie wach ist und spricht (erst ab SSW 16)
			SinceVoice = 17.0f;
			const FGenesisMotherMoment Now = GenesisMotherDay::Evaluate(FGenesisMotherDayTuning(), HourOfDay, DayIndex, Weeks, Seed);
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Moment %d – SSW %.1f, %02d:%02d Uhr, Mutter: %s, %.2f lx im Mutterleib"),
				MomentIndex, Weeks, FMath::FloorToInt32(HourOfDay), FMath::FloorToInt32(FMath::Fmod(HourOfDay, 1.0) * 60.0),
				*GenesisMotherDay::GetActivityName(Now.Activity), Now.WombLux);
		}
		LastMomentIndex = MomentIndex;
		bInRun = true;
		return;
	}
	// Ohne Durchlauf: die Prüfansicht, die Zeit läuft langsam weiter
	bInRun = false;
	Weeks = PreviewWeeks;
	HourOfDay = FMath::Fmod(static_cast<double>(PreviewHour) + SceneSeconds * 8.0 / 3600.0, 24.0);
	DayIndex = 100;
}

void AGenesisWombScene::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SceneSeconds += DeltaSeconds;
	SinceVoice += DeltaSeconds;
	UpdateTime();

	const FGenesisFetalReference& Reference = GenesisFetalLogic::GetReference();
	const FGenesisFetalView Fetal = GenesisFetalLogic::Evaluate(Reference, Weeks);
	Mother = GenesisMotherDay::Evaluate(FGenesisMotherDayTuning(), HourOfDay, DayIndex, Weeks, Seed);
	if (!bInRun && PreviewWombLux >= 0.0f)
	{
		Mother.WombLux = PreviewWombLux;
	}
	Perception = GenesisWombPerception::Compute(Fetal, Mother.WombLux);
	const TArray<EGenesisFetalEvent> Events = GenesisFetalLogic::AdvanceBehaviour(Behaviour, Fetal, Reference.Milestones, DeltaSeconds, Mother.Rocking, Random);

	// Die Höhle wächst mit der Woche (gebaut für 10 cm Innenradius)
	Cavity->SetRelativeScale3D(FVector(Perception.CavityRadiusCm / 10.0f * WorldScale));

	// Licht: Die vordere Wand leuchtet mit dem Licht, das durch den Bauch kommt (nur Rot); Gewebe davor scheint durch
	for (UMaterialInstanceDynamic* Material : LitMaterials)
	{
		Material->SetScalarParameterValue(TEXT("Glow"), Mother.WombLux * GlowPerLux);
		Material->SetVectorParameterValue(TEXT("BellyDirection"), FLinearColor(BellyDirection.GetSafeNormal()));
		Material->SetVectorParameterValue(TEXT("Center"), FLinearColor(GetActorLocation()));
	}

	UpdateCamera(DeltaSeconds, Events);
	UpdateSound(DeltaSeconds);

	// Diese Kamera ist das Kind
	if (APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (Controller->GetViewTarget() != this)
		{
			Controller->SetViewTarget(this);
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawn->SetActorHiddenInGame(true);
			}
		}
	}
}

void AGenesisWombScene::UpdateCamera(float DeltaSeconds, const TArray<EGenesisFetalEvent>& Events)
{
	const float Radius = Perception.CavityRadiusCm;
	// Eigene Bewegungen als Ruck: Schreck stark, Tritt mittel, Schluckauf klein und nach oben
	for (EGenesisFetalEvent Event : Events)
	{
		float Strength = 0.0f;
		FVector Direction = FVector(Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-1.0f, 1.0f), Random.FRandRange(-0.5f, 0.5f));
		switch (Event)
		{
		case EGenesisFetalEvent::Startle: Strength = 0.06f; break;
		case EGenesisFetalEvent::Kick:    Strength = 0.025f; break;
		case EGenesisFetalEvent::Stretch: Strength = 0.03f; break;
		case EGenesisFetalEvent::Hiccup:  Strength = 0.012f; Direction = FVector(0.0, 0.0, 1.0); break;
		case EGenesisFetalEvent::HeadTurn: Strength = 0.02f; break;
		default: break;
		}
		if (Strength > JoltStrength * FMath::Clamp(1.0f - JoltSeconds / 0.6f, 0.0f, 1.0f))
		{
			JoltStrength = Strength;
			JoltDirection = Direction.GetSafeNormal();
			JoltSeconds = 0.0f;
		}
		if (Event == EGenesisFetalEvent::Kick && Weeks >= 20.0f && Mother.Activity != EGenesisMotherActivity::Sleeping
			&& !bKickLineSaid && KickLine)
		{
			// Sie spürt den Tritt und antwortet
			MotherVoice->SetSound(KickLine);
			MotherVoice->Play();
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: Sie spürt den Tritt und spricht (SSW %.1f)."), Weeks);
			bKickLineSaid = true;
			SinceVoice = 0.0f;
		}
	}
	JoltSeconds += DeltaSeconds;
	const float Decay = FMath::Exp(-JoltSeconds / 0.18f) * FMath::Sin(FMath::Min(JoltSeconds / 0.08f, PI));
	const FVector Jolt = JoltDirection * JoltStrength * Radius * Decay;

	// Wiegen, wenn sie geht (Schrittfrequenz ~1,8 Hz), dazu ihr Atem (~0,25 Hz)
	const float Walk = Mother.Rocking * 0.03f * Radius * FMath::Sin(2.0f * PI * 1.8f * SceneSeconds);
	const float Breath = 0.01f * Radius * FMath::Sin(2.0f * PI * 0.25f * SceneSeconds);
	const FVector Head(-0.25f * Radius, 0.0f, 0.1f * Radius + Walk + Breath);
	Camera->SetRelativeLocation((Head + Jolt) * WorldScale);
	const FRotator Look = LookDirection.Rotation() + FRotator(-6.0f + 3.0f * FMath::Sin(0.07f * SceneSeconds), 8.0f * FMath::Sin(0.05f * SceneSeconds), 0.0f);
	Camera->SetRelativeRotation(Look + FRotator(Jolt.Z * 20.0f / FMath::Max(1.0f, Radius), Jolt.Y * 20.0f / FMath::Max(1.0f, Radius), 0.0f));

	// Wahrnehmung: Das Auge im Dunkeln passt sich an die Lichtmenge an (die Wand leuchtet linear mit den lx) –
	// wie hell es sich anfühlt, folgt der Wahrnehmung (logarithmisch, Lider, Pupille). Was nicht ankommt, bleibt
	// dunkel; vor dem bewussten Erleben (Thalamus–Rinde) gedämpft.
	const float Adapted = -FMath::Log2(FMath::Max(0.05f, Mother.WombLux) / ReferenceLux);
	const float Felt = FMath::Log2(FMath::Max(0.01f, Perception.Brightness) / ReferenceBrightness);
	Camera->PostProcessSettings.AutoExposureBias = ExposureBias + Adapted + Felt - 2.0f * (1.0f - Perception.Presence);
	// Eigengrau: Auch ohne Licht ist das Dunkel nicht schwarz, sondern ein schwaches, rauschendes Dunkelrotbraun
	const float Dark = 1.0f - FMath::Clamp(Perception.Brightness / 0.3f, 0.0f, 1.0f);
	Camera->PostProcessSettings.bOverride_ColorOffset = true;
	Camera->PostProcessSettings.ColorOffset = FVector4(0.03f, 0.021f, 0.019f, 0.0f) * Dark * (0.4f + 0.6f * Perception.Presence)
		* (0.85f + 0.15f * FMath::Sin(2.0f * PI * 0.07f * SceneSeconds));
	Camera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
	Camera->PostProcessSettings.FilmGrainIntensity = 0.3f * Dark;
	Camera->PostProcessSettings.bOverride_FilmGrainIntensityShadows = true;
	Camera->PostProcessSettings.FilmGrainIntensityShadows = 1.0f;
	// Unschärfe: Ein Fetus sieht Flächen und große Formen, keine Einzelheiten. Hinter geschlossenen Lidern nur
	// diffuses Leuchten (Fokus dicht vor dem Auge), mit offenen Augen das Grobe wenige Zentimeter vor dem Gesicht.
	FCameraFocusSettings Focus = Camera->FocusSettings;
	Focus.ManualFocusDistance = FMath::Lerp(0.6f, 4.0f, Perception.EyesOpen) * WorldScale;
	Camera->SetFocusSettings(Focus);
	// Gleiche Unschärfe wie im echten Maßstab: Öffnung (Brennweite / Blende) mit dem Maßstab vergrößern
	Camera->SetCurrentAperture(FMath::Lerp(5.6f, 1.4f, Perception.Blur) / FMath::Max(1.0f, WorldScale));
	Camera->PostProcessSettings.bOverride_ColorSaturation = true;
	Camera->PostProcessSettings.ColorSaturation = FVector4(1.0f, 1.0f, 1.0f, FMath::Lerp(0.5f, 0.9f, Perception.EyesOpen));
}

void AGenesisWombScene::UpdateSound(float DeltaSeconds)
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisAudioSubsystem* Audio = GameInstance ? GameInstance->GetSubsystem<UGenesisAudioSubsystem>() : nullptr;
	const FGenesisHearingPerception Hearing = Audio ? Audio->GetHearingPerception() : FGenesisHearingPerception();

	// Der Ort: ihr Herz, ihr Blut, ihr Darm – so, wie das Kind es hört
	for (TActorIterator<AGenesisWorldSoundActor> It(GetWorld()); It; ++It)
	{
		if (It->Params.Place == EGenesisPlace::Womb && It->WorldSound)
		{
			It->WorldSound->bHeardByUnborn = true;
			FGenesisWorldSoundParams Params = It->WorldSound->GetWorldSoundParams();
			Params.MaternalHeartRateBpm = Mother.HeartRateBpm;
			Params.Digestion = Mother.Digestion;
			Params.Activity = FMath::Clamp(0.2f + 0.6f * Mother.Rocking + 0.3f * Mother.Speaking, 0.0f, 1.0f);
			It->WorldSound->SetWorldSoundParams(Params);
		}
	}

	// Ihre Stimme: durch ihren eigenen Körper, dann durch das Gehör des Kindes
	MotherVoice->SetLowPassFilterEnabled(true);
	MotherVoice->SetLowPassFilterFrequency(FMath::Max(40.0f, Hearing.LowPassCutoffHz));
	MotherVoice->SetHighPassFilterEnabled(Hearing.HighPassCutoffHz > 20.0f);
	MotherVoice->SetHighPassFilterFrequency(FMath::Max(20.0f, Hearing.HighPassCutoffHz));
	MotherVoice->SetVolumeMultiplier(1.2f * GenesisAudioCoreLogic::UnbornPresentationGain(Hearing.BodyAudibility));

	const bool bAwake = Mother.Activity != EGenesisMotherActivity::Sleeping;
	if (bAwake && Weeks >= 16.0f && Mother.Speaking > 0.1f && SinceVoice > 20.0f && !MotherVoice->IsPlaying())
	{
		USoundBase* Line = nullptr;
		if (Weeks >= 36.0f && LateLine && !bLateLineSaid)
		{
			Line = LateLine;
			bLateLineSaid = true;
		}
		else if (BellyLines.Num() > 0 && (Mother.bTalkingToBelly || Random.FRand() < Mother.Speaking * DeltaSeconds * 0.1f || SinceVoice < 21.0f))
		{
			Line = BellyLines[Random.RandRange(0, BellyLines.Num() - 1)];
		}
		if (Line)
		{
			MotherVoice->SetSound(Line);
			MotherVoice->Play();
			UE_LOG(LogGenesis, Display, TEXT("Mutterleib: ihre Stimme %s (SSW %.1f, Tiefpass %.0f Hz, Hochpass %.0f Hz, Lautstärke %.2f)"), *Line->GetName(), Weeks,
				Hearing.LowPassCutoffHz, Hearing.HighPassCutoffHz, MotherVoice->VolumeMultiplier);
			SinceVoice = 0.0f;
		}
	}
}
