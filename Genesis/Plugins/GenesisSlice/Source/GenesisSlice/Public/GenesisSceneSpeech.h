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
		/** 0..1 – wie laut das Kind gerade schreit (aus dem Zustand der ersten Stunde). */
		float CryLoudness = 0.0f;
		/** Die ersten Atemzüge (Stufe FirstBreaths) – der erste Schrei. */
		bool bFirstBreaths = false;
		/** Die Hebamme rubbelt das Kind gerade trocken / es liegt zugedeckt. */
		bool bBeingDried = false;
		bool bCovered = false;
	};

	/**
	 * Was das Kind gerade von sich gibt (echte Aufnahmen, eigener Kanal) oder NAME_None.
	 * SFX_Schrei_Erster bei den ersten Atemzügen, SFX_Schrei_Stark / SFX_Schrei_Wimmern je nach Schreilautstärke,
	 * SFX_Baby_Laute ruhig auf der Haut der Mutter (selten). bChildFree: Läuft gerade kein Laut des Kindes?
	 */
	GENESISSLICE_API FName ChooseChild(const FInputs& In, float Now, float& InOutLastQuietSound, bool& bInOutFirstCryDone, bool bChildFree);

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

	/** Zum Prüfen (Konsole genesis.Speech.Say <Zeile>): spricht die Zeile sofort, mit Richtung und Mund. */
	void Say(FName LineId) { Play(LineId); }

	/**
	 * Das Ohr des Neugeborenen: In den ersten Stunden stecken noch Fruchtwasser und Käseschmiere in Gehörgang
	 * und Mittelohr (deshalb fällt das Hörscreening in den ersten 24 h häufiger durch). Hohe Töne kommen
	 * gedämpft an und werden in der ersten Lebensstunde klarer. Grenzfrequenz bei der Geburt / nach 60 min (Hz).
	 */
	UPROPERTY(EditAnywhere, Category = "Genesis|Speech")
	float NewbornCutoffAtBirthHz = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "Genesis|Speech")
	float NewbornCutoffAfterHourHz = 12000.0f;

	/** Die Hebamme: wo ihre Stimme herkommt (vom Actor der Hebamme gesetzt, sobald es ihn gibt). */
	void SetMidwifeLocation(const FVector& Location) { MidwifeLocation = Location; bHasMidwife = true; }

	/** Grenzfrequenz des Hörens beim Kind gerade jetzt (Hz) – für Anzeige und Test. */
	static float NewbornCutoff(float MinutesSinceBirth, float AtBirthHz, float AfterHourHz);

private:
	GenesisSceneSpeechLogic::FInputs GatherInputs() const;
	void Play(FName LineId);

	/** Die Figur, die LineId spricht, bewegt den Mund nach Track (nullptr = sie verstummt). */
	void SpeakerSpeak(FName LineId, const class UGenesisLipSyncTrack* Track);

	/** Zeile auf dem Redekanal (nicht die Laute) – um beim Unterbrechen den richtigen Mund anzuhalten. */
	FName CurrentLine;
	bool IsGirl() const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Current;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Vocal;

	/** Lautstärke des Raumklangs (nachts im Kreißsaal). Unter der Sprache, nie davor. */
	UPROPERTY(EditAnywhere, Category = "Genesis|Speech")
	float RoomVolume = 0.35f;

	void PlayChild(FName SoundId);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ChildAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Room;

	/** Das Tuch beim Abrubbeln – ganz nah, am eigenen Körper, eigener Kanal. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Care;
	bool bWasDrying = false;

	/** Wo spricht diese Person gerade? Vor der Geburt ohne Richtung (durch die Bauchdecke). */
	bool SpeakerLocation(FName LineId, FVector& OutLocation) const;

	UPROPERTY(Transient)
	TObjectPtr<class USoundAttenuation> Attenuation;

	FVector MidwifeLocation = FVector::ZeroVector;
	bool bHasMidwife = false;

	GenesisSceneSpeechLogic::FMemory Memory;
	float Busy = 0.0f;
	float BusyVocal = 0.0f;
	float ChildBusy = 0.0f;
	float LastQuietSound = -100.0f;
	bool bFirstCryDone = false;
	int32 ChildVariant = 0;
	FName LastChildSound;
	float BornAt = -1.0f;
	FName LastLine;
};
