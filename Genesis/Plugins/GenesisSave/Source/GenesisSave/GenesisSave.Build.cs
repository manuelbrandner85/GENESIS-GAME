// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSave : ModuleRules
{
	public GenesisSave(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GenesisCore"
		});
	}
}
