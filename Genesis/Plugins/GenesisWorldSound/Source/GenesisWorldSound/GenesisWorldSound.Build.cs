// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisWorldSound : ModuleRules
{
	public GenesisWorldSound(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AudioMixer",
			"GameplayTags",
			"GenesisCore",
			"GenesisBody",
			"GenesisBirth",
			"GenesisAudioCore",
			"GenesisSound"
		});
	}
}
