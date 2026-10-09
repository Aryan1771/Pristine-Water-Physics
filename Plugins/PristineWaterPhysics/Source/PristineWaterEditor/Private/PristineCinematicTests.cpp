#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PristineWaterRegion.h"
#include "PristineWaterSource.h"
#include "PristineWaterComponent.h"
#include "ProceduralMeshComponent.h"
#include "Editor.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineCinematicTest,"PristineWater.Rendering.CinematicPreset",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineCinematicTest::RunTest(const FString&)
{
    auto* W=GEditor->GetEditorWorldContext().World(); if(!W) return false;
    auto* R=W->SpawnActor<APristineWaterRegion>(); const double Volume=R->GetWaterVolumeM3();
    R->ApplyCinematicPreset();
    TestEqual(TEXT("Cinematic preset does not refill water"),R->GetWaterVolumeM3(),Volume);
    TestTrue(TEXT("Physical cinematic material available"),R->Water->DynamicMaterial&&R->Water->DynamicMaterial->Parent->GetName()==TEXT("M_PristineCinematic"));
    TestTrue(TEXT("Calm water has low dielectric roughness"),R->Water->Optics.Roughness<0.01f&&R->Water->Optics.Reflectivity==1);
    R->ResetWater();
    TestEqual(TEXT("Shore foam remains off after reset"),R->Water->DynamicMaterial->K2_GetScalarParameterValue(TEXT("ShoreFoam")),0.f);
    auto* S=W->SpawnActor<APristineWaterSource>(); S->ApplyCinematicPreset();
    TestTrue(TEXT("Cinematic source uses real wave geometry"),S->bRayTracingWaveGeometry&&S->Quality==EPristineWaterQuality::High);
    TestEqual(TEXT("Cinematic visual rate preserved by custom preset"),S->Performance.MaxSurfaceHz,60.f);
    S->Destroy(); R->Destroy(); return true;
}
#endif
