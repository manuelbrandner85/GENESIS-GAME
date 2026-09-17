// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisMemory : ModuleRules
{
	public GenesisMemory(ReadOnlyTargetRules Target) : base(Target)
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
