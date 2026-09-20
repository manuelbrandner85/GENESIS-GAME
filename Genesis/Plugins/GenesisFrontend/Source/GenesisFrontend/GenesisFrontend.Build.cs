// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisFrontend : ModuleRules
{
	public GenesisFrontend(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"ApplicationCore",
			"RHI",
			"GenesisCore",
			"GenesisAudioCore"
		});
	}
}
