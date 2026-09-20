// GENESIS: Der Kreislauf des Lebens

#include "GenesisWorldSoundExport.h"
#include "GenesisLog.h"
#include "GenesisSoundExport.h"
#include "GenesisWorldSoundSynth.h"
#include "Misc/Paths.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

namespace GenesisWorldSoundExport
{
	EGenesisPlace ParsePlace(const FString& Name)
	{
		const FString Lower = Name.ToLower();
		if (Lower == TEXT("eileiter") || Lower == TEXT("oviduct")) return EGenesisPlace::OviductAmpulla;
		if (Lower == TEXT("kreisssaal") || Lower == TEXT("kreißsaal") || Lower == TEXT("saal")) return EGenesisPlace::DeliveryRoom;
		if (Lower == TEXT("nichts") || Lower == TEXT("none")) return EGenesisPlace::None;
		return EGenesisPlace::Womb;
	}

	FString GetPlaceName(EGenesisPlace Place)
	{
		switch (Place)
		{
		case EGenesisPlace::OviductAmpulla: return TEXT("Eileiter");
		case EGenesisPlace::DeliveryRoom: return TEXT("Kreisssaal");
		case EGenesisPlace::Womb: return TEXT("Mutterleib");
		default: return TEXT("Stille");
		}
	}

	FGenesisWorldSoundParams GetPresetParams(EGenesisPlace Place)
	{
		FGenesisWorldSoundParams Params;
		Params.Place = Place;
		switch (Place)
		{
		case EGenesisPlace::OviductAmpulla:
			Params.Loudness = 0.9f;
			Params.MaternalHeartRateBpm = 72.0f;
			Params.Activity = 0.1f;
			break;
		case EGenesisPlace::DeliveryRoom:
			Params.Loudness = 0.85f;
			Params.MaternalHeartRateBpm = 96.0f;
			Params.Activity = 0.7f;
			break;
		case EGenesisPlace::Womb:
		default:
			Params.Loudness = 0.9f;
			Params.MaternalHeartRateBpm = 78.0f;
			Params.Activity = 0.3f;
			Params.Digestion = 0.6f;
			break;
		}
		return Params;
	}

	void RenderPlace(EGenesisPlace Place, float SampleRate, float Seconds, TArray<float>& OutSamples)
	{
		const int32 Rate = FMath::Max(8000, FMath::RoundToInt(SampleRate));
		const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));

		FGenesisWorldSoundSynth Synth(0x4F52544Full);
		Synth.Initialize(static_cast<float>(Rate));
		Synth.SetParams(GetPresetParams(Place));

		OutSamples.SetNumUninitialized(TotalFrames);
		Synth.Render(OutSamples.GetData(), TotalFrames);
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisWorldRenderCommand(
		TEXT("genesis.World.RenderWav"),
		TEXT("Schreibt eine Raumprobe: <mutterleib|eileiter|kreisssaal> [Sekunden]. Ablage in Saved/Audio."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			const EGenesisPlace Place = GenesisWorldSoundExport::ParsePlace(Args.Num() > 0 ? Args[0] : TEXT("mutterleib"));
			const float Seconds = FMath::Clamp(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 15.0f, 1.0f, 120.0f);

			TArray<float> Samples;
			GenesisWorldSoundExport::RenderPlace(Place, 48000.0f, Seconds, Samples);

			const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audio"),
				FString::Printf(TEXT("GENESIS_Ort_%s.wav"), *GenesisWorldSoundExport::GetPlaceName(Place)));
			if (GenesisSoundExport::WriteWav(Path, Samples, 48000))
			{
				UE_LOG(LogGenesis, Display, TEXT("Ort: %s geschrieben (%.1f s)"), *Path, Seconds);
			}
		}));
}
#endif
