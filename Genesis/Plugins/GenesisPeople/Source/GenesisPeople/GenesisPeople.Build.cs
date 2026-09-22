// GENESIS: Der Kreislauf des Lebens

using UnrealBuildTool;

public class GenesisPeople : ModuleRules
{
	public GenesisPeople(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AnimationCore",
			"GenesisCore"
		});

		// LippensynchronitÃ¤t: Die Spuren entstehen im Editor aus den Aufnahmen (StreamingADA + MetaHuman-Umrechnung
		// der Regler auf RigLogic-Rohsteuerungen). Das Spiel spielt nur die gespeicherten Spuren ab.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"SpeechAnimationSolver",
				"MetaHumanCoreTech",
				"NNE",
				"AssetRegistry",
				"AudioPlatformConfiguration"
			});
		}
	}
}
