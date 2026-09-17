// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSoulMusic : ModuleRules
{
	public GenesisSoulMusic(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"DeveloperSettings",
			"GenesisCore",
			"GenesisSoul",
			"GenesisMemory"
		});
	}
}
