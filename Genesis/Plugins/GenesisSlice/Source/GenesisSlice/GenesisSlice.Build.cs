// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSlice : ModuleRules
{
	public GenesisSlice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"SlateCore",
			"GameplayTags",
			"GenesisCore",
			"GenesisConception",
			"GenesisEmbryo",
			"GenesisBody",
			"GenesisBirth",
			"GenesisEarlyLife",
			"GenesisFrontend",
			"GenesisPeople",
			"GenesisSound",
			"GenesisVoice",
			"GenesisWorldSound",
			"GenesisAudioCore",
			"GenesisGenetics",
			"AudioMixer",
			"CinematicCamera"
		});
	}
}
