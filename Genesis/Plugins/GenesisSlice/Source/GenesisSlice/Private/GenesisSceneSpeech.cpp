// GENESIS: Der Kreislauf des Lebens

#include "GenesisSceneSpeech.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenesisBirthSubsystem.h"
#include "GenesisBodySubsystem.h"
#include "GenesisEarlyLifeSubsystem.h"
#include "GenesisGeneticsSubsystem.h"
#include "GenesisLog.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

namespace GenesisSceneSpeechLogic
{
	namespace
	{
		FName Variant(const TCHAR* Base, bool bGirl)
		{
			return FName(FString(Base) + (bGirl ? TEXT("_F") : TEXT("_M")));
		}

		float SinceSaid(const FMemory& Memory, FName Id)
		{
			const float* When = Memory.LastSaid.Find(Id);
			return When ? Memory.Now - *When : 1.0e9f;
		}
	}

	bool IsMother(FName LineId)
	{
		return LineId.ToString().Contains(TEXT("_M_"));
	}

	bool IsVocalization(FName LineId)
	{
		return LineId == FName(TEXT("P_M_Pressen"));
	}

	FName ChooseChild(const FInputs& In, float Now, float& InOutLastQuietSound, bool& bInOutFirstCryDone, bool bChildFree)
	{
		if (!In.bNewborn || In.SecondsSinceBirth < 0.0f)
		{
			return NAME_None;
		}
		// Der erste Schrei kommt eine halbe Sekunde nach der Geburt – die Hebamme spricht über ihn hinweg
		if (!bInOutFirstCryDone)
		{
			if (In.SecondsSinceBirth >= 0.5f)
			{
				bInOutFirstCryDone = true;
				return TEXT("SFX_Schrei_Erster");
			}
			return NAME_None;
		}
		if (!bChildFree || In.bAsleep)
		{
			return NAME_None;
		}
		// Schreien läuft ohne Pause weiter, solange der Zustand es verlangt – ein Kind schreit in Wellen, nicht in Einzelsätzen
		if (In.bCrying && In.CryLoudness > 0.6f)
		{
			return TEXT("SFX_Schrei_Stark");
		}
		if (In.bCrying && In.CryLoudness > 0.25f)
		{
			return TEXT("SFX_Schrei_Wimmern");
		}
		// Ruhig auf der Haut: ab und zu ein Grunzen, Seufzen, Schmatzen
		if (In.bSkinToSkin && !In.bCrying && Now - InOutLastQuietSound > 22.0f)
		{
			InOutLastQuietSound = Now;
			return TEXT("SFX_Baby_Laute");
		}
		return NAME_None;
	}

