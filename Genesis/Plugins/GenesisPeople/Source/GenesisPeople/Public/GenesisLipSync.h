// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenesisLipSync.generated.h"

class USoundWave;

/**
 * Lippen, Kiefer und Zunge zu einer gesprochenen Zeile – als Spur der MetaHuman-Steuerkurven
 * (CTRL_expressions_*), 50 Bilder je Sekunde, zeitgleich mit der Aufnahme.
 *
 * Erzeugt im Editor aus der Aufnahme selbst (UGenesisLipSyncLibrary::BakeLipSync): Ein neuronales Modell
 * von Epic (StreamingADA, das auch MetaHuman Animator nutzt) hört die Stimme und setzt sie in
 * Gesichtsbewegung um – Kieferöffnung, Lippenschluss bei m/b/p, gespitzte Lippen bei o/u, die Zunge bei l/t/d.
 * Im Spiel wird nur noch abgespielt: kein Netz, keine Rechenzeit, und jedes Mal genau dieselbe Bewegung.
 */
UCLASS(BlueprintType)
class GENESISPEOPLE_API UGenesisLipSyncTrack : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "LipSync")
	float FramesPerSecond = 50.0f;

	UPROPERTY(VisibleAnywhere, Category = "LipSync")
	int32 NumFrames = 0;

	/** Die Kurven, die in dieser Zeile wirklich etwas tun (Rohsteuerungen der MetaHuman-RigLogic). */
	UPROPERTY(VisibleAnywhere, Category = "LipSync")
	TArray<FName> CurveNames;

	/** Werte Bild für Bild: [Frame * CurveNames.Num() + Kurve]. */
	UPROPERTY()
	TArray<float> Values;

	float GetDuration() const { return FramesPerSecond > 0.0f ? NumFrames / FramesPerSecond : 0.0f; }

	/** Wert jeder Kurve zum Zeitpunkt Seconds (linear zwischen den Bildern), mal Weight, in Out geschrieben. */
	void Sample(float Seconds, float Weight, TMap<FName, float>& Out) const;

	/** Höchster Wert einer Kurve über die ganze Zeile (für Prüfung und Anzeige). */
	float PeakOf(FName Curve) const;
};

namespace GenesisLipSyncLogic
{
	/**
	 * Wie stark die Spur gerade wirkt: blendet am Anfang und Ende weich ein und aus, damit das Gesicht nicht
	 * aus der Ruhe in die erste Silbe springt oder am Ende in die Ruhe zurückschnappt.
	 */
	GENESISPEOPLE_API float Envelope(float Seconds, float Duration, float FadeSeconds = 0.1f);

	/** Kurven, die die Figur selbst führt (Lidschlag, Blick) – die Sprachspur lässt sie in Ruhe. */
	GENESISPEOPLE_API bool IsOwnedByCharacter(FName RawCurve);
}

/**
 * Wer gerade spricht und wie weit er ist. Hängt an einer Figur (Mutter, Hebamme) und liefert pro Bild die Kurven.
 * Zeitbasis ist die Audio-Uhr der Welt – dieselbe, nach der der Ton läuft –, damit Mund und Stimme nicht auseinanderlaufen.
 */
USTRUCT()
struct GENESISPEOPLE_API FGenesisLipSyncPlayer
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<const UGenesisLipSyncTrack> Track;

	double StartAudioTime = 0.0;

	void Start(const UGenesisLipSyncTrack* InTrack, double AudioTimeNow) { Track = InTrack; StartAudioTime = AudioTimeNow; }
	void Stop() { Track = nullptr; }
	bool IsSpeaking(double AudioTimeNow) const;

	/** Kurven für jetzt; leert Out, wenn niemand spricht. */
	void Evaluate(double AudioTimeNow, TMap<FName, float>& Out);
};

UCLASS()
class GENESISPEOPLE_API UGenesisLipSyncLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Nur im Editor: hört die Aufnahme ab und speichert die Gesichtsbewegung als UGenesisLipSyncTrack unter AssetPath
	 * (z. B. /Game/Genesis/Audio/LipSync/LS_H_Da). Das Modell schaut 80 ms voraus; um diese Zeit wird die Spur
	 * zurückgeschoben, damit der Mund sich öffnet, wenn der Laut kommt – und nicht danach.
	 */
	UFUNCTION(BlueprintCallable, Category = "Genesis|LipSync")
	static bool BakeLipSync(USoundWave* Sound, const FString& AssetPath, float LookaheadMs = 80.0f);
};
