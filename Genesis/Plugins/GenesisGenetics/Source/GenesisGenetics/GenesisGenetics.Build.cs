// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisGenetics : ModuleRules
{
	public GenesisGenetics(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"DeveloperSettings",
			"GenesisCore"
		});
	}
}
