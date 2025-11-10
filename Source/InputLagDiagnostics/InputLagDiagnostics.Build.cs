using UnrealBuildTool;

public class InputLagDiagnostics : ModuleRules
{
    public InputLagDiagnostics(TargetInfo Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "UnrealTournament"
            }
        );
    }
}
