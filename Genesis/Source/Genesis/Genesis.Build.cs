// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

/**
 * Primäres Spielmodul. Bewusst schlank: verbindet die Genesis-Plugins zum Spielablauf
 * (GameMode, Lebenszyklus-Orchestrierung). Systemlogik gehört in die Plugins.
 */
public class Genesis : ModuleRules
{
	public Genesis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"GenesisCore",
			"GenesisSave"
		});
	}
}
