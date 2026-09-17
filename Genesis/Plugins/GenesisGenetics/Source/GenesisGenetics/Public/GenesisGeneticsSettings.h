// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GenesisGeneticsTypes.h"
#include "GenesisGeneticsSettings.generated.h"

/** Projekteinstellungen der Genetik (Project Settings → Genesis → Genetics). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Genetics"))
class GENESISGENETICS_API UGenesisGeneticsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	/** Gen-Katalog der Welt. Leer = eingebaute Standardmerkmale. */
	UPROPERTY(Config, EditAnywhere, Category = "Catalog")
	TSoftObjectPtr<UGenesisGeneticsCatalog> Catalog;

	UPROPERTY(Config, EditAnywhere, Category = "Conception")
	FGenesisConceptionParams Conception;

	/** Wie stark eine Exposition epigenetische Markierungen verschiebt (0..1). */
	UPROPERTY(Config, EditAnywhere, Category = "Epigenetics", meta = (ClampMin = "0", ClampMax = "1"))
	float EpigeneticPlasticity = 0.1f;

	/** Rückbildung ohne Exposition pro Jahr (0..1). */
	UPROPERTY(Config, EditAnywhere, Category = "Epigenetics", meta = (ClampMin = "0", ClampMax = "1"))
	float EpigeneticReversionPerYear = 0.05f;
};
