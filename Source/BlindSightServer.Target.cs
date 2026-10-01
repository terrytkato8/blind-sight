using UnrealBuildTool;
using System.Collections.Generic;

/**
 * Dedicated server target. Builds a headless binary with no rendering, no audio device
 * and no client-only assets — the authoritative simulation only.
 *
 * Build:  Build.bat BlindSightServer Linux Development -project="<path>/BlindSight.uproject"
 * Cook:   RunUAT BuildCookRun -server -serverplatform=Linux -noclient
 */
public class BlindSightServerTarget : TargetRules
{
	public BlindSightServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		CppStandard = CppStandardVersion.Cpp20;
		WindowsPlatform.bStrictConformanceMode = true;

		ExtraModuleNames.Add("BlindSight");

		// Headless: no audio mixer, no rendering. Saves ~400MB RAM per container.
		bUseLoggingInShipping = true;
		bCompileAgainstEngine = true;
	}
}
