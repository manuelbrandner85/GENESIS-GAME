// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GenesisVoiceTypes.h"
#include "GenesisVoiceSubsystem.generated.h"

struct FGenesisBodyState;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FGenesisOnUtterance, const FGuid& /*Speaker*/, const FGenesisUtterance& /*Utterance*/, const FGenesisVoiceProfile& /*Profile*/);

/**
 * Wer in dieser Welt eine Stimme hat – und was sie gerade von sich gibt.
 *
 * Das Stimmprofil wird nicht eingestellt, sondern aus dem Körper berechnet: Alter, Geschlecht,
 * Körpergröße, Gesundheit von Kehlkopf und Lunge, Erschöpfung. Ändert sich der Körper, ändert sich
 * die Stimme – ein Kind, das wächst, klingt jedes Jahr anders, und zwar ohne dass jemand etwas umstellt.
 */
UCLASS()
class GENESISVOICE_API UGenesisVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Die Stimme dieser Person – aus dem Körper berechnet und bis zur nächsten Änderung gemerkt. */
	const FGenesisVoiceProfile& GetProfile(const FGuid& EntityId);

	/** Profil ohne Körper (Entwicklerbefehle, Platzhalterfiguren). */
	FGenesisVoiceProfile BuildProfile(const FGenesisVoiceInputs& Inputs) const;

	/**
	 * Die Stimme der Mutter. PLATZHALTER: Solange die Mutter keine Person der Simulation ist,
	 * steht hier eine erschöpfte Dreißigjährige – Alter und Zustand sind angenommen, nicht simuliert.
	 */
	const FGenesisVoiceProfile& GetMotherVoice();

	/** Feste Kennung der Platzhalter-Mutter, damit Befehle und Actors dieselbe Stimme meinen. */
	static FGuid GetMotherId();

	/** Jemand macht einen Laut. Wer zuhört, entscheidet die Komponente. */
	void Say(const FGuid& Speaker, const FGenesisUtterance& Utterance);

	/** Vergisst ein gemerktes Profil – nötig, wenn der Körper sich deutlich verändert hat. */
	void InvalidateProfile(const FGuid& EntityId);

	FGenesisOnUtterance OnUtterance;

	UPROPERTY(EditAnywhere, Category = "Genesis|Voice")
	FGenesisVoiceTuning Tuning;

private:
	FGenesisVoiceInputs MakeInputsFromBody(const FGenesisBodyState& Body) const;
	void RegisterDebugPage();

	TMap<FGuid, FGenesisVoiceProfile> Profiles;
	FGenesisVoiceProfile MotherVoice;
	bool bMotherVoiceBuilt = false;
	bool bDebugPageRegistered = false;

	/** Nur für die Anzeige: was zuletzt gesagt wurde. */
	FString LastUtterance;
};
