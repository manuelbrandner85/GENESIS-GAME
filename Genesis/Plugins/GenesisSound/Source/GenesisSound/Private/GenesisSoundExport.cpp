// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoundExport.h"
#include "GenesisLog.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/ArrayWriter.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"
#endif

namespace GenesisSoundExport
{
	namespace
	{
		/** Werte für einen Zeitpunkt der Probe. Der Verlauf ist der Inhalt – ein Standbild klingt nach nichts. */
		FGenesisBodySoundParams ParamsAt(EPreset Preset, float Time, float Duration)
		{
			FGenesisBodySoundParams Params;
			Params.MasterGain = 0.8f;

			switch (Preset)
			{
			case EPreset::Womb:
				Params.bInWomb = true;
				Params.LowPassCutoffHz = 420.0f;
				Params.HeartRateBpm = 142.0f;
				Params.MotherHeartRateBpm = 72.0f;
				Params.BreathsPerMinute = 14.0f;
				Params.BodyAudibility = 1.0f;
				Params.ExternalAudibility = 0.12f;
				break;

			case EPreset::Labor:
			{
				// Eine Wehe alle 150 Sekunden, 80 Sekunden lang: In einer kurzen Probe ist das eine Welle.
				const float Cycle = FMath::Fmod(Time, 150.0f);
				const float Phase = FMath::Clamp(Cycle / 80.0f, 0.0f, 1.0f);
				const float Wave = Phase > 0.0f && Phase < 1.0f
					? FMath::SmoothStep(0.0f, 0.32f, Phase) * (1.0f - FMath::SmoothStep(0.45f, 1.0f, Phase))
					: 0.0f;
				Params.bInWomb = true;
				Params.Pressure = Wave;
				Params.Oxygen = FMath::Clamp(1.0f - 0.45f * Wave, 0.0f, 1.0f);
				Params.HeartRateBpm = FMath::Lerp(145.0f, 105.0f, Wave);
				Params.MotherHeartRateBpm = FMath::Lerp(88.0f, 105.0f, Wave);
				Params.LowPassCutoffHz = 380.0f;
				Params.BodyAudibility = 1.0f;
				Params.ExternalAudibility = 0.15f;
				break;
			}

			case EPreset::Birth:
			{
				// Erste Hälfte im Kanal, dann der Sprung: Der Tiefpass öffnet sich in einer Viertelsekunde.
				const float Moment = Duration * 0.55f;
				const float After = FMath::Clamp((Time - Moment) / 0.25f, 0.0f, 1.0f);
				Params.bInWomb = After < 0.5f;
				Params.LowPassCutoffHz = FMath::Lerp(360.0f, 16000.0f, After);
				Params.HeartRateBpm = FMath::Lerp(112.0f, 138.0f, After);
				Params.MotherHeartRateBpm = FMath::Lerp(96.0f, 0.0f, After);
				Params.BreathsPerMinute = FMath::Lerp(14.0f, 44.0f, After);
				Params.BodyAudibility = FMath::Lerp(1.0f, 0.55f, After);
				Params.ExternalAudibility = FMath::Lerp(0.12f, 1.0f, After);
				Params.Pressure = FMath::Lerp(0.85f, 0.0f, After);
				Params.Oxygen = FMath::Lerp(0.55f, 0.95f, After);
				break;
			}

			case EPreset::Newborn:
			default:
				Params.bInWomb = false;
				Params.LowPassCutoffHz = 15000.0f;
				Params.HeartRateBpm = 132.0f;
				Params.MotherHeartRateBpm = 0.0f;
				Params.BreathsPerMinute = 44.0f;
				Params.BodyAudibility = 0.5f;
				Params.ExternalAudibility = 1.0f;
				break;
			}

			return Params;
		}
	}

