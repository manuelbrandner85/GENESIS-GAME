// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisBirth : ModuleRules
{
	public GenesisBirth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CinematicCamera",
			"GenesisCore",
			"GenesisBody",
			"GenesisAudioCore",
			"GenesisSoulMusic"
		});
	}
}
