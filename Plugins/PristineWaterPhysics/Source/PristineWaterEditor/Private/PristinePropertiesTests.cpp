#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PristineWaterProperties.h"
#include "PristineWaterPerformance.h"
#include "PristineWaterRegion.h"
#include "PristineWaterComponent.h"
#include "PristineSmallFluid.h"
#include "ProceduralMeshComponent.h"
#include "Editor.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristinePropertiesTest,"PristineWater.Properties.FreshWater",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristinePropertiesTest::RunTest(const FString&)
{
    const auto Cold=UPristineWaterProperties::FreshWaterAtTemperature(0.01f);
    const auto Four=UPristineWaterProperties::FreshWaterAtTemperature(4.01f);
    const auto Room=UPristineWaterProperties::FreshWaterAtTemperature(20);
    const auto Hot=UPristineWaterProperties::FreshWaterAtTemperature(80);
    TestTrue(TEXT("Density maximum near four Celsius"),Four.DensityKgM3>Cold.DensityKgM3&&Four.DensityKgM3>Room.DensityKgM3);
    TestTrue(TEXT("Room-temperature density reference"),FMath::Abs(Room.DensityKgM3-998.2f)<0.1f);
    TestTrue(TEXT("Room-temperature viscosity reference"),FMath::Abs(Room.DynamicViscosityPaS-0.001002f)<0.000002f);
    TestTrue(TEXT("Room-temperature surface tension reference"),FMath::Abs(Room.SurfaceTensionNm-0.07274f)<0.0001f);
    TestTrue(TEXT("Hot liquid has lower viscosity and surface tension"),Hot.KinematicViscosityM2S<Room.KinematicViscosityM2S&&Hot.SurfaceTensionNm<Room.SurfaceTensionNm);
    TestEqual(TEXT("Temperature constrained to liquid reference range"),UPristineWaterProperties::FreshWaterAtTemperature(500).TemperatureC,80.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineBudgetTest,"PristineWater.Performance.Adaptation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineBudgetTest::RunTest(const FString&)
{
    FPristineWaterPerformance P; P.Preset=EPristinePerformancePreset::Custom; P.MaxSurfaceHz=60; P.BudgetMs=2; P.ResetMeasurements();
    for(int I=0;I<300;++I) P.Observe(1.f/60,1,12,true);
    TestTrue(TEXT("Expensive meshes reduce visual rate"),P.EffectiveSurfaceHz<10&&P.EffectiveSurfaceHz>=P.MinSurfaceHz);
    for(int I=0;I<120;++I) P.Observe(1.f/60,5,12,true);
    TestTrue(TEXT("Unmet solver budget exposed"),P.bSolverOverBudget);
    P.bAdaptiveVisuals=false; P.Observe(1.f/60,5,12,true);
    TestEqual(TEXT("Manual rate respected"),P.EffectiveSurfaceHz,60.f);
    UPristineWaterPerformanceLibrary::Apply(P);
    TestEqual(TEXT("Custom budget retained"),P.BudgetMs,2.f);
    TestTrue(TEXT("Hardware profile has CPU and memory"),UPristineWaterPerformanceLibrary::GetWaterHardware().PhysicalCores>0&&UPristineWaterPerformanceLibrary::GetWaterHardware().RAMGiB>0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineThermalTest,"PristineWater.Properties.ConservationAndPressure",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineThermalTest::RunTest(const FString&)
{
    UWorld* W=GEditor->GetEditorWorldContext().World(); if(!W) return false;
    auto* R=W->SpawnActor<APristineWaterRegion>(); R->Resolution=16; R->ResetWater();
    const double Mass=R->GetWaterVolumeM3()*R->Water->Volume.Density*1000;
    R->SetWaterTemperature(80);
    TestTrue(TEXT("Uniform expansion preserves liquid mass"),FMath::Abs(R->GetWaterVolumeM3()*R->Water->Volume.Density*1000-Mass)<Mass*1.e-6);
    const auto S=R->SampleRegion(FVector(0,0,-50));
    TestTrue(TEXT("Hydrostatic pressure follows rho g h"),FMath::Abs(S.HydrostaticPressurePa-S.DepthCm*0.01*R->Water->Volume.Density*1000*9.81)<0.1);
    const double Volume=R->GetWaterVolumeM3(); R->SetPerformancePreset(EPristinePerformancePreset::Low);
    TestEqual(TEXT("Quality does not refill region"),R->GetWaterVolumeM3(),Volume);
    const auto* Section=R->Surface->GetProcMeshSection(0);
    bool Up=Section&&!Section->ProcVertexBuffer.IsEmpty();
    if(Section) for(const auto& V:Section->ProcVertexBuffer) Up&=V.Normal.Z>0.99;
    TestTrue(TEXT("Flat surface normals point upward"),Up);
    auto* F=W->SpawnActor<APristineSmallFluid>(); F->ParticlesPerAxis=3; F->bSceneCollision=false; F->ResetFluid();
    const double SmallMass=F->GetLiquidMassKg();
    F->SetWaterTemperature(80); F->SetPerformancePreset(EPristinePerformancePreset::Low);
    TestEqual(TEXT("Heat and quality preserve particle mass"),F->GetLiquidMassKg(),SmallMass);
    TestEqual(TEXT("Quality preserves particle count"),F->GetParticleCount(),27);
    F->SetFluidProperties(990,1.e-6,0.07,10);
    TestEqual(TEXT("Custom density retains mass"),F->GetLiquidMassKg(),SmallMass);
    for(int I=0;I<30;++I) F->AdvanceSimulation(1.f/60);
    TestTrue(TEXT("Modified liquid remains finite and contained"),F->IsFluidStateValid());
    F->Destroy(); R->Destroy(); return true;
}
#endif