	FName Choose(const FInputs& In, FMemory& Memory)
	{
		// Wehen erkennen, auch während gerade jemand spricht: Anstieg über 0,5, Abklingen unter 0,25
		const bool bContracting = Memory.bWasContracting ? In.ContractionIntensity > 0.25f : In.ContractionIntensity > 0.5f;
		const bool bContractionStarts = bContracting && !Memory.bWasContracting;
		const bool bContractionEnds = !bContracting && Memory.bWasContracting;
		Memory.bWasContracting = bContracting;
		const bool bEyeContactStarts = In.bEyeContact && !Memory.bHadEyeContact;
		Memory.bHadEyeContact = In.bEyeContact;
		const bool bSkinStarts = In.bSkinToSkin && !Memory.bWasSkin;
		Memory.bWasSkin = In.bSkinToSkin;
		if (bEyeContactStarts)
		{
			++Memory.EyeContacts;
		}

		FName Pick = NAME_None;
		auto Once = [&Memory](FName Id) { return Memory.Said.Contains(Id) ? NAME_None : Id; };
		auto Rested = [&Memory](FName Id, float Seconds) { return SinceSaid(Memory, Id) >= Seconds ? Id : NAME_None; };

		if (In.SecondsSinceBirth >= 0.0f && In.bNewborn)
		{
			// Die ersten Minuten: erst der Ruf der Hebamme, dann die Mutter, dann Haut an Haut
			const float T = In.SecondsSinceBirth;
			const FName Announce = Variant(TEXT("D_H_Da"), In.bGirl);
			const FName ToChest = Variant(TEXT("D_H_Brust"), In.bGirl);
			const FName Breathing = Variant(TEXT("D_H_Atmung"), In.bGirl);
			if (T >= 2.5f && !Memory.Said.Contains(Announce)) Pick = Announce;
			else if (Memory.Said.Contains(Announce) && SinceSaid(Memory, Announce) > 1.0f) Pick = Once(TEXT("D_M_Hallo"));
			if (Pick.IsNone() && In.bAsleep) Pick = Once(TEXT("Z_M_Schlaf"));
			if (Pick.IsNone() && In.bCrying && In.bSkinToSkin && Memory.Said.Contains(TEXT("S_M_Warm"))) Pick = Rested(TEXT("S_M_Beruhigen"), 25.0f);
			if (Pick.IsNone() && bEyeContactStarts)
			{
				Pick = Memory.EyeContacts <= 1 ? Once(TEXT("E_M_Blick_01")) : Rested(TEXT("E_M_Blick_02"), 40.0f);
				if (Pick.IsNone()) Pick = Once(TEXT("E_M_Blick_02"));
			}
			if (Pick.IsNone() && (bSkinStarts || In.bSkinToSkin)) Pick = Once(TEXT("S_M_Warm"));
			if (Pick.IsNone() && !In.bSkinToSkin && Memory.Said.Contains(TEXT("D_M_Hallo")) && T >= 12.0f) Pick = Once(ToChest);
			if (Pick.IsNone() && In.bSkinToSkin && SinceSaid(Memory, TEXT("S_M_Warm")) > 15.0f) Pick = Once(Breathing);
			if (Pick.IsNone() && In.bSkinToSkin && SinceSaid(Memory, Breathing) > 20.0f) Pick = Once(TEXT("S_M_Geschafft"));
			if (Pick.IsNone() && In.RootingEffort > 0.3f) Pick = Once(Variant(TEXT("F_H_Suchen"), In.bGirl));
		}
		else if (In.Stage > EGenesisLaborStage::NotStarted && In.Stage < EGenesisLaborStage::Delivered)
		{
			const bool bPushing = In.Stage == EGenesisLaborStage::Pushing;
			// Erschwernisse haben Vorrang – die Hebamme reagiert, bevor sie lobt
			if (In.Complication == EGenesisBirthComplication::CordCompression && In.Stage >= EGenesisLaborStage::Active && bContracting)
				Pick = Once(TEXT("C_H_Herztoene"));
			if (Pick.IsNone() && In.Complication == EGenesisBirthComplication::Breech && bPushing)
				Pick = Once(TEXT("C_H_Steiss"));
			if (Pick.IsNone() && In.Complication == EGenesisBirthComplication::ShoulderDystocia && bPushing && In.Descent > 0.9f)
				Pick = Once(TEXT("C_H_Schulter"));
			// Beim Durchtritt des Kopfes: nicht mehr pressen, hecheln (Dammschutz)
			if (Pick.IsNone() && bPushing && In.Descent > 0.85f)
				Pick = Once(TEXT("P_H_Kopf"));

			if (Pick.IsNone() && bContractionStarts)
			{
				if (bPushing)
				{
					Pick = (Memory.PushCount++ % 2 == 0) ? TEXT("P_H_Schieben_01") : TEXT("P_H_Schieben_02");
				}
				else if (In.Stage == EGenesisLaborStage::Transition)
				{
					Pick = Once(TEXT("T_M_KannNicht"));
					if (Pick.IsNone()) Pick = Rested(TEXT("L_M_Wehe_01"), 20.0f);
				}
				else
				{
					Pick = Rested(TEXT("L_M_Wehe_01"), 25.0f);
				}
			}
			else if (Pick.IsNone() && bContractionEnds)
			{
				if (bPushing)
				{
					Pick = Rested(TEXT("P_H_Pause"), 15.0f);
				}
				else if (In.Stage == EGenesisLaborStage::Transition && Memory.Said.Contains(TEXT("T_M_KannNicht")))
				{
					Pick = Once(TEXT("T_H_Doch"));
					if (Pick.IsNone()) Pick = Rested(TEXT("L_H_Atmen_02"), 20.0f);
				}
				else
				{
					// Nach der Wehe: abwechselnd Hebamme und Mutter – nie zweimal dasselbe hintereinander
					static const FName Rotation[] = { TEXT("L_H_Atmen_01"), TEXT("L_M_Wehe_02"), TEXT("L_H_Lob_01"), TEXT("L_H_Atmen_02") };
					for (int32 Offset = 0; Offset < UE_ARRAY_COUNT(Rotation) && Pick.IsNone(); ++Offset)
					{
						Pick = Rested(Rotation[(Memory.LaborLineCount + Offset) % UE_ARRAY_COUNT(Rotation)], 18.0f);
					}
					++Memory.LaborLineCount;
				}
			}
		}
		else if (In.bInBirthScene && In.Stage == EGenesisLaborStage::NotStarted)
		{
			// Schwangerschaft: Die Mutter spricht mit ihrem Bauch – selten, und das Kind hört es gedämpft
			if (SinceSaid(Memory, TEXT("G_M_Bauch_01")) > 1.0e8f && Memory.Now > 6.0f) Pick = TEXT("G_M_Bauch_01");
			else if (SinceSaid(Memory, TEXT("G_M_Bauch_01")) > 25.0f) Pick = Once(TEXT("G_M_Bauch_02"));
		}

		// Das Pressen der Mutter ist keine Rede: Es läuft auf einem eigenen Kanal, auch während die Hebamme
		// spricht – im Kreißsaal zählt die Hebamme mit, während die Mutter presst.
		FName Groan = NAME_None;
		if (In.Stage == EGenesisLaborStage::Pushing && bContracting && In.ContractionIntensity > 0.8f)
		{
			Groan = Rested(TEXT("P_M_Pressen"), 12.0f);
		}

		// Wer gerade nicht zu Wort kommt, sagt es gleich danach – aber nur, solange es noch passt (3 s).
		// Vorher gingen so die Pause-Ansage, „Ich sehe das Köpfchen" und die Antwort der Hebamme verloren
		// (gemessen im Spiel: in der Austreibung nur „Schieben", sonst nichts).
		if (!Pick.IsNone() && !In.bCanSpeak)
		{
			if (Memory.Pending.IsNone() || Memory.Now - Memory.PendingAt > 3.0f)
			{
				Memory.Pending = Pick;
				Memory.PendingAt = Memory.Now;
			}
			if (!Groan.IsNone() && In.bCanVocalize)
			{
				Memory.Said.Add(Groan);
				Memory.LastSaid.Add(Groan, Memory.Now);
				return Groan;
			}
			return NAME_None;
		}
		if (Pick.IsNone() && In.bCanSpeak && !Memory.Pending.IsNone() && Memory.Now - Memory.PendingAt <= 3.0f)
		{
			Pick = Memory.Pending;
		}
		if (In.bCanSpeak)
		{
			Memory.Pending = NAME_None;
		}
		if (Pick.IsNone() || !In.bCanSpeak)
		{
			if (!Groan.IsNone() && In.bCanVocalize)
			{
				Memory.Said.Add(Groan);
				Memory.LastSaid.Add(Groan, Memory.Now);
				return Groan;
			}
			return NAME_None;
		}
		Memory.Said.Add(Pick);
		Memory.LastSaid.Add(Pick, Memory.Now);
		return Pick;
	}
}

