using UnrealBuildTool;

public class DriftsteadTarget : TargetRules
{
    public DriftsteadTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("Driftstead");
    }
}
