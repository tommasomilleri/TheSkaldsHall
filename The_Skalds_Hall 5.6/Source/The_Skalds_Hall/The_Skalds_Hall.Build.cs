using UnrealBuildTool;

public class The_Skalds_Hall : ModuleRules
{
	public The_Skalds_Hall(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore", "HeadMountedDisplay", "XRBase", "Niagara","NavigationSystem" });
		
		
	}
}