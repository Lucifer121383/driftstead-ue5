using UnrealBuildTool;

public class DriftsteadEditorTarget : TargetRules
{
    public DriftsteadEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
        ExtraModuleNames.Add("Driftstead");
    }
}
