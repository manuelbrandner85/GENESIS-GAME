// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisConception : ModuleRules
{
	public GenesisConception(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CinematicCamera",
			"GenesisCore"
		});
	}
}
