using UnrealBuildTool;
public class The_Skalds_HallEditorTarget : TargetRules
{
    public The_Skalds_HallEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("The_Skalds_Hall");
    }
}