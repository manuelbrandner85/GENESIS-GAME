// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenesisBirthTypes.h"
#include "GenesisEarlyLifeTypes.h"
#include "GenesisSceneSpeech.generated.h"

class UAudioComponent;

/**
 * Was die Menschen im Kreißsaal sagen – und wann.
 *
 * Nichts ist ein Skript mit Zeitmarken: Jede Zeile hat einen Anlass in der Simulation (eine Wehe
 * beginnt, der Kopf tritt durch, die Schulter hängt, das Kind liegt auf der Haut, es schaut die Mutter an).
 * Die Aufnahmen sind echte deutsche Sprache (kie.ai, Gemini TTS; Tools/Audio/speech_lines.py).
 */
namespace GenesisSceneSpeechLogic
{
	/** Alles, was die Auswahl einer Zeile braucht – ohne Welt, damit sie testbar bleibt. */
	struct FInputs
	{
		EGenesisLaborStage Stage = EGenesisLaborStage::NotStarted;
		EGenesisBirthComplication Complication = EGenesisBirthComplication::None;
		float ContractionIntensity = 0.0f;
		float Descent = 0.0f;
		/** Sekunden (Echtzeit) seit der Geburt; negativ = noch nicht geboren. */
		float SecondsSinceBirth = -1.0f;
		bool bInBirthScene = false;
		bool bNewborn = false;
		bool bSkinToSkin = false;
		bool bEyeContact = false;
		bool bCrying = false;
		float RootingEffort = 0.0f;
		bool bAsleep = false;
		bool bGirl = true;
		/** Ist gerade niemand am Sprechen? Wenn nein, werden nur Ereignisse vermerkt, keine Zeile gewählt. */
		bool bCanSpeak = true;
		/** Ist der Kanal für Laute ohne Worte (Pressen, Stöhnen) frei? Er darf über der Rede liegen. */
		bool bCanVocalize = true;
	};

	/** Gedächtnis der Szene: was schon gesagt wurde, und wie die letzte Wehe war. */
	struct FMemory
	{
		TSet<FName> Said;
		TMap<FName, float> LastSaid;
		float Now = 0.0f;
		bool bWasContracting = false;
		int32 PushCount = 0;
		int32 LaborLineCount = 0;
		int32 EyeContacts = 0;
		bool bHadEyeContact = false;
		bool bWasSkin = false;
		/** Eine Zeile, deren Anlass kam, während noch jemand sprach – sie wartet kurz auf ihre Gelegenheit. */
		FName Pending;
		float PendingAt = 0.0f;
	};

	/**
	 * Wählt die nächste Zeile oder NAME_None. Sie merkt sich, was gewählt wurde.
	 * Variante für Mädchen/Jungen wird hier gewählt (Suffix _F/_M).
	 */
	GENESISSLICE_API FName Choose(const FInputs& In, FMemory& Memory);

	/** Spricht die Mutter (true) oder die Hebamme? Aus der ID (…_M_… / …_H_…). */
	GENESISSLICE_API bool IsMother(FName LineId);

	/** Laut ohne Worte, der über der Rede liegen darf (eigener Kanal). */
	GENESISSLICE_API bool IsVocalization(FName LineId);
}

UCLASS()
class GENESISSLICE_API AGenesisSceneSpeech : public AActor
{
	GENERATED_BODY()

public:
	AGenesisSceneSpeech();

	virtual void Tick(float DeltaSeconds) override;

	/** Pause nach jeder Zeile (s): Menschen reden unter der Geburt nicht ohne Luft zu holen. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Speech")
	float GapSeconds = 1.2f;

	/** Grenzfrequenz, wenn das Kind noch im Mutterleib hört (Hz): Bauchdecke und Fruchtwasser lassen nur Tiefen durch. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Speech")
	float WombCutoffHz = 450.0f;

	/** Zuletzt gesprochene Zeile – für die Anzeige und die Prüfung. */
	FName GetLastLine() const { return LastLine; }

private:
	GenesisSceneSpeechLogic::FInputs GatherInputs() const;
	void Play(FName LineId);
	bool IsGirl() const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Current;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Vocal;

	GenesisSceneSpeechLogic::FMemory Memory;
	float Busy = 0.0f;
	float BusyVocal = 0.0f;
	float BornAt = -1.0f;
	FName LastLine;
};
