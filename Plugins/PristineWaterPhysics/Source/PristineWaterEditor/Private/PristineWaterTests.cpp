#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Engine/World.h"
#include "PristineWaterSource.h"
#include "PristineWaterComponent.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineWaterTest,"PristineWater.SourceAndMaterial",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineWaterTest::RunTest(const FString& Parameters)
{
    UWorld* World=GEditor->GetEditorWorldContext().World();
    if (!TestNotNull(TEXT("Editor world"),World)) return false;
    APristineWaterSource* Source=World->SpawnActor<APristineWaterSource>();
    Source->SetBodyType(EPristineWaterBody::Puddle);
    Source->SetQuality(EPristineWaterQuality::Low);
    TestEqual(TEXT("Low grid vertex budget"),Source->Surface->GetProcMeshSection(0)->ProcVertexBuffer.Num(),1089);
    UMaterialInstanceDynamic* MID=Source->Water->DynamicMaterial;
    TestNotNull(TEXT("Bundled material loaded"),MID);
    Source->ApplyAppearance();
    TestEqual(TEXT("Repeated apply reuses MID"),Source->Water->DynamicMaterial,MID);
    Source->Water->SetReflectivity(5);
    TestEqual(TEXT("Setter clamps reflectivity"),Source->Water->Optics.Reflectivity,1.0f);
    TestTrue(TEXT("Outside ellipse rejected"),!Source->SampleWater(FVector(1000,1000,0)).bInside);
    Source->Water->SetWaveAmplitude(0);
    const auto Sample=Source->SampleWater(FVector(0,0,-1));
    TestTrue(TEXT("Point below puddle inside"),Sample.bInside);
    TestEqual(TEXT("Flat surface height"),Sample.SurfaceZ,0.0f);
    TestEqual(TEXT("Density unit conversion"),Sample.DensityKgM3,1000.0f);
    TestTrue(TEXT("Buoyancy pushes upward"),Source->GetSphereBuoyancyForce(FVector(0,0,-1),1).Z>0);
    TestTrue(TEXT("Negative radius rejected"),Source->GetSphereBuoyancyForce(FVector::ZeroVector,-1).IsZero());
    Source->SetQuality(EPristineWaterQuality::High);
    TestEqual(TEXT("High grid vertex budget"),Source->Surface->GetProcMeshSection(0)->ProcVertexBuffer.Num(),16641);
    Source->AddRipple(FVector::ZeroVector,1);
    TestTrue(TEXT("Geometry ripple affects sampled height"),FMath::Abs(Source->SampleWater(FVector(30,0,0)).SurfaceZ)>0.01);
    Source->bRayTracingWaveGeometry=false; Source->ApplyAppearance();
    TestEqual(TEXT("Disabling geometry waves restores rest mesh"),Source->Surface->GetProcMeshSection(0)->ProcVertexBuffer[0].Position.Z,0.0);
    Source->bRayTracingWaveGeometry=true; Source->ApplyAppearance();
    Source->Water->SetLiquidType(EPristineLiquidType::Oil);
    TestEqual(TEXT("Oil density preset"),Source->Water->Volume.Density,0.9f);
    Source->SetBodyType(EPristineWaterBody::Ocean);
    TestEqual(TEXT("Ocean geometry remains bounded"),Source->Surface->GetProcMeshSection(0)->ProcVertexBuffer.Num(),16641);
    TestTrue(TEXT("Wave bounds have vertical padding"),Source->Surface->Bounds.BoxExtent.Z>=1000);
    Source->Destroy();
    return true;
}
#endif
