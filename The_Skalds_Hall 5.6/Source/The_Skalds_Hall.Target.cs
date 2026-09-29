using UnrealBuildTool;
public class The_Skalds_HallTarget : TargetRules
{
    public The_Skalds_HallTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("The_Skalds_Hall");
    }
}