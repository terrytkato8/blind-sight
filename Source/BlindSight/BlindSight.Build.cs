using UnrealBuildTool;

public class BlindSight : ModuleRules
{
	public BlindSight(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { "BlindSight/Public" });
		PrivateIncludePaths.AddRange(new string[] { "BlindSight/Private" });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"EnhancedInput",
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			"NetCore", "DeveloperSettings",
			"UMG", "Slate", "SlateCore", "CommonUI",
			"AudioMixer", "MetasoundEngine",
			"OnlineSubsystem", "OnlineSubsystemUtils",
			"HTTP", "Json", "JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsCore" });
	}
}
