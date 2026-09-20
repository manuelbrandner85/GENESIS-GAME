// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisSound : ModuleRules
{
	public GenesisSound(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AudioMixer",
			"GenesisCore",
			"GenesisAudioCore",
			"GenesisBody",
			"GenesisBirth"
		});
	}
}
