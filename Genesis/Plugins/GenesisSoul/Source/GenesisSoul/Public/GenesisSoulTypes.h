// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenesisTypes.h"
#include "GenesisSoulTypes.generated.h"

/**
 * Zustand eines Seelen-Echos.
 * Das Karma-Gericht entscheidet nicht über gut/böse, sondern darüber, ob ein Thema abgeschlossen ist.
 */
UENUM(BlueprintType)
enum class EGenesisEchoState : uint8
{
	/** Offenes Thema – wirkt als Narbe: sucht in späteren Leben häufiger thematisch ähnliche Situationen. */
	Open,
	/** Abgeschlossenes Thema – wirkt als Fundament: stabilisiert, öffnet Möglichkeiten. */
	Integrated
};

/**
 * Resonanz: ein tiefes Muster der Seele, unabhängig von DNA und Kultur.
 * Beispiele: unerklärliche Vorliebe für das Meer, Angst vor Tiefe, eine bestimmte Melodie, wiederkehrender Traum.
 * Das Muster ist ein Gameplay Tag unter Genesis.Soul.Pattern.* (Inhalte definieren die konkreten Blätter).
 */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulResonance
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Pattern;

	/** 0..1 – wie stark das Muster in der aktuellen Inkarnation durchscheint. */
	UPROPERTY()
	float Intensity = 0.0f;

	/** Inkarnation, in der das Muster entstanden ist. */
	UPROPERTY()
	int32 OriginIncarnation = 0;

	/** Letzte Inkarnation, in der das Muster erlebt und verstärkt wurde. */
	UPROPERTY()
	int32 LastReinforcedIncarnation = 0;

	UPROPERTY()
	int32 ReinforcementCount = 0;
};

/** Echo: ein Lebensthema, das über den Tod hinaus mitgenommen wird (Ergebnis des Karma-Gerichts). */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulEcho
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid EchoId;

	/** Thema, z. B. Genesis.Theme.Abandonment. */
	UPROPERTY()
	FGameplayTag Theme;

	UPROPERTY()
	EGenesisEchoState State = EGenesisEchoState::Open;

	/** 0..1 – wie stark das Thema nach Ausdruck in späteren Leben drängt. */
	UPROPERTY()
	float Weight = 0.0f;

	UPROPERTY()
	int32 SourceIncarnation = 0;

	/** Ursprungsereignis im Kausalgraph (für die Kosmische Bibliothek). */
	UPROPERTY()
	FGuid SourceEventId;

	/** Seelen, die an diesem Thema beteiligt waren. */
	UPROPERTY()
	TArray<FGuid> RelatedSoulIds;

	/** Wie oft das Thema in späteren Leben erneut auftrat. */
	UPROPERTY()
	int32 ManifestationCount = 0;

	UPROPERTY()
	int32 LastTouchedIncarnation = 0;
};

/** Bindung zu einer anderen Seele – Grundlage für Seelen-Begegnungen (Déjà-vu, Musik, Traum, Gefühl). */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulBond
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid OtherSoulId;

	/** 0..1 */
	UPROPERTY()
	float Strength = 0.0f;

	/** Themen, die beide Seelen geteilt haben (Liebe, Verlust, Konflikt …). */
	UPROPERTY()
	FGameplayTagContainer SharedThemes;

	UPROPERTY()
	int32 FirstIncarnation = 0;

	UPROPERTY()
	int32 LastIncarnation = 0;

	UPROPERTY()
	int32 SharedLifetimes = 0;
};

/**
 * Musikalisches Motiv der Seele. Wird von GenesisAudio (MetaSounds) interpretiert.
 * Die ersten zwei Intervalle sind der unveränderliche Identitätskern – der Rest entwickelt sich über Leben.
 */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulMotif
{
	GENERATED_BODY()

	/** Intervalle in Halbtönen relativ zur vorherigen Note. */
	UPROPERTY()
	TArray<int32> Intervals;

	/** Notenlängen in Sechzehnteln. */
	UPROPERTY()
	TArray<int32> Durations;

	/** 0 = Ionisch … 6 = Lokrisch. */
	UPROPERTY()
	int32 ModeIndex = 0;

	/** 0..1 – steigt mit offenen Echos. */
	UPROPERTY()
	float Tension = 0.0f;

	/** 0..1 – steigt mit Liebe und integrierten Themen. */
	UPROPERTY()
	float Warmth = 0.5f;

	/** Anzahl der Weiterentwicklungen (= abgeschlossene Leben). */
	UPROPERTY()
	int32 Variation = 0;

	static constexpr int32 IdentityCoreLength = 2;
};

/** Datensatz einer Inkarnation. Die Seele kennt Körper und Welt nur über IDs. */
USTRUCT()
struct GENESISSOUL_API FGenesisIncarnationRecord
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Index = 0;

	/** Person in der Welt (GenesisLifeSimulation/GenesisNPC). */
	UPROPERTY()
	FGuid EntityId;

	/** Genom des Körpers (GenesisGenetics) – nur als Referenz, keine Abhängigkeit. */
	UPROPERTY()
	FGuid GenomeId;

	UPROPERTY()
	FGuid WorldId;

	UPROPERTY()
	FGenesisTimestamp BirthTime;

	UPROPERTY()
	FGenesisTimestamp DeathTime;

	UPROPERTY()
	FGameplayTag Epoch;

	UPROPERTY()
	FGameplayTag Culture;

	UPROPERTY()
	FGameplayTag DeathKind;

	UPROPERTY()
	FGameplayTag AfterlifeRealm;

	UPROPERTY()
	bool bCompleted = false;
};

