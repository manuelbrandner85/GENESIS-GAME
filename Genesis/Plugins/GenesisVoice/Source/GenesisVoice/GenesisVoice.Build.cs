// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisVoice : ModuleRules
{
	public GenesisVoice(ReadOnlyTargetRules Target) : base(Target)
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
			"GenesisGenetics",
			"GenesisBody",
			"GenesisAudioCore",
			"GenesisSound",
			"GenesisEarlyLife"
		});
	}
}