AGenesisSceneSpeech::AGenesisSceneSpeech()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool AGenesisSceneSpeech::IsGirl() const
{
	// Aus dem Genom des Kindes: Welcher Satz („Da ist sie" / „Da ist er") fällt, entscheidet die Befruchtung
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr;
	const UGenesisBodySubsystem* Body = GameInstance ? GameInstance->GetSubsystem<UGenesisBodySubsystem>() : nullptr;
	const UGenesisGeneticsSubsystem* Genetics = GameInstance ? GameInstance->GetSubsystem<UGenesisGeneticsSubsystem>() : nullptr;
	if (!EarlyLife || !Body || !Genetics || !EarlyLife->HasNewborn())
	{
		return true;
	}
	for (int32 Index = 0; Index < Body->GetBodyCount(); ++Index)
	{
		const FGenesisBodyState* State = Body->GetBodyByIndex(Index);
		if (State && State->EntityId == EarlyLife->GetState().EntityId)
		{
			const FGenesisGenome* Genome = Genetics->FindGenome(State->GenomeId);
			return !Genome || Genome->Sex == EGenesisBiologicalSex::Female;
		}
	}
	return true;
}

GenesisSceneSpeechLogic::FInputs AGenesisSceneSpeech::GatherInputs() const
{
	GenesisSceneSpeechLogic::FInputs In;
	In.bInBirthScene = true;
	const UGameInstance* GameInstance = GetGameInstance();
	if (const UGenesisBirthSubsystem* Birth = GameInstance ? GameInstance->GetSubsystem<UGenesisBirthSubsystem>() : nullptr)
	{
		const FGenesisBirthState& State = Birth->GetState();
		In.Stage = State.Stage;
		In.Complication = State.Complication;
		In.ContractionIntensity = State.ContractionIntensity;
		In.Descent = State.Descent;
	}
	if (const UGenesisEarlyLifeSubsystem* EarlyLife = GameInstance ? GameInstance->GetSubsystem<UGenesisEarlyLifeSubsystem>() : nullptr)
	{
		if (EarlyLife->HasNewborn())
		{
			const FGenesisNewbornState& Child = EarlyLife->GetState();
			In.bNewborn = true;
			In.bSkinToSkin = Child.bSkinToSkin;
			In.bEyeContact = Child.bEyeContact;
			In.bCrying = Child.IsCrying();
			In.RootingEffort = Child.RootingEffort;
			In.bAsleep = Child.Stage == EGenesisNewbornStage::FirstSleep;
			In.CryLoudness = Child.GetCryLoudness();
			In.bFirstBreaths = Child.Stage == EGenesisNewbornStage::FirstBreaths;
		}
	}
	In.SecondsSinceBirth = BornAt >= 0.0f ? Memory.Now - BornAt : -1.0f;
	In.bGirl = IsGirl();
	return In;
}

