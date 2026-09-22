// GENESIS: Der Kreislauf des Lebens

#include "GenesisLipSync.h"
#include "Sound/SoundWave.h"
#include "GenesisLog.h"

#if WITH_EDITOR
#include "SpeechAnimationSolverV4.h"
#include "SpeechAnimationSolverTypes.h"
#include "GuiToRawControlsUtils.h"
#include "NNEModelData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

void UGenesisLipSyncTrack::Sample(float Seconds, float Weight, TMap<FName, float>& Out) const
{
	const int32 NumCurves = CurveNames.Num();
	if (NumFrames <= 0 || NumCurves == 0 || Values.Num() < NumFrames * NumCurves)
	{
		return;
	}
	const float Position = FMath::Clamp(Seconds * FramesPerSecond, 0.0f, float(NumFrames - 1));
	const int32 A = FMath::FloorToInt32(Position);
	const int32 B = FMath::Min(A + 1, NumFrames - 1);
	const float Alpha = Position - A;
	for (int32 Curve = 0; Curve < NumCurves; ++Curve)
	{
		const float Value = FMath::Lerp(Values[A * NumCurves + Curve], Values[B * NumCurves + Curve], Alpha);
		Out.Add(CurveNames[Curve], Value * Weight);
	}
}

float UGenesisLipSyncTrack::PeakOf(FName Curve) const
{
	const int32 Index = CurveNames.IndexOfByKey(Curve);
	const int32 NumCurves = CurveNames.Num();
	float Peak = 0.0f;
	for (int32 Frame = 0; Index != INDEX_NONE && Frame < NumFrames; ++Frame)
	{
		Peak = FMath::Max(Peak, Values[Frame * NumCurves + Index]);
	}
	return Peak;
}

float GenesisLipSyncLogic::Envelope(float Seconds, float Duration, float FadeSeconds)
{
	if (Seconds < 0.0f || Seconds > Duration)
	{
		return 0.0f;
	}
	const float Fade = FMath::Max(0.001f, FadeSeconds);
	const float In = FMath::Clamp(Seconds / Fade, 0.0f, 1.0f);
	const float OutRamp = FMath::Clamp((Duration - Seconds) / Fade, 0.0f, 1.0f);
	return FMath::SmoothStep(0.0f, 1.0f, FMath::Min(In, OutRamp));
}

bool GenesisLipSyncLogic::IsOwnedByCharacter(FName RawCurve)
{
	const FString Name = RawCurve.ToString();
	// Lidschlag und Blickrichtung führt die Figur selbst: Sie blinzelt in ihrem Takt und schaut das Kind an
	return Name.Contains(TEXT("eyeBlink")) || Name.Contains(TEXT("eyeLook"));
}

bool FGenesisLipSyncPlayer::IsSpeaking(double AudioTimeNow) const
{
	return Track && AudioTimeNow - StartAudioTime <= Track->GetDuration();
}

void FGenesisLipSyncPlayer::Evaluate(double AudioTimeNow, TMap<FName, float>& Out)
{
	Out.Reset();
	if (!Track)
	{
		return;
	}
	const float Seconds = float(AudioTimeNow - StartAudioTime);
	const float Duration = Track->GetDuration();
	if (Seconds > Duration)
	{
		Track = nullptr;
		return;
	}
	Track->Sample(Seconds, GenesisLipSyncLogic::Envelope(Seconds, Duration), Out);
}

