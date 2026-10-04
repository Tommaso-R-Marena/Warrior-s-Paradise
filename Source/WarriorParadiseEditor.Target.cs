using UnrealBuildTool;

public class WarriorParadiseEditorTarget : TargetRules
{
    public WarriorParadiseEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarriorParadise");
    }
}
