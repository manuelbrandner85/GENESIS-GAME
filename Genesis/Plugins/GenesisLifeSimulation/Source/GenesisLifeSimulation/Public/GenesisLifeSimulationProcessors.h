// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "GenesisLifeSimulationProcessor.h"
#include "GenesisLifeSimulationProcessors.generated.h"

/*
 * Die 14 verbundenen Systeme der Life Simulation Engine – Reihenfolge der Pipeline:
 *
 *  Wahrnehmung:  CulturalIdentity(0) → Misunderstanding(10)
 *  Inneres:      Indoctrination(0) → Belief(5) → Propaganda(10) → Zeitgeist(20) → GroupPressure(30) → EgoismAltruism(40) → DoubleStandard(50)
 *  Sozial:       Trust(0) → ReputationRumors(10)
 *  Integration:  Karma(0) → Epigenetics(10) → ConsequenceNetwork(100)
 */

// ===================================== Wahrnehmung =====================================

/** 9 – Kulturelle Identität: Tabus, kulturelle Distanz der Beobachter, Bindung an die eigene Kultur. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_CulturalIdentity : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Perception; }
	virtual int32 GetOrder() const override { return 0; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 13 – Missverständnis: Wird die Handlung bemerkt? Wird sie richtig verstanden? */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Misunderstanding : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Perception; }
	virtual int32 GetOrder() const override { return 10; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

// ======================================= Inneres =======================================

/** 3 – Indoktrination: verinnerlichte Ideologien werden durch Handeln vertieft oder erodieren (Dissonanz). */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Indoctrination : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 0; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 10 – Glaube: Gewissheit, Offenheit, Haltung – neutral, ohne Bewertung. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Belief : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 5; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 11 – Propaganda & Manipulation: Einflüsse formen Entscheidungen und erzeugen unbemerkt Überzeugungen bei anderen. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Propaganda : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 10; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 8 – Zeitgeist: Mitschwimmen ist leicht, Widerstand kostet – und Handlungen verschieben die Epoche langsam. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Zeitgeist : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 20; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
	virtual void AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const override;
};

/** 7 – Gruppenzwang: anwesende Gruppen stützen oder verurteilen die Handlung. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_GroupPressure : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 30; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 4 – Egoismus/Altruismus: Spektrum verschiebt sich; Opfer wiegen schwerer. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_EgoismAltruism : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 40; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 12 – Doppelmoral-Detektor: verletzte eigene Werte und Anprangern trotz eigener Vergangenheit. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_DoubleStandard : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Inner; }
	virtual int32 GetOrder() const override { return 50; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

// ======================================== Sozial ========================================

/** 5 – Vertrauen: wahrgenommene Absicht, Negativitätsverzerrung, Verrat wiegt aus hohem Vertrauen schwerer. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Trust : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Social; }
	virtual int32 GetOrder() const override { return 0; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 6 – Ruf & Gerüchte: Ruf je Eigenschaft; Gerüchte verbreiten sich über das Vertrauensnetz und verzerren sich. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_ReputationRumors : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Social; }
	virtual int32 GetOrder() const override { return 10; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
	virtual void AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const override;
};

// ====================================== Integration ======================================

/** 1 – Karma: integriert den modulierten Impuls mit Sättigung und Gewohnheit. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Karma : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Integration; }
	virtual int32 GetOrder() const override { return 0; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
};

/** 14 – Genetischer Code & Epigenetik: Belastung abhängig von Veranlagung, jährliche Übertragung ins Genom. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_Epigenetics : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Integration; }
	virtual int32 GetOrder() const override { return 10; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
	virtual void AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const override;
};

/** 2 – Konsequenz-Netzwerk: Ereignis in den Kausalgraph, Verknüpfungen, Erinnerungen, verzögerte Folgen. */
UCLASS()
class GENESISLIFESIMULATION_API UGenesisProcessor_ConsequenceNetwork : public UGenesisLifeSimulationProcessor
{
	GENERATED_BODY()
public:
	virtual FGameplayTag GetSystemTag() const override;
	virtual EGenesisSimulationStage GetStage() const override { return EGenesisSimulationStage::Integration; }
	virtual int32 GetOrder() const override { return 100; }
	virtual void ProcessAction(const FGenesisLifeAction& Action, const UGenesisLifeActionDefinition& Definition, FGenesisLifeSimulationFrame& Frame, FGenesisLifeSimulationContext& Context) const override;
	virtual void AdvanceTime(const FGenesisSimulationStep& Step, FGenesisLifeSimulationContext& Context) const override;
};
