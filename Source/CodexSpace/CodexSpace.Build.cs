using UnrealBuildTool;

public class CodexSpace : ModuleRules
{
    public CodexSpace(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore" });
    }
}
