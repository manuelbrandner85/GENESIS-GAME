// GENESIS: Der Kreislauf des Lebens

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GenesisSoulTypes.h"
#include "GenesisSoulSettings.generated.h"

/** Projekteinstellungen der Soul Engine (Project Settings → Genesis → Soul). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Genesis Soul"))
class GENESISSOUL_API UGenesisSoulSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGenesisSoulSettings();

	virtual FName GetCategoryName() const override { return TEXT("Genesis"); }

	/** Übertragsregeln zwischen Leben. */
	UPROPERTY(Config, EditAnywhere, Category = "Carry Over")
	FGenesisSoulCarryOverParams CarryOver;

	/** Mögliche angeborene Muster einer neuen Seele (Genesis.Soul.Pattern.*). */
	UPROPERTY(Config, EditAnywhere, Category = "Creation", meta = (Categories = "Genesis.Soul.Pattern"))
	TArray<FGameplayTag> InnatePatternPool;
};