void AGenesisSceneSpeech::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Memory.Now += DeltaSeconds;
	Busy = FMath::Max(0.0f, Busy - DeltaSeconds);
	BusyVocal = FMath::Max(0.0f, BusyVocal - DeltaSeconds);

	GenesisSceneSpeechLogic::FInputs In = GatherInputs();
	if (In.bNewborn && BornAt < 0.0f)
	{
		BornAt = Memory.Now;
		In.SecondsSinceBirth = 0.0f;
	}
	In.bCanSpeak = Busy <= 0.0f;
	In.bCanVocalize = BusyVocal <= 0.0f;
	const FName Line = GenesisSceneSpeechLogic::Choose(In, Memory);
	if (!Line.IsNone())
	{
		Play(Line);
	}

	// Das Kind: echte Aufnahmen auf eigenem Kanal (Schreien liegt unter der Rede der Erwachsenen)
	ChildBusy = FMath::Max(0.0f, ChildBusy - DeltaSeconds);
	const FName ChildSound = GenesisSceneSpeechLogic::ChooseChild(In, Memory.Now, LastQuietSound, bFirstCryDone, ChildBusy <= 0.0f);
	if (!ChildSound.IsNone())
	{
		PlayChild(ChildSound);
	}
	// Beruhigt sich das Kind, klingt das Schreien aus, statt bis zum Ende der Aufnahme weiterzulaufen
	if (ChildAudio && ChildAudio->IsPlaying() && LastChildSound.ToString().StartsWith(TEXT("SFX_Schrei_")) && LastChildSound != FName(TEXT("SFX_Schrei_Erster")) && !In.bCrying)
	{
		ChildAudio->FadeOut(1.2f, 0.0f);
		ChildBusy = FMath::Min(ChildBusy, 1.2f);
		LastChildSound = NAME_None;
	}

	// Vor der Geburt hört das Kind alles durch Bauchdecke und Fruchtwasser – nur die Tiefen kommen durch
	const UGameInstance* GameInstance = GetGameInstance();
	const UGenesisBirthSubsystem* Birth = GameInstance ? GameInstance->GetSubsystem<UGenesisBirthSubsystem>() : nullptr;
	const float Muffling = Birth ? FMath::Clamp(Birth->GetPerception().SoundMuffling, 0.0f, 1.0f) : 0.0f;
	const float Cutoff = FMath::Exp(FMath::Lerp(FMath::Loge(20000.0f), FMath::Loge(WombCutoffHz), Muffling));

	// Der Raum: nachts im Kreißsaal – Lüftung, ferne Schritte, Monitore. Vor der Geburt nur als dumpfes Rauschen
	if (!Room)
	{
		if (USoundWave* RoomSound = Cast<USoundWave>(FSoftObjectPath(TEXT("/Game/Genesis/Audio/Sounds/SFX_Kreisssaal_Nacht.SFX_Kreisssaal_Nacht")).TryLoad()))
		{
			Room = UGameplayStatics::SpawnSound2D(this, RoomSound, RoomVolume, 1.0f, 0.0f, nullptr, false, false);
		}
	}
	if (Room)
	{
		Room->SetLowPassFilterEnabled(Muffling > 0.02f);
		Room->SetLowPassFilterFrequency(Cutoff);
		Room->SetVolumeMultiplier(RoomVolume * FMath::Lerp(1.0f, 0.3f, Muffling));
	}

	for (UAudioComponent* Component : { Current.Get(), Vocal.Get() })
	{
		if (Component && Component->IsPlaying())
		{
			Component->SetLowPassFilterEnabled(Muffling > 0.02f);
			Component->SetLowPassFilterFrequency(Cutoff);
			Component->SetVolumeMultiplier(FMath::Lerp(1.0f, 0.55f, Muffling));
		}
	}
}

