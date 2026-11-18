using UnrealBuildTool;

public class CodexSpaceEditorTarget : TargetRules
{
    public CodexSpaceEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CodexSpace");
    }
}
