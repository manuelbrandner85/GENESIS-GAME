// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisPeople : ModuleRules
{
	public GenesisPeople(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AnimationCore",
			"GenesisCore"
		});
	}
}