void AGenesisSceneSpeech::PlayChild(FName SoundId)
{
	// Zwei Aufnahmen je Laut, abwechselnd: Ein Kind schreit nie zweimal genau gleich
	const FString Name = SoundId.ToString() + ((ChildVariant++ % 2 == 1) ? TEXT("_v2") : TEXT(""));
	const FString Path = FString::Printf(TEXT("/Game/Genesis/Audio/Sounds/%s.%s"), *Name, *Name);
	USoundWave* Sound = Cast<USoundWave>(FSoftObjectPath(Path).TryLoad());
	if (!Sound)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Klang fehlt: %s"), *Path);
		return;
	}
	const bool bQuiet = SoundId == FName(TEXT("SFX_Baby_Laute"));
	ChildAudio = UGameplayStatics::SpawnSound2D(this, Sound, bQuiet ? 0.55f : 0.9f);
	// Schreien schließt ohne Pause an (Wellen), ruhige Laute brauchen keinen Anschluss
	ChildBusy = Sound->GetDuration() - (bQuiet ? 0.0f : 0.15f);
	LastChildSound = SoundId;
	UE_LOG(LogGenesis, Display, TEXT("Kind: %s"), *Name);
}

void AGenesisSceneSpeech::Play(FName LineId)
{
	const FString Path = FString::Printf(TEXT("/Game/Genesis/Audio/Speech/VO_%s.VO_%s"), *LineId.ToString(), *LineId.ToString());
	USoundWave* Sound = Cast<USoundWave>(FSoftObjectPath(Path).TryLoad());
	if (!Sound)
	{
		UE_LOG(LogGenesis, Warning, TEXT("Sprache fehlt: %s"), *Path);
		return;
	}
	if (GenesisSceneSpeechLogic::IsVocalization(LineId))
	{
		// Eigener Kanal: liegt über der Rede, unterbricht sie nicht
		Vocal = UGameplayStatics::SpawnSound2D(this, Sound, 0.85f);
		BusyVocal = Sound->GetDuration() + 0.5f;
	}
	else
	{
		if (Current)
		{
			Current->Stop();
		}
		Current = UGameplayStatics::SpawnSound2D(this, Sound);
		Busy = Sound->GetDuration() + GapSeconds;
	}
	LastLine = LineId;
	UE_LOG(LogGenesis, Display, TEXT("Sprache: %s (%s)"), *LineId.ToString(),
		GenesisSceneSpeechLogic::IsMother(LineId) ? TEXT("Mutter") : TEXT("Hebamme"));
}
