#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PristineShallowGrid.h"
#include "PristineWaterRegion.h"
#include "PristineSmallFluid.h"
#include "Editor.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineGridTest,"PristineWater.Fluid.ConservationAndDryBeds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineGridTest::RunTest(const FString&)
{
    FPristineShallowGrid G; G.Initialize(24,12,12);
    // Non-flat bed below a level free surface: pressure must remain balanced.
    for(int32 I=0;I<G.Bed.Num();++I) G.Bed[I]=0.1*FMath::Sin(double(I));
    G.Fill(1); const double Initial=G.Volume();
    for(int32 I=0;I<120;++I) G.Step(G.StableStep(),0.35);
    double Error=0; for(int32 I=0;I<G.Depth.Num();++I) Error=FMath::Max(Error,FMath::Abs(G.Bed[I]+G.Depth[I]-1));
    TestTrue(TEXT("Still lake remains level above uneven terrain"),Error<1.e-10);
    TestTrue(TEXT("Still lake conserves volume"),FMath::Abs(G.Volume()-Initial)<1.e-8);
    G.Fill(0); for(int32 Y=0;Y<G.N;++Y) for(int32 X=0;X<G.N;++X) {G.Bed[Y*G.N+X]=0; G.Depth[Y*G.N+X]=X<8?1:0;}
    const double DamVolume=G.Volume(); G.Impulse(4,12,FVector2D(4,0));
    for(int32 I=0;I<600;++I) G.Step(G.StableStep(),0.35);
    bool Valid=true; for(double H:G.Depth) Valid&=FMath::IsFinite(H)&&H>=0;
    TestTrue(TEXT("Dam break stays finite and nonnegative"),Valid);
    TestTrue(TEXT("Dam break conserves volume"),FMath::Abs(G.Volume()-DamVolume)<1.e-7);
    TestTrue(TEXT("Water wets initially dry cells"),G.Depth[12*G.N+12]>0);
    // Internal wall must block all flux, even with a large impulse.
    G.Fill(0); for(int32 Y=0;Y<G.N;++Y) for(int32 X=0;X<G.N;++X) {G.Active[Y*G.N+X]=X!=12; G.Depth[Y*G.N+X]=X<12?1:0;}
    for(int32 I=0;I<120;++I) {G.Impulse(11,12,FVector2D(10,0)); G.Step(G.StableStep(),0.1);}
    double Outside=0; for(int32 Y=0;Y<G.N;++Y) for(int32 X=13;X<G.N;++X) Outside+=G.Depth[Y*G.N+X];
    TestEqual(TEXT("No water crosses closed wall"),Outside,0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineRegionTest,"PristineWater.Fluid.RegionAndSplashes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineRegionTest::RunTest(const FString&)
{
    UWorld* W=GEditor->GetEditorWorldContext().World(); if(!W) return false;
    auto* R=W->SpawnActor<APristineWaterRegion>(); R->Resolution=16; R->ResetWater();
    const double Initial=R->GetWaterVolumeM3();
    TestTrue(TEXT("Region starts filled"),Initial>0);
    TestTrue(TEXT("Boundary rejects exterior queries"),!R->SampleRegion(FVector(5000,0,0)).bWet);
    R->AddSplash(FVector::ZeroVector,FVector(150,0,-400),400,2);
    TestTrue(TEXT("Splash transfers rather than creates volume"),FMath::Abs(R->GetWaterVolumeM3()-Initial)<1.e-7);
    for(int32 I=0;I<180;++I) R->AdvanceSimulation(1.f/30);
    TestTrue(TEXT("Splash landing conserves total volume"),FMath::Abs(R->GetWaterVolumeM3()-Initial)<1.e-6);
    R->SetWaterLevel(50); TestEqual(TEXT("Level tool sets height"),R->SampleRegion(FVector::ZeroVector).SurfaceZ,50.f);
    TestNotNull(TEXT("Ray tracing compatible procedural surface exists"),R->Surface->GetProcMeshSection(0));
    R->SetWaterLevel(-300); TestEqual(TEXT("Level below bed dries region"),R->GetWaterVolumeM3(),0.0);
    R->Destroy(); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineSmallTest,"PristineWater.Fluid.SmallVolume",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineSmallTest::RunTest(const FString&)
{
    UWorld* W=GEditor->GetEditorWorldContext().World(); if(!W) return false;
    auto* F=W->SpawnActor<APristineSmallFluid>(); F->ParticlesPerAxis=4; F->SurfaceResolution=16; F->bSceneCollision=false; F->ResetFluid();
    const double Mass=F->GetLiquidMassKg(); F->AddImpulse(FVector(100,30,100));
    for(int32 I=0;I<90;++I) F->AdvanceSimulation(1.f/60);
    TestEqual(TEXT("Particle count conserved"),F->GetParticleCount(),64);
    TestEqual(TEXT("Mass conserved"),F->GetLiquidMassKg(),Mass);
    TestTrue(TEXT("Particles finite and inside container after impact"),F->IsFluidStateValid());
    TestNotNull(TEXT("Continuous liquid skin generated"),F->Surface->GetProcMeshSection(0));
    F->Destroy(); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineBakeTest,"PristineWater.Fluid.TerrainBake",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineBakeTest::RunTest(const FString&)
{
    UWorld* W=GEditor->GetEditorWorldContext().World(); if(!W) return false;
    const FVector Origin(100000,100000,1000);
    auto* Bed=W->SpawnActor<AStaticMeshActor>(Origin+FVector(0,0,-150),FRotator::ZeroRotator);
    auto* Mesh=Bed->GetStaticMeshComponent(); Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Mesh->SetCollisionObjectType(ECC_WorldStatic); Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Bed->SetActorScale3D(FVector(40,30,1));
    Mesh->RecreatePhysicsState();
    auto* R=W->SpawnActor<APristineWaterRegion>(Origin,FRotator::ZeroRotator); R->Resolution=8; R->TerrainActors.Add(Bed); R->BakeLandscape();
    TestTrue(TEXT("Bake marked valid"),R->bLandscapeBakeValid);
    AddInfo(FString::Printf(TEXT("Terrain measured volume %.9f m3 from %d hits"),R->GetWaterVolumeM3(),R->LandscapeSampleCount));
    int32 Active=0; double MinBed=DBL_MAX,MaxBed=-DBL_MAX;
    for(int32 I=0;I<R->Grid.Active.Num();++I) {Active+=R->Grid.Active[I]; MinBed=FMath::Min(MinBed,R->Grid.Bed[I]); MaxBed=FMath::Max(MaxBed,R->Grid.Bed[I]);}
    AddInfo(FString::Printf(TEXT("Terrain active %d, bed %.9f .. %.9f"),Active,MinBed,MaxBed));
    TestTrue(TEXT("Traced one-meter bed gives 600 cubic meters"),FMath::Abs(R->GetWaterVolumeM3()-600)<0.1);
    TestTrue(TEXT("Below bed query rejected"),!R->SampleRegion(Origin-FVector(0,0,110)).bWet);
    R->Resolution=12; R->ResetWater();
    TestTrue(TEXT("Changed grid invalidates terrain bake"),!R->bLandscapeBakeValid);
    TestEqual(TEXT("Stale bake cannot silently refill flat ground"),R->GetWaterVolumeM3(),0.0);
    R->BakeLandscape(); TestTrue(TEXT("Rebake restores water"),R->GetWaterVolumeM3()>0);
    R->Destroy(); Bed->Destroy(); return true;
}
#endif
