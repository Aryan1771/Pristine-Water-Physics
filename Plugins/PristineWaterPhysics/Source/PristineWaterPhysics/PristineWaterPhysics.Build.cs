using UnrealBuildTool;

public class PristineWaterPhysics : ModuleRules
{
    public PristineWaterPhysics(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "ProceduralMeshComponent"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "RHI" });
    }
}
