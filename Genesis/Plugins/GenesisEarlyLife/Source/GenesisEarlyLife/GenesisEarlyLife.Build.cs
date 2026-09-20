// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisEarlyLife : ModuleRules
{
	public GenesisEarlyLife(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"CinematicCamera",
			"GameplayTags",
			"GenesisCore",
			"GenesisBody",
			"GenesisBirth",
			"GenesisMemory",
			"GenesisSoul",
			"GenesisSoulMusic",
			"GenesisAudioCore"
		});
	}
}
