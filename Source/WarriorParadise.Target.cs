using UnrealBuildTool;

public class WarriorParadiseTarget : TargetRules
{
    public WarriorParadiseTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarriorParadise");
    }
}
