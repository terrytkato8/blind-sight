using UnrealBuildTool;
using System.Collections.Generic;

public class BlindSightTarget : TargetRules
{
	public BlindSightTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// UE 5.6: V5 is still the current build-settings version (V6 arrives with 5.7).
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		// 5.6 dropped C++17 support; stated explicitly so an engine upgrade can't silently change it.
		CppStandard = CppStandardVersion.Cpp20;
		WindowsPlatform.bStrictConformanceMode = true;
		ExtraModuleNames.Add("BlindSight");
	}
}
