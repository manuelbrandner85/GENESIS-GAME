// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisEmbryoTypes.generated.h"

/**
 * Die erste Woche. Die Stufen folgen der menschlichen Embryonalentwicklung:
 * Aus einer Zelle werden zwei, vier, acht; ab acht Zellen rücken sie zusammen (Kompaktierung),
 * es bildet sich ein Hohlraum, die Zona reißt auf, und der Keim nistet sich ein.
 */
UENUM(BlueprintType)
enum class EGenesisEmbryoStage : uint8
{
	/** Eine Zelle, zwei Vorkerne – das Erbgut beider Eltern liegt noch getrennt. */
	Zygote,
	/** Furchung: Die Zellen teilen sich, ohne zu wachsen. Der Keim bleibt so groß wie die Eizelle. */
	Cleavage,
	/** Ab ~8 Zellen verzahnen sich die Zellen; aus einem Zellhaufen wird ein Verband (Morula). */
	Morula,
	/** Im Inneren sammelt sich Flüssigkeit: Blastozyste mit Embryoblast und Trophoblast. */
	Blastocyst,
	/** Die Blastozyste dehnt sich, die Zona wird dünn und reißt – der Keim schlüpft. */
	Hatching,
	/** Der Trophoblast dringt in die Gebärmutterschleimhaut ein. */
	Implanting,
	/** Eingenistet: Ab hier führt die Körpersimulation die Schwangerschaft weiter. */
	Implanted,
	/** Entwicklungsstillstand – der Keim teilt sich nicht mehr weiter. */
	Arrested
};

/** Warum ein Keim stehen geblieben ist. Wird im Entwickler-HUD angezeigt und geht in die Biografie ein. */
UENUM(BlueprintType)
enum class EGenesisEmbryoArrestReason : uint8
{
	None,
	/** Fehlverteilung der Chromosomen – der häufigste Grund, dass ein Keim nicht weiterkommt. */
	Aneuploidy,
	/** Zu wenig Energie: Die Mitochondrien stammen allein aus der Eizelle. */
	EnergyFailure,
	/** Zu starke Fragmentierung nach unsauberen Teilungen. */
	Fragmentation,
	/** Die Einnistung ist nicht gelungen. */
	ImplantationFailed
};

/**
 * Eine Zelle des Keims (Blastomere). Positionen in µm relativ zur Mitte,
 * damit Darstellung und Simulation dieselben Zahlen benutzen (1 µm = 1 Unreal-Einheit).
 */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisBlastomere
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	FVector Position = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float RadiusUm = 55.0f;

	/** Teilungsrunde, aus der die Zelle stammt (0 = Zygote). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	int32 Generation = 0;

	/** Ab der Blastozyste: Gehört die Zelle zum Embryoblast (daraus wird der Mensch)? */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	bool bInnerCellMass = false;

	/** 0..1 – eigener Farbton für die Darstellung (deterministisch). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Tint = 0.5f;

	/** Stunden bis zur nächsten Teilung dieser Zelle (Teilungen laufen nicht synchron). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float HoursToDivision = 0.0f;
};

/** Zustand des Keims. Persistiert; die Zellliste ist die Grundlage der Darstellung. */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisEmbryoState
{
	GENERATED_BODY()

	UPROPERTY() FGuid EntityId;
	UPROPERTY() FGuid GenomeId;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	EGenesisEmbryoStage Stage = EGenesisEmbryoStage::Zygote;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	EGenesisEmbryoArrestReason ArrestReason = EGenesisEmbryoArrestReason::None;

	/** Stunden seit der Verschmelzung – die Uhr der ersten Woche. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	double HoursSinceFusion = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	TArray<FGenesisBlastomere> Cells;

	/** 0..1 – wie sauber die Teilungen laufen. Geht später in die Organentwicklung ein. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Quality = 1.0f;

	/** 0..1 – Zelltrümmer aus unsauberen Teilungen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Fragmentation = 0.0f;

	/** 0..1 – Fortschritt der Kompaktierung (die Zellen verzahnen sich). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Compaction = 0.0f;

	/** 0..1 – Anteil des Hohlraums (Blastozöl) am Innenvolumen. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Cavity = 0.0f;

	/** Dicke der Zona pellucida (µm). Sie wird beim Ausdehnen dünner, bis der Keim schlüpft. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float ZonaThicknessUm = 14.0f;

	/** 0..1 – Fortschritt der Einnistung in die Gebärmutterschleimhaut. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	float Implantation = 0.0f;

	/** Zufallsstrom des Keims (deterministisch aus dem Genom). */
	UPROPERTY() uint64 Seed = 0;

	/** Lebenskraft und Widerstandskraft aus der Zeugung – sie bestimmen, wie robust die Teilungen laufen. */
	UPROPERTY() float Vitality = 0.75f;
	UPROPERTY() float Resilience = 0.6f;

	/** Der Keim des Spielers darf nicht stillstehen – sonst gäbe es kein Leben zu spielen. */
	UPROPERTY() bool bPlayerEmbryo = true;

	UPROPERTY() FGenesisTimestamp FusionTime;

	int32 GetCellCount() const { return Cells.Num(); }
	bool IsAlive() const { return Stage != EGenesisEmbryoStage::Arrested; }
	/** Der Radius, den der Keim insgesamt einnimmt (µm) – ohne Zona. */
	float GetOuterRadiusUm() const { return 58.0f * (1.0f + 0.35f * Cavity); }
};