	EPreset ParsePreset(const FString& Name)
	{
		if (Name.Equals(TEXT("labor"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("wehen"), ESearchCase::IgnoreCase))
		{
			return EPreset::Labor;
		}
		if (Name.Equals(TEXT("birth"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("geburt"), ESearchCase::IgnoreCase))
		{
			return EPreset::Birth;
		}
		if (Name.Equals(TEXT("newborn"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("neugeboren"), ESearchCase::IgnoreCase))
		{
			return EPreset::Newborn;
		}
		return EPreset::Womb;
	}

	FString GetPresetName(EPreset Preset)
	{
		switch (Preset)
		{
		case EPreset::Labor: return TEXT("Wehen");
		case EPreset::Birth: return TEXT("Geburt");
		case EPreset::Newborn: return TEXT("Neugeboren");
		default: return TEXT("Mutterleib");
		}
	}

	void RenderPreset(EPreset Preset, float SampleRate, float Seconds, TArray<float>& OutSamples)
	{
		const int32 Rate = FMath::Max(8000, FMath::RoundToInt(SampleRate));
		const int32 TotalFrames = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));
		const int32 BlockFrames = 256;

		FGenesisBodySynth Synth;
		Synth.Initialize(static_cast<float>(Rate));

		OutSamples.SetNumUninitialized(TotalFrames);
		for (int32 Start = 0; Start < TotalFrames; Start += BlockFrames)
		{
			const int32 Frames = FMath::Min(BlockFrames, TotalFrames - Start);
			const float Time = static_cast<float>(Start) / static_cast<float>(Rate);
			Synth.SetParams(ParamsAt(Preset, Time, Seconds));
			Synth.Render(OutSamples.GetData() + Start, Frames);
		}
	}

	bool WriteWav(const FString& FilePath, const TArray<float>& Samples, int32 SampleRate)
	{
		const int32 DataBytes = Samples.Num() * sizeof(int16);
		TArray<uint8> File;
		File.Reserve(44 + DataBytes);

		auto Append = [&File](const void* Data, int32 Bytes)
		{
			File.Append(static_cast<const uint8*>(Data), Bytes);
		};
		auto AppendUInt32 = [&Append](uint32 Value) { Append(&Value, 4); };
		auto AppendUInt16 = [&Append](uint16 Value) { Append(&Value, 2); };

		// Kopf einer WAV-Datei (RIFF/PCM, mono, 16 Bit)
		Append("RIFF", 4);
		AppendUInt32(static_cast<uint32>(36 + DataBytes));
		Append("WAVEfmt ", 8);
		AppendUInt32(16);                                        // Länge des Formatblocks
		AppendUInt16(1);                                         // PCM
		AppendUInt16(1);                                         // Kanäle
		AppendUInt32(static_cast<uint32>(SampleRate));
		AppendUInt32(static_cast<uint32>(SampleRate * 2));       // Bytes je Sekunde
		AppendUInt16(2);                                         // Blockausrichtung
		AppendUInt16(16);                                        // Bits je Abtastwert
		Append("data", 4);
		AppendUInt32(static_cast<uint32>(DataBytes));

		for (float Sample : Samples)
		{
			const int16 Value = static_cast<int16>(FMath::Clamp(Sample, -1.0f, 1.0f) * 32767.0f);
			AppendUInt16(static_cast<uint16>(Value));
		}

		TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*FilePath));
		if (!Writer)
		{
			UE_LOG(LogGenesis, Error, TEXT("Klang: Datei konnte nicht geschrieben werden: %s"), *FilePath);
			return false;
		}
		Writer->Serialize(File.GetData(), File.Num());
		Writer->Close();
		return true;
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GenesisSoundRenderCommand(
		TEXT("genesis.Sound.RenderWav"),
		TEXT("Schreibt eine Klangprobe: <mutterleib|wehen|geburt|neugeboren> [Sekunden]. Ablage in Saved/Audio."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			const GenesisSoundExport::EPreset Preset = GenesisSoundExport::ParsePreset(Args.Num() > 0 ? Args[0] : TEXT("mutterleib"));
			const float Seconds = FMath::Clamp(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 12.0f, 1.0f, 120.0f);

			TArray<float> Samples;
			GenesisSoundExport::RenderPreset(Preset, 48000.0f, Seconds, Samples);

			const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audio"),
				FString::Printf(TEXT("GENESIS_%s.wav"), *GenesisSoundExport::GetPresetName(Preset)));
			if (GenesisSoundExport::WriteWav(Path, Samples, 48000))
			{
				UE_LOG(LogGenesis, Display, TEXT("Klang: %s geschrieben (%.1f s, %d Abtastwerte)"), *Path, Seconds, Samples.Num());
			}
		}));
}
#endif
