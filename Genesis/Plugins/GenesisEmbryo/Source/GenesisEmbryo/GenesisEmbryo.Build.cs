// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisEmbryo : ModuleRules
{
	public GenesisEmbryo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GenesisCore",
			"GenesisBody",
			"GenesisGenetics",
			"GenesisConception",
			"GenesisSoulMusic"
		});
	}
}