/**
 * Soul Seed – der persistente Kern einer Seele.
 * Getrennt von DNA: Körper, Familie, Kultur und Epoche wechseln, der Soul Seed bleibt.
 */
USTRUCT()
struct GENESISSOUL_API FGenesisSoulSeed
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid SoulId;

	/** Ursprünglicher Seed – alle Ableitungen dieser Seele sind daraus reproduzierbar. */
	UPROPERTY()
	uint64 OriginSeed = 0;

	UPROPERTY()
	TArray<FGenesisSoulResonance> Resonances;

	UPROPERTY()
	TArray<FGenesisSoulEcho> Echoes;

	UPROPERTY()
	TArray<FGenesisSoulBond> Bonds;

	UPROPERTY()
	FGenesisSoulMotif Motif;

	UPROPERTY()
	TArray<FGenesisIncarnationRecord> Incarnations;

	/** 0..1 – Fähigkeit loszulassen (Zugang zu "Das Nichts"). */
	UPROPERTY()
	float Detachment = 0.0f;

	bool IsValid() const { return SoulId.IsValid(); }

	/** Index der aktuellen (letzten) Inkarnation oder INDEX_NONE. */
	int32 GetCurrentIncarnationIndex() const { return Incarnations.Num() > 0 ? Incarnations.Last().Index : INDEX_NONE; }

	/** true, wenn die letzte Inkarnation noch nicht abgeschlossen ist. */
	bool IsIncarnated() const { return Incarnations.Num() > 0 && !Incarnations.Last().bCompleted; }
};

/** Ein vom Karma-Gericht ausgewähltes Thema (1–3 pro Leben). */
USTRUCT(BlueprintType)
struct GENESISSOUL_API FGenesisLifeClosureTheme
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGameplayTag Theme;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGuid SourceEventId;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	float Intensity = 0.5f;

	/** Abgeschlossen → Fundament. Offen → Narbe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	bool bResolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	TArray<FGuid> RelatedSoulIds;
};

/** Intensive Beziehung zu einer anderen Seele in diesem Leben. */
USTRUCT(BlueprintType)
struct GENESISSOUL_API FGenesisBondExperience
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGuid OtherSoulId;

	/** 0..1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	float Intensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGameplayTagContainer Themes;
};

/** Alles, was ein Leben beim Tod an die Seele übergibt. */
USTRUCT(BlueprintType)
struct GENESISSOUL_API FGenesisLifeClosure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGenesisTimestamp DeathTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGameplayTag DeathKind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	FGameplayTag AfterlifeRealm;

	/** Vom Karma-Gericht gewählt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	TArray<FGenesisLifeClosureTheme> CourtThemes;

	/** In diesem Leben erlebte Muster (verstärken Resonanzen). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	TArray<FGenesisWeightedTag> ExperiencedPatterns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	TArray<FGenesisBondExperience> Bonds;

	/** 0..1 – wie viel Liebe/Wärme dieses Leben trug (formt das Motiv). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	float Warmth = 0.5f;

	/** 0..1 – wie sehr am Lebensende losgelassen wurde. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Genesis|Soul")
	float LettingGo = 0.0f;
};

/** Regeln für den Übertrag zwischen Leben. Designer-tunebar über UGenesisSoulSettings. */
USTRUCT(BlueprintType)
struct GENESISSOUL_API FGenesisSoulCarryOverParams
{
	GENERATED_BODY()

	/** Intensitäts-Faktor pro Leben für nicht erneut erlebte Resonanzen. */
	UPROPERTY(EditAnywhere, Category = "Resonance", meta = (ClampMin = "0", ClampMax = "1"))
	float ResonancePersistence = 0.6f;

	/** Anteil des verbleibenden Raums bis 1, den eine volle Verstärkung auffüllt. */
	UPROPERTY(EditAnywhere, Category = "Resonance", meta = (ClampMin = "0", ClampMax = "1"))
	float ResonanceReinforcementGain = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Resonance", meta = (ClampMin = "0", ClampMax = "1"))
	float ResonanceRemovalThreshold = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Resonance", meta = (ClampMin = "1"))
	int32 MaxResonances = 32;

	/** Offene Themen verblassen langsam … */
	UPROPERTY(EditAnywhere, Category = "Echo", meta = (ClampMin = "0", ClampMax = "1"))
	float OpenEchoPersistence = 0.85f;

	/** … integrierte schneller (sie haben ihren Zweck erfüllt). */
	UPROPERTY(EditAnywhere, Category = "Echo", meta = (ClampMin = "0", ClampMax = "1"))
	float IntegratedEchoPersistence = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Echo", meta = (ClampMin = "0", ClampMax = "1"))
	float EchoGain = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Echo", meta = (ClampMin = "0", ClampMax = "1"))
	float EchoRemovalThreshold = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Echo", meta = (ClampMin = "1"))
	int32 MaxEchoes = 24;

	UPROPERTY(EditAnywhere, Category = "Bond", meta = (ClampMin = "0", ClampMax = "1"))
	float BondPersistence = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Bond", meta = (ClampMin = "0", ClampMax = "1"))
	float BondGain = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Bond", meta = (ClampMin = "0", ClampMax = "1"))
	float BondRemovalThreshold = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Bond", meta = (ClampMin = "1"))
	int32 MaxBonds = 48;

	/** Rate, mit der Loslassen am Lebensende die Detachment-Fähigkeit erhöht. */
	UPROPERTY(EditAnywhere, Category = "Detachment", meta = (ClampMin = "0", ClampMax = "1"))
	float DetachmentGain = 0.2f;
};
