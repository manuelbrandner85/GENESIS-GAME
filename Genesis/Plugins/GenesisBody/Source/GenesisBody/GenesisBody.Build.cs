// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisBody : ModuleRules
{
	public GenesisBody(ReadOnlyTargetRules Target) : base(Target)
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
			"GenesisMemory",
			"GenesisGenetics",
			"GenesisLifeSimulation"
		});
	}
}
