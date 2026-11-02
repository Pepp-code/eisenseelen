using UnrealBuildTool;

public class EisenseelenTarget : TargetRules
{
	public EisenseelenTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Eisenseelen");
	}
}
