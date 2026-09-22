// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisTypes.h"
#include "GenesisEmbryogenesisTypes.h"
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
 * Die zweite Woche (GENESIS-040): Einnistung und zweiblättrige Keimscheibe, Carnegie-Stadien 5 und 6.
 * Die Zeiten der Stufen sind Tage nach der Befruchtung (Langman, Moore „The Developing Human", Hertig-Rock-Präparate).
 */
UENUM(BlueprintType)
enum class EGenesisImplantationPhase : uint8
{
	/** Noch frei in der Gebärmutter. */
	None,
	/** Tag 6: Die geschlüpfte Blastozyste legt sich mit dem Embryoblast-Pol an die Schleimhaut. */
	Apposition,
	/** Tag 6,5: Fest verhaftet (Integrine, L-Selektin) – sie rollt nicht mehr ab. */
	Adhesion,
	/** Tag 7–8: Der Trophoblast teilt sich in Zyto- und Synzytiotrophoblast; das Synzytium frisst sich ins Stroma. */
	Invasion,
	/** Tag 9: Im Synzytium öffnen sich Lakunen, der Keim ist fast ganz versunken, ein Fibrinpfropf verschließt die Lücke. */
	Lacunar,
	/** Tag 10: Ganz in der Schleimhaut, die Oberfläche wächst darüber wieder zu. */
	Embedded,
	/** Tag 11–12: Das Synzytium öffnet mütterliche Kapillaren – Blut strömt durch die Lakunen (erster Kreislauf). */
	Uteroplacental,
	/** Tag 13: Primärzotten aus Zytotrophoblast wachsen ins Synzytium, sekundärer Dottersack. Die zweite Woche ist vorbei. */
	PrimaryVilli
};

/**
 * Zustand der Einnistung. Längen in µm wie der übrige Keim. Der Keim ist hier weit größer als die Zellen,
 * die ihn bilden – ab jetzt beschreiben Gewebe (Schichten, Höhlen) den Keim, nicht mehr einzelne Zellen.
 */
USTRUCT(BlueprintType)
struct GENESISEMBRYO_API FGenesisImplantationState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") EGenesisImplantationPhase Phase = EGenesisImplantationPhase::None;

	/** Stunden nach der Verschmelzung, zu denen sich der Keim anlegte (0 = noch nicht). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float AppositionAtHours = 0.0f;

	/** Durchmesser des ganzen Keims (Trophoblast bis Trophoblast, µm): 200 beim Anlegen, ~1,2 mm an Tag 13. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float ConceptusDiameterUm = 0.0f;

	/** 0..1 – Anteil des Keims unter der Oberfläche der Schleimhaut. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float Embedded = 0.0f;

	/** Wie tief der Keim ins Stroma reicht (µm unter der Oberfläche). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float DepthUm = 0.0f;

	/** Dicke des Synzytiotrophoblasten am Embryonalpol (µm). Er ist das eindringende Gewebe. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float SyncytiumThicknessUm = 0.0f;

	/** Lakunen im Synzytium und wie weit sie mit mütterlichem Blut gefüllt sind (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 Lacunae = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float LacunarBlood = 0.0f;

	/** 0..1 – der Verschluss der Oberfläche: erst Fibrinpfropf (bis 0,5), dann nachgewachsenes Epithel (1). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float SurfaceClosure = 0.0f;

	/** 0..1 – Deziduareaktion: Stromazellen schwellen, speichern Glykogen und Fett, Kapillaren weiten sich. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float Decidualization = 0.0f;

	/** Zweiblättrige Keimscheibe: Epiblast (hochprismatisch, daraus wird der ganze Mensch) und Hypoblast (kubisch). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 EpiblastCells = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 HypoblastCells = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float DiscDiameterUm = 0.0f;

	/** 0..1 – Amnionhöhle (öffnet sich an Tag 8 im Epiblast), primärer Dottersack (Tag 9, Heuser-Membran), sekundärer (Tag 12–13). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float AmnioticCavity = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float PrimaryYolkSac = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float SecondaryYolkSac = 0.0f;

	/** 0..1 – extraembryonales Mesoderm und die Chorionhöhle, die sich darin öffnet (Tag 11–13). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float ExtraembryonicMesoderm = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float ChorionicCavity = 0.0f;

	/** Primärzotten (Tag 13). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") int32 PrimaryVilli = 0;

	/** hCG im Blut der Mutter (mIU/ml). Hält den Gelbkörper am Leben – ohne hCG bliebe die Regel nicht aus. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float HcgMilliIU = 0.0f;

	/** Tag nach dem Eisprung, an dem hCG erstmals messbar wird – die Größe, nach der Wilcox das Risiko misst. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float ImplantationDayPostOvulation = 0.0f;

	/** Wahrscheinlichkeit, dass diese Schwangerschaft früh endet (0..1), aus dem Zeitpunkt der Einnistung. */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo") float EarlyLossRisk = 0.0f;

	/** Einmal ausgewürfelt beim Anheften: Kommt das Synzytium nicht an mütterliches Blut? */
	UPROPERTY() bool bRiskRolled = false;
	UPROPERTY() bool bWillFail = false;
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

	/** Die zweite Woche im Einzelnen (GENESIS-040). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	FGenesisImplantationState Nidation;

	/** Die dritte und vierte Woche: Keimblätter, Neuralrohr, erster Herzschlag (GENESIS-041). */
	UPROPERTY(BlueprintReadOnly, Category = "Genesis|Embryo")
	FGenesisEmbryogenesisState Embryogenesis;

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

	/**
	 * Zeiten gegen die Mediane aus IVF-Zeitraffer-Aufnahmen gesetzt (340 Keime, HROpen 2024; Docs/26):
	 * t2 25,8 · t3 36,9 · t4 38,3 · t8 58,7 · Kompaktierung 80,2 · Morula 88,9 · Blastulation 99,0 ·
	 * volle Blastozyste 109,9 · Schlüpfen ab 116 h. Das Modell trifft jede Marke auf gut 2 h
	 * (Test Genesis.Embryo.ClinicalTimings). Vorher kamen Kompaktierung und Blastozyste fast einen Tag zu früh.
	 *
	 * Der zweite Zellzyklus (2 → 4) ist kurz, ab dem dritten (4 → 8) werden die Zyklen deutlich länger –
	 * dort übernimmt das eigene Erbgut des Keims die Steuerung.
	 */
	UPROPERTY(EditAnywhere, Category = "Timing") FFloatInterval CleavageIntervalHours = FFloatInterval(10.0f, 13.0f);

	/** Zellzyklus ab der dritten Teilungsrunde (h). */
	UPROPERTY(EditAnywhere, Category = "Timing") FFloatInterval LaterCleavageIntervalHours = FFloatInterval(15.0f, 20.0f);

	/** Ab dieser Zellzahl beginnt die Kompaktierung (klinisch um 80 h, zwischen 8 und 16 Zellen). */
	UPROPERTY(EditAnywhere, Category = "Timing") int32 CompactionCellCount = 15;

	/** Dauer der Kompaktierung (h): 80,2 → 88,9 h. */
	UPROPERTY(EditAnywhere, Category = "Timing") float CompactionHours = 9.0f;

	/** Ab dieser Zellzahl bildet sich der Hohlraum. */
	UPROPERTY(EditAnywhere, Category = "Timing") int32 CavitationCellCount = 28;

	/** Dauer bis zur voll ausgedehnten Blastozyste (h). Hälfte (volle Blastozyste) nach 11 h. */
	UPROPERTY(EditAnywhere, Category = "Timing") float ExpansionHours = 22.0f;

	/** Vorkerne sichtbar ab (h, Median 8,3) bis zu ihrer Auflösung (h, Median 23,3). */
	UPROPERTY(EditAnywhere, Category = "Timing") float PronucleiAppearHours = 8.3f;
	UPROPERTY(EditAnywhere, Category = "Timing") float PronucleiFadeHours = 23.3f;

	/** Vor einer Teilung löst sich die Kernhülle auf; so lange ist kein Kern zu sehen (h). */
	UPROPERTY(EditAnywhere, Category = "Timing") float MitosisHours = 1.5f;

	/** Ab diesem Ausdehnungsgrad reißt die Zona. */
	UPROPERTY(EditAnywhere, Category = "Timing", meta = (ClampMin = "0", ClampMax = "1")) float HatchingCavity = 0.95f;

	/**
	 * Die zweite Woche (GENESIS-040), Stunden nach der Befruchtung beim mittleren Keim. Ein Keim, der später
	 * schlüpft, legt sich später an – alle Stufen verschieben sich mit. Tag 6 = 144 h.
	 */
	UPROPERTY(EditAnywhere, Category = "Implantation") float NominalAppositionHours = 144.0f;
	/** Nach dem Schlüpfen treibt der Keim noch einige Stunden frei, bis er sich anlegt. */
	UPROPERTY(EditAnywhere, Category = "Implantation") float FloatAfterHatchingHours = 4.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float AdhesionHours = 156.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float InvasionHours = 168.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float LacunarHours = 216.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float EmbeddedHours = 240.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float UteroplacentalHours = 264.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float PrimaryVilliHours = 312.0f;

	/**
	 * hCG: ab ~5 mIU/ml im Blut messbar (empfindlicher Labortest), in den ersten Wochen Verdopplung alle 1,3–2 Tage
	 * (Barnhart 2004: mindestens +53 % in zwei Tagen). Ein Urintest (25 mIU/ml) schlägt damit um Tag 12–13 an,
	 * rund zum Termin der ausgebliebenen Regel.
	 */
	UPROPERTY(EditAnywhere, Category = "Implantation") float HcgDetectableHours = 228.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float HcgDoublingHours = 31.0f;

	/**
	 * Frühes Ende nach dem Tag der Einnistung (Wilcox 1999, NEJM, 189 Schwangerschaften mit täglichem hCG):
	 * bis Tag 9 nach dem Eisprung 13 %, Tag 10 26 %, Tag 11 52 %, später 82 %. Ein spät ankommender Keim trifft auf eine
	 * Schleimhaut, deren Empfänglichkeitsfenster sich schließt.
	 * Wilcox zählt als Einnistung den ersten Anstieg von hCG im Urin (hochempfindlicher Test): beim mittleren Keim
	 * 36 h nach Beginn der Invasion, Tag 9 nach dem Eisprung. Die Befruchtung liegt ~12 h nach dem Eisprung.
	 */
	UPROPERTY(EditAnywhere, Category = "Implantation") float HcgFirstRiseAfterInvasionHours = 36.0f;
	UPROPERTY(EditAnywhere, Category = "Implantation") float HoursFusionAfterOvulation = 12.0f;

	/** Nicht mehr benutzt (vor GENESIS-040: lineare Einnistung in 48 h). */
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