/**
 * Stellschrauben der ersten Woche. Die Zeiten sind Stunden nach der Verschmelzung und folgen
 * der menschlichen Entwicklung: erste Teilung nach gut einem Tag, dann alle 12–20 Stunden,
 * Kompaktierung am dritten Tag, Blastozyste am vierten bis fünften, Schlüpfen am fünften bis sechsten,
 * Einnistung ab Tag sieben.
 */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisEmbryoTuning
{
	GENERATED_BODY()

	/** Stunden bis zur ersten Furchungsteilung. */
	UPROPERTY(EditAnywhere, Category = "Timing") FFloatInterval FirstCleavageHours = FFloatInterval(24.0f, 30.0f);

	/** Stunden zwischen den weiteren Teilungen einer Zelle. */
	UPROPERTY(EditAnywhere, Category = "Timing") FFloatInterval CleavageIntervalHours = FFloatInterval(11.0f, 17.0f);

	/** Ab dieser Zellzahl beginnt die Kompaktierung. */
	UPROPERTY(EditAnywhere, Category = "Timing") int32 CompactionCellCount = 8;

	/** Dauer der Kompaktierung (h). */
	UPROPERTY(EditAnywhere, Category = "Timing") float CompactionHours = 14.0f;

	/** Ab dieser Zellzahl bildet sich der Hohlraum. */
	UPROPERTY(EditAnywhere, Category = "Timing") int32 CavitationCellCount = 16;

	/** Dauer bis zur voll ausgedehnten Blastozyste (h). */
	UPROPERTY(EditAnywhere, Category = "Timing") float ExpansionHours = 34.0f;

	/** Ab diesem Ausdehnungsgrad reißt die Zona. */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0", ClampMax = "1")) float HatchingCavity = 0.95f;

	/** Dauer der Einnistung (h). */
	UPROPERTY(EditAnywhere, Category = "Timing") float ImplantationHours = 48.0f;

	/** Zellen teilen sich nicht mehr, wenn sie so klein geworden sind (µm). */
	UPROPERTY(EditAnywhere, Category = "Geometry") float MinimumBlastomereRadiusUm = 8.0f;

	/** Innenradius der Zona (µm) – dort hinein passen alle Zellen. */
	UPROPERTY(EditAnywhere, Category = "Geometry") float InnerRadiusUm = 58.0f;

	/**
	 * Wahrscheinlichkeit je Teilung, dass der Keim stehen bleibt (0..1), bei mittlerer Lebenskraft.
	 * Biologisch kommt nur etwa die Hälfte aller befruchteten Eizellen bis zur Einnistung.
	 */
	UPROPERTY(EditAnywhere, Category = "Risk", meta = (ClampMin = "0", ClampMax = "1")) float ArrestChancePerDivision = 0.045f;

	/**
	 * Bis zu dieser Zellzahl entscheidet sich, ob ein Keim weiterkommt.
	 * Dort schaltet das eigene Erbgut die Entwicklung an (Genomaktivierung im 4- bis 8-Zell-Stadium);
	 * danach ist ein Stillstand selten. Ohne dieses Fenster stiege das Risiko über 40 Teilungen auf über 90 Prozent.
	 */
	UPROPERTY(EditAnywhere, Category = "Risk") int32 RiskWindowCellCount = 16;

	/** Anteil Zelltrümmer je Teilung bei schwacher Lebenskraft (ebenfalls nur im Risikofenster). */
	UPROPERTY(EditAnywhere, Category = "Risk", meta = (ClampMin = "0", ClampMax = "1")) float FragmentationPerDivision = 0.06f;
};
