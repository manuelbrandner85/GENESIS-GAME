// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSlice : ModuleRules
{
	public GenesisSlice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"GenesisCore",
			"GenesisConception",
			"GenesisEmbryo",
			"GenesisBody",
			"GenesisBirth",
			"GenesisEarlyLife"
		});
	}
}
