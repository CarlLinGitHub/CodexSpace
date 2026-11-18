using UnrealBuildTool;

public class CodexSpaceTarget : TargetRules
{
    public CodexSpaceTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CodexSpace");
    }
}
