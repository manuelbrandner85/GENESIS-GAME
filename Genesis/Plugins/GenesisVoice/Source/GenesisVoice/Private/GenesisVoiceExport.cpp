// GENESIS: Der Kreislauf des Lebens

#include "GenesisVoiceExport.h"
#include "GenesisLog.h"
#include "GenesisSoundExport.h"
#include "GenesisVoiceLogic.h"
#include "GenesisVoiceSynth.h"
#include "Misc/Paths.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

namespace GenesisVoiceExport
{
	namespace
	{
		/** Wer da spricht – Alter, Geschlecht, Zustand. */
		FGenesisVoiceInputs InputsFor(EPreset Preset)
		{
			FGenesisVoiceInputs Inputs;
			switch (Preset)
			{
			case EPreset::Newborn:
				Inputs.Sex = EGenesisBiologicalSex::Female;
				Inputs.AgeYears = 0.0f;
				Inputs.HeightCm = 50.0f;
				Inputs.Arousal = 0.95f;
				Inputs.IndividualSeed = 0x4E455742ull;
				break;
			case EPreset::Infant:
				Inputs.Sex = EGenesisBiologicalSex::Male;
				Inputs.AgeYears = 0.5f;
				Inputs.HeightCm = 68.0f;
				Inputs.Arousal = 0.35f;
				Inputs.IndividualSeed = 0x494E4641ull;
				break;
			case EPreset::Child:
				Inputs.Sex = EGenesisBiologicalSex::Female;
				Inputs.AgeYears = 6.0f;
				Inputs.HeightCm = 118.0f;
				Inputs.Arousal = 0.45f;
				Inputs.IndividualSeed = 0x4348494Cull;
				break;
			case EPreset::Woman:
				Inputs.Sex = EGenesisBiologicalSex::Female;
				Inputs.AgeYears = 32.0f;
				Inputs.HeightCm = 167.0f;
				Inputs.Arousal = 0.25f;
				Inputs.IndividualSeed = 0x574F4D41ull;
				break;
			case EPreset::Man:
				Inputs.Sex = EGenesisBiologicalSex::Male;
				Inputs.AgeYears = 35.0f;
				Inputs.HeightCm = 179.0f;
				Inputs.Arousal = 0.25f;
				Inputs.IndividualSeed = 0x4D414E4Eull;
				break;
			case EPreset::Elder:
				Inputs.Sex = EGenesisBiologicalSex::Male;
				Inputs.AgeYears = 80.0f;
				Inputs.HeightCm = 172.0f;
				Inputs.RespiratoryHealth = 0.62f;
				Inputs.Arousal = 0.2f;
				Inputs.IndividualSeed = 0x454C4445ull;
				break;
			case EPreset::Hoarse:
				Inputs.Sex = EGenesisBiologicalSex::Female;
				Inputs.AgeYears = 29.0f;
				Inputs.HeightCm = 165.0f;
				Inputs.Illness = 0.8f;
				Inputs.Exhaustion = 0.5f;
				Inputs.RespiratoryHealth = 0.7f;
				Inputs.IndividualSeed = 0x484F4152ull;
				break;
			case EPreset::Mother:
			default:
				Inputs.Sex = EGenesisBiologicalSex::Female;
				Inputs.AgeYears = 30.0f;
				Inputs.HeightCm = 168.0f;
				Inputs.Exhaustion = 0.45f;
				Inputs.Arousal = 0.2f;
				Inputs.IndividualSeed = 0x4D4F5448ull;
				break;
			}
			return Inputs;
		}

		/** Welche Laute die Probe zeigt. Ein Neugeborenes schreit – etwas anderes kann es nicht. */
		void UtterancesFor(EPreset Preset, TArray<EGenesisUtterance>& OutTypes, float& OutPauseSeconds, float& OutIntensity)
		{
			OutPauseSeconds = 0.6f;
			OutIntensity = 0.6f;
			switch (Preset)
			{
			case EPreset::Newborn:
				OutTypes = { EGenesisUtterance::Cry, EGenesisUtterance::Fuss };
				OutIntensity = 0.95f;
				OutPauseSeconds = 0.8f;
				break;
			case EPreset::Infant:
				OutTypes = { EGenesisUtterance::Coo, EGenesisUtterance::Babble, EGenesisUtterance::Laugh };
				OutIntensity = 0.5f;
				break;
			case EPreset::Child:
				OutTypes = { EGenesisUtterance::Speak, EGenesisUtterance::Laugh, EGenesisUtterance::Call };
				OutIntensity = 0.65f;
				break;
			case EPreset::Mother:
				OutTypes = { EGenesisUtterance::Soothe, EGenesisUtterance::Hum, EGenesisUtterance::Speak };
				OutIntensity = 0.45f;
				break;
			case EPreset::Elder:
				OutTypes = { EGenesisUtterance::Speak, EGenesisUtterance::Sigh };
				OutIntensity = 0.45f;
				break;
			default:
				OutTypes = { EGenesisUtterance::Speak, EGenesisUtterance::Speak, EGenesisUtterance::Laugh };
				break;
			}
		}
	}