bool UGenesisLipSyncLibrary::BakeLipSync(USoundWave* Sound, const FString& AssetPath, float LookaheadMs)
{
#if WITH_EDITOR
	if (!Sound)
	{
		return false;
	}
	TArray<uint8> Pcm;
	uint32 SampleRate = 0;
	uint16 NumChannels = 0;
	if (!Sound->GetImportedSoundWaveData(Pcm, SampleRate, NumChannels) || SampleRate == 0 || NumChannels == 0)
	{
		UE_LOG(LogGenesis, Warning, TEXT("LipSync: keine Rohdaten in %s"), *Sound->GetPathName());
		return false;
	}

	// 1. Mono, 16 kHz (das Modell hört in 20-ms-Schritten zu 320 Proben). Lineare Interpolation statt der
	//    Treppen-Umrechnung des Solvers, damit keine Spiegelfrequenzen in die Konsonanten geraten.
	const int16* Samples = reinterpret_cast<const int16*>(Pcm.GetData());
	const int32 InFrames = Pcm.Num() / (sizeof(int16) * NumChannels);
	TArray<float> Mono;
	Mono.SetNumUninitialized(InFrames);
	for (int32 Frame = 0; Frame < InFrames; ++Frame)
	{
		float Sum = 0.0f;
		for (int32 Channel = 0; Channel < NumChannels; ++Channel)
		{
			Sum += Samples[Frame * NumChannels + Channel] / 32768.0f;
		}
		Mono[Frame] = Sum / NumChannels;
	}
	constexpr int32 ModelRate = 16000;
	constexpr int32 Hop = 320;
	const double Step = double(SampleRate) / ModelRate;
	const int32 LookaheadFrames = FMath::Max(0, FMath::RoundToInt32(LookaheadMs / 20.0f));
	const int32 SpeechFrames = FMath::CeilToInt32(InFrames / Step / Hop);
	// Hinten Stille anhängen, damit das Modell auch die letzten Silben mit Vorausschau sieht
	const int32 TotalHops = SpeechFrames + LookaheadFrames + 2;
	TArray<float> Resampled;
	Resampled.SetNumZeroed(TotalHops * Hop);
	for (int32 Index = 0; Index < Resampled.Num(); ++Index)
	{
		const double Source = Index * Step;
		const int32 A = FMath::FloorToInt32(Source);
		if (A + 1 >= InFrames)
		{
			break;
		}
		Resampled[Index] = FMath::Lerp(Mono[A], Mono[A + 1], float(Source - A));
	}

	// 2. Das Modell
	UNNEModelData* Model = LoadObject<UNNEModelData>(nullptr, *ISpeechAnimationSolver::GetLatestModelAssetPath());
	if (!Model)
	{
		UE_LOG(LogGenesis, Warning, TEXT("LipSync: Modell fehlt (%s)"), *ISpeechAnimationSolver::GetLatestModelAssetPath());
		return false;
	}
	FSpeechAnimationSolverV4 Solver(Model, TEXT("NNERuntimeORTCpu"));
	if (!Solver.Initialize())
	{
		UE_LOG(LogGenesis, Warning, TEXT("LipSync: Solver startet nicht"));
		return false;
	}

	// 3. Schritt für Schritt: Ausgabe k beschreibt das Gesicht zur Zeit (k+1)·20 ms − Vorausschau
	TArray<TMap<FString, float>> RawFrames;
	for (int32 HopIndex = 0; HopIndex < TotalHops; ++HopIndex)
	{
		FSpeechAnimationAudioFrame In;
		In.AudioSamples = TArray<float>(&Resampled[HopIndex * Hop], Hop);
		In.SamplesCount = Hop;
		In.SampleRate = ModelRate;
		In.NumChannels = 1;
		In.bContiguous = HopIndex > 0;
		In.Mood = EAudioDrivenAnimationMood::AutoDetect;
		In.MoodIntensity = 1.0f;
		In.Lookahead = LookaheadFrames * 20;
		FSpeechAnimationFrameData Solved;
		if (!Solver.SolveAudioFrame(In, Solved))
		{
			UE_LOG(LogGenesis, Warning, TEXT("LipSync: Schritt %d fehlgeschlagen"), HopIndex);
			return false;
		}
		if (HopIndex + 1 <= LookaheadFrames)
		{
			continue; // beschreibt die Zeit vor dem ersten Laut
		}
		TMap<FString, float> Gui;
		for (int32 Curve = 0; Curve < Solved.CurveNames.Num(); ++Curve)
		{
			Gui.Add(Solved.CurveNames[Curve].ToString(), Solved.CurveValues[Curve]);
		}
		RawFrames.Add(GuiToRawControlsUtils::ConvertGuiToRawControls(Gui));
	}
	// Frame 0 der Spur = Zeit 0 der Aufnahme: RawFrames[i] gehört zu (i + 1) · 20 ms − … → um ein Bild vorn auffüllen
	if (RawFrames.Num() > 0)
	{
		const TMap<FString, float> First = RawFrames[0];
		RawFrames.Insert(First, 0);
	}
	const int32 NumFrames = FMath::Min(RawFrames.Num(), SpeechFrames + 1);

	// 4. Nur Kurven behalten, die etwas tun – und nicht die, die die Figur selbst führt
	TArray<FString> Keep;
	if (NumFrames > 0)
	{
		for (const TPair<FString, float>& Pair : RawFrames[0])
		{
			float Peak = 0.0f;
			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				Peak = FMath::Max(Peak, FMath::Abs(RawFrames[Frame].FindRef(Pair.Key)));
			}
			if (Peak > 0.02f && !GenesisLipSyncLogic::IsOwnedByCharacter(FName(Pair.Key)))
			{
				Keep.Add(Pair.Key);
			}
		}
	}
	Keep.Sort();

	// 5. Messen statt vermuten: Wann öffnet sich der Kiefer im Verhältnis zur Lautstärke? (Verschiebung mit der
	//    höchsten Korrelation zwischen Hüllkurve und jawOpen – sollte nahe 0 liegen)
	TArray<float> Loudness;
	Loudness.SetNumZeroed(NumFrames);
	for (int32 Frame = 0; Frame < NumFrames; ++Frame)
	{
		double Energy = 0.0;
		for (int32 Index = 0; Index < Hop; ++Index)
		{
			const int32 Sample = Frame * Hop + Index;
			Energy += Resampled.IsValidIndex(Sample) ? FMath::Square(Resampled[Sample]) : 0.0;
		}
		Loudness[Frame] = FMath::Sqrt(float(Energy / Hop));
	}
	int32 BestLag = 0;
	double BestCorrelation = -1.0;
	for (int32 Lag = -10; Lag <= 10; ++Lag)
	{
		double Sum = 0.0;
		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			const int32 Other = Frame + Lag;
			if (Other >= 0 && Other < NumFrames)
			{
				Sum += Loudness[Frame] * RawFrames[Other].FindRef(TEXT("CTRL_expressions_jawOpen"));
			}
		}
		if (Sum > BestCorrelation)
		{
			BestCorrelation = Sum;
			BestLag = Lag;
		}
	}

	// 6. Speichern
	const FString PackageName = AssetPath;
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();
	UGenesisLipSyncTrack* Track = FindObject<UGenesisLipSyncTrack>(Package, *AssetName);
	if (!Track)
	{
		Track = NewObject<UGenesisLipSyncTrack>(Package, *AssetName, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Track);
	}
	Track->FramesPerSecond = 50.0f;
	Track->NumFrames = NumFrames;
	Track->CurveNames.Reset();
	for (const FString& Name : Keep)
	{
		Track->CurveNames.Add(FName(Name));
	}
	Track->Values.SetNumZeroed(NumFrames * Keep.Num());
	for (int32 Frame = 0; Frame < NumFrames; ++Frame)
	{
		for (int32 Curve = 0; Curve < Keep.Num(); ++Curve)
		{
			Track->Values[Frame * Keep.Num() + Curve] = RawFrames[Frame].FindRef(Keep[Curve]);
		}
	}
	Track->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const bool bSaved = UPackage::SavePackage(Package, Track, *Filename, Args);
	UE_LOG(LogGenesis, Display, TEXT("LipSync: %s – %d Bilder (%.2f s), %d Kurven, Kiefer max %.2f, Versatz Kiefer/Lautstärke %+d ms%s"),
		*AssetName, NumFrames, NumFrames / 50.0f, Keep.Num(), Track->PeakOf(TEXT("CTRL_expressions_jawOpen")), BestLag * 20,
		bSaved ? TEXT("") : TEXT(" – NICHT GESPEICHERT"));
	return bSaved;
#else
	return false;
#endif
}
