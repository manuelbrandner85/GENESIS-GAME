// GENESIS: Der Kreislauf des Lebens

#include "GenesisSoulSettings.h"
#include "GenesisSoulGameplayTags.h"

UGenesisSoulSettings::UGenesisSoulSettings()
{
	// Grundausstattung: Kategorien als Fallback, bis Content konkrete Muster definiert
	InnatePatternPool = {
		GenesisSoulTags::Pattern_Affinity,
		GenesisSoulTags::Pattern_Fear,
		GenesisSoulTags::Pattern_Melody,
		GenesisSoulTags::Pattern_RecurringDream,
		GenesisSoulTags::Pattern_Talent,
		GenesisSoulTags::Pattern_Emotional
	};
}
