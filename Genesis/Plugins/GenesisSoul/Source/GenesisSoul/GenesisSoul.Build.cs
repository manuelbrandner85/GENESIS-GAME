// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSoul : ModuleRules
{
	public GenesisSoul(ReadOnlyTargetRules Target) : base(Target)
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