	EPreset ParsePreset(const FString& Name)
	{
		const FString Lower = Name.ToLower();
		if (Lower == TEXT("neugeboren") || Lower == TEXT("newborn") || Lower == TEXT("schrei")) return EPreset::Newborn;
		if (Lower == TEXT("saeugling") || Lower == TEXT("säugling") || Lower == TEXT("infant")) return EPreset::Infant;
		if (Lower == TEXT("kind") || Lower == TEXT("child")) return EPreset::Child;
		if (Lower == TEXT("frau") || Lower == TEXT("woman")) return EPreset::Woman;
		if (Lower == TEXT("mann") || Lower == TEXT("man")) return EPreset::Man;
		if (Lower == TEXT("alt") || Lower == TEXT("greis") || Lower == TEXT("elder")) return EPreset::Elder;
		if (Lower == TEXT("heiser") || Lower == TEXT("hoarse")) return EPreset::Hoarse;
		return EPreset::Mother;
	}

	FString GetPresetName(EPreset Preset)
	{
		switch (Preset)
		{
		case EPreset::Newborn: return TEXT("Neugeboren");
		case EPreset::Infant: return TEXT("Saeugling");
		case EPreset::Child: return TEXT("Kind");
		case EPreset::Woman: return TEXT("Frau");
		case EPreset::Man: return TEXT("Mann");
		case EPreset::Elder: return TEXT("Alt");
		case EPreset::Hoarse: return TEXT("Heiser");
		default: return TEXT("Mutter");
		}
	}

	FGenesisVoiceProfile GetPresetProfile(EPreset Preset)
	{
		return GenesisVoiceLogic::BuildProfile(InputsFor(Preset), FGenesisVoiceTuning());
	}

	void RenderUtterance(const FGenesisVoiceProfile& Profile, const FGenesisUtterance& Utterance,
		float SampleRate, float Seconds, TArray<float>& OutSamples)
	{
		const int32 Rate = FMath::Max(8000, FMath::RoundToInt(SampleRate));
		const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));

		FGenesisVoiceSynth Synth(Utterance.Seed != 0 ? Utterance.Seed : 0x564F4943ull);
		Synth.Initialize(static_cast<float>(Rate));
		Synth.SetProfile(Profile);
		Synth.Begin(Utterance);

		OutSamples.SetNumUninitialized(TotalFrames);
		Synth.Render(OutSamples.GetData(), TotalFrames);
	}

	void RenderPreset(EPreset Preset, float SampleRate, float Seconds, TArray<float>& OutSamples)
	{
		const int32 Rate = FMath::Max(8000, FMath::RoundToInt(SampleRate));
		const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));

		const FGenesisVoiceProfile Profile = GetPresetProfile(Preset);

		TArray<EGenesisUtterance> Types;
		float Pause = 0.6f;
		float Intensity = 0.6f;
		UtterancesFor(Preset, Types, Pause, Intensity);

		FGenesisVoiceSynth Synth(0x50524F42ull);
		Synth.Initialize(static_cast<float>(Rate));
		Synth.SetProfile(Profile);

		OutSamples.SetNumUninitialized(TotalFrames);
		FMemory::Memzero(OutSamples.GetData(), sizeof(float) * TotalFrames);

		int32 Cursor = 0;
		int32 Index = 0;
		while (Cursor < TotalFrames)
		{
			FGenesisUtterance Utterance;
			Utterance.Type = Types.IsValidIndex(Index % Types.Num()) ? Types[Index % Types.Num()] : EGenesisUtterance::Speak;
			Utterance.Intensity = Intensity;
			Utterance.Seed = 0x50524F42ull + Index * 977u;
			Synth.Begin(Utterance);

			const int32 Frames = FMath::Min(TotalFrames - Cursor, FMath::RoundToInt(Synth.GetPlannedDurationSeconds() * Rate) + 1);
			Synth.Render(OutSamples.GetData() + Cursor, Frames);
			Cursor += Frames + FMath::RoundToInt(Pause * Rate);
			++Index;
		}
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisVoiceRenderCommand(
		TEXT("genesis.Voice.RenderWav"),
		TEXT("Schreibt eine Stimmprobe: <neugeboren|saeugling|kind|frau|mann|alt|mutter|heiser> [Sekunden]. Ablage in Saved/Audio."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			const GenesisVoiceExport::EPreset Preset = GenesisVoiceExport::ParsePreset(Args.Num() > 0 ? Args[0] : TEXT("mutter"));
			const float Seconds = FMath::Clamp(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 10.0f, 1.0f, 120.0f);

			TArray<float> Samples;
			GenesisVoiceExport::RenderPreset(Preset, 48000.0f, Seconds, Samples);

			const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audio"),
				FString::Printf(TEXT("GENESIS_Stimme_%s.wav"), *GenesisVoiceExport::GetPresetName(Preset)));
			if (GenesisSoundExport::WriteWav(Path, Samples, 48000))
			{
				const FGenesisVoiceProfile Profile = GenesisVoiceExport::GetPresetProfile(Preset);
				UE_LOG(LogGenesis, Display, TEXT("Stimme: %s geschrieben (%.1f s, %s)"),
					*Path, Seconds, *GenesisVoiceLogic::DescribeVoice(Profile));
			}
		}));
}
#endif
