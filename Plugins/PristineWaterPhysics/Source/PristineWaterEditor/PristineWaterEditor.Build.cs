using UnrealBuildTool;

public class PristineWaterEditor : ModuleRules
{
    public PristineWaterEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry", "MaterialEditor", "RenderCore", "RHI", "PristineWaterPhysics", "ProceduralMeshComponent", "ToolMenus", "Slate", "SlateCore" });
    }
}
