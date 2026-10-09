#include "PristineWaterRegion.h"
#include "PristineWaterComponent.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "PristineWaterProperties.h"
#include "HAL/PlatformTime.h"

APristineWaterRegion::APristineWaterRegion()
{
    PrimaryActorTick.bCanEverTick=true;
    Boundary=CreateDefaultSubobject<USplineComponent>(TEXT("ClosedRegion")); SetRootComponent(Boundary);
    Boundary->ClearSplinePoints(false);
    for(FVector P:{FVector(-1500,-1000,0),FVector(1500,-1000,0),FVector(1500,1000,0),FVector(-1500,1000,0)}) Boundary->AddSplinePoint(P,ESplineCoordinateSpace::Local,false);
    for(int32 I=0;I<4;++I) Boundary->SetSplinePointType(I,ESplinePointType::Linear,false);
    Boundary->SetClosedLoop(true);
    Surface=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FluidSurface")); Surface->SetupAttachment(Boundary);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision); Surface->SetCastShadow(false);
    Splashes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SplashDroplets")); Splashes->SetupAttachment(Boundary);
    Splashes->SetCollisionEnabled(ECollisionEnabled::NoCollision); Splashes->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Splashes->SetStaticMesh(Sphere.Object);
    Water=CreateDefaultSubobject<UPristineWaterComponent>(TEXT("WaterSettings")); Water->bAssignLiquidMaterialOnApply=false;
    Water->Motion.WaveAmplitude=0; Water->Optics.Reflectivity=1; Water->Optics.TotalInternalReflection=0;
    SurfaceMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineFluid.M_PristineFluid")));
    SplashMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineDroplet.M_PristineDroplet")));
}
bool APristineWaterRegion::InPolygon(FVector2D P) const
{
    bool Inside=false; const int32 Count=Boundary->GetNumberOfSplinePoints();
    for(int32 I=0,J=Count-1;I<Count;J=I++)
    {
        const FVector A=Boundary->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::Local), B=Boundary->GetLocationAtSplinePoint(J,ESplineCoordinateSpace::Local);
        if((A.Y>P.Y)!=(B.Y>P.Y) && P.X<(B.X-A.X)*(P.Y-A.Y)/(B.Y-A.Y)+A.X) Inside=!Inside;
    }
    return Inside;
}
void APristineWaterRegion::ResetWater()
{
    if(IsTemplate()) return;
    Resolution=FMath::Clamp(Resolution,8,128); Min=FVector2D(DBL_MAX,DBL_MAX); Max=-Min;
    if(Boundary->GetNumberOfSplinePoints()<3) {Grid=FPristineShallowGrid(); Surface->ClearAllMeshSections(); return;}
    for(int32 I=0;I<Boundary->GetNumberOfSplinePoints();++I)
    {
        const FVector P=Boundary->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::Local);
        Min.X=FMath::Min(Min.X,P.X); Min.Y=FMath::Min(Min.Y,P.Y); Max.X=FMath::Max(Max.X,P.X); Max.Y=FMath::Max(Max.Y,P.Y);
    }
    if(Max.X-Min.X<1 || Max.Y-Min.Y<1) {Grid=FPristineShallowGrid(); Surface->ClearAllMeshSections(); return;}
    Grid.Initialize(Resolution,(Max.X-Min.X)*0.01,(Max.Y-Min.Y)*0.01);
    const bool Baked=BakedResolution==Resolution && BakedMin.Equals(Min,0.01) && BakedMax.Equals(Max,0.01) && BakedBed.Num()==Resolution*Resolution && BakedValid.Num()==BakedBed.Num();
    bLandscapeBakeValid=Baked;
    for(int32 Y=0;Y<Grid.N;++Y) for(int32 X=0;X<Grid.N;++X)
    {
        const int32 I=Y*Grid.N+X;
        // Require all corners inside so the rendered grid never crosses the polygon.
        bool Inside=true;
        for(int32 V=0;V<2;++V) for(int32 U=0;U<2;++U)
            Inside &= InPolygon(Min+FVector2D((X+U)*Grid.DX*100,(Y+V)*Grid.DY*100)+FVector2D(U?-0.001:0.001,V?-0.001:0.001));
        // Also reject a concave boundary cutting through a cell with four inside corners.
        const FBox Interior(FVector(Min.X+X*Grid.DX*100+0.002,Min.Y+Y*Grid.DY*100+0.002,-1),FVector(Min.X+(X+1)*Grid.DX*100-0.002,Min.Y+(Y+1)*Grid.DY*100-0.002,1));
        for(int32 J=0;Inside&&J<Boundary->GetNumberOfSplinePoints();++J)
        {
            FVector A=Boundary->GetLocationAtSplinePoint(J,ESplineCoordinateSpace::Local),B=Boundary->GetLocationAtSplinePoint((J+1)%Boundary->GetNumberOfSplinePoints(),ESplineCoordinateSpace::Local); A.Z=0; B.Z=0;
            if(FMath::LineBoxIntersection(Interior,A,B,B-A)) Inside=false;
        }
        Grid.Active[I]=Inside && (BakedBed.IsEmpty() || (Baked && BakedValid[I]));
        Grid.Bed[I]=Baked?BakedBed[I]:-FMath::Max(1.f,DefaultDepth)*0.01;
    }
    Grid.Fill(WaterLevel*0.01); Drops.Empty(); Splashes->ClearInstances();
    if(UMaterialInterface* M=SurfaceMaterial.LoadSynchronous()) Surface->SetMaterial(0,M);
    Water->Motion.WaveAmplitude=0; Water->BindToMesh(Surface);
    if(Water->DynamicMaterial) Water->DynamicMaterial->SetScalarParameterValue(TEXT("WaveAmplitude"),0);
    if(UMaterialInterface* M=SplashMaterial.LoadSynchronous()) Splashes->SetMaterial(0,M);
    UpdateMesh();
}
void APristineWaterRegion::BakeLandscape()
{
    if(!GetWorld()||IsTemplate()) return;
    Modify(); ResetWater(); if(Grid.N==0) return;
    BakedBed.SetNum(Grid.N*Grid.N); BakedValid.Init(0,Grid.N*Grid.N);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PristineTerrain),true,this);
    if(!TerrainActors.IsEmpty())
        for(TActorIterator<AActor> It(GetWorld());It;++It)
            if(!TerrainActors.Contains(*It)) Params.AddIgnoredActor(*It);
    LandscapeSampleCount=0;
    for(int32 Y=0;Y<Grid.N;++Y) for(int32 X=0;X<Grid.N;++X)
    {
        const int32 I=Y*Grid.N+X;
        const FVector P=GetActorTransform().TransformPosition(FVector(Min.X+(X+0.5)*Grid.DX*100,Min.Y+(Y+0.5)*Grid.DY*100,WaterLevel));
        FHitResult Hit;
        const FVector Start=P+FVector(0,0,FMath::Max(1.f,TraceAbove)),End=P-FVector(0,0,FMath::Max(1.f,TraceBelow));
        bool Found=GetWorld()->LineTraceSingleByChannel(Hit,Start,End,TerrainChannel,Params);
        // Some meshes expose only simple collision, or have a complex-triangle seam miss.
        if(!Found)
        {
            FCollisionQueryParams SimpleParams=Params; SimpleParams.bTraceComplex=false;
            Found=GetWorld()->LineTraceSingleByChannel(Hit,Start,End,TerrainChannel,SimpleParams);
        }
        BakedBed[I]=Found?GetActorTransform().InverseTransformPosition(Hit.ImpactPoint).Z*0.01:0;
        BakedValid[I]=Found;
        LandscapeSampleCount+=Found?1:0;
    }
    BakedResolution=Resolution; BakedMin=Min; BakedMax=Max; ResetWater();
    if(LandscapeSampleCount==0) UE_LOG(LogTemp,Warning,TEXT("Pristine Water: no terrain collision hits. Check the terrain channel and trace range, then bake again in the editor."));
}
void APristineWaterRegion::ClearLandscapeBake(){Modify(); BakedBed.Empty(); BakedValid.Empty(); BakedResolution=0; LandscapeSampleCount=0; ResetWater();}
void APristineWaterRegion::SetWaterLevel(float Z){WaterLevel=FMath::IsFinite(Z)?Z:0; ResetWater();}
void APristineWaterRegion::OnConstruction(const FTransform& T){Super::OnConstruction(T); ResetWater();}
void APristineWaterRegion::BeginPlay(){Super::BeginPlay(); ApplyPerformanceSettings(); ResetWater();}
void APristineWaterRegion::SetPerformancePreset(EPristinePerformancePreset P){Performance.Preset=P; ApplyPerformanceSettings();}
void APristineWaterRegion::ApplyPerformanceSettings(){UPristineWaterPerformanceLibrary::Apply(Performance);}
void APristineWaterRegion::SetWaterTemperature(float T){TemperatureC=T; ApplyWaterTemperature();}
void APristineWaterRegion::ApplyWaterTemperature()
{
    const auto P=UPristineWaterProperties::FreshWaterAtTemperature(TemperatureC);
    const double Ratio=FMath::Max(1.0,double(Water->Volume.Density)*1000)/P.DensityKgM3;
    for(double& Depth:Grid.Depth) Depth*=Ratio;
    for(double& Q:Grid.MX) Q*=Ratio;
    for(double& Q:Grid.MY) Q*=Ratio;
    for(FDrop& Drop:Drops) Drop.Volume*=Ratio;
    Water->Volume.Density=P.DensityKgM3/1000; Water->ApplyWaterSettings(); TemperatureC=P.TemperatureC;
    // Uniform thermal expansion preserves rho*volume, with no heat-transfer simulation.
    if(Grid.N>0) UpdateMesh();
}
void APristineWaterRegion::Tick(float Dt){Super::Tick(Dt); if(bSimulate) AdvanceSimulation(Dt);}
int32 APristineWaterRegion::Cell(FVector P) const
{
    if(Grid.N==0) return INDEX_NONE;
    const int32 X=FMath::FloorToInt((P.X-Min.X)/(Grid.DX*100)),Y=FMath::FloorToInt((P.Y-Min.Y)/(Grid.DY*100));
    return X>=0&&Y>=0&&X<Grid.N&&Y<Grid.N&&Grid.Active[Y*Grid.N+X]?Y*Grid.N+X:INDEX_NONE;
}
FPristineRegionSample APristineWaterRegion::SampleRegion(FVector P) const
{
    FPristineRegionSample S; const FVector L=GetActorTransform().InverseTransformPosition(P); const int32 I=Cell(L);
    if(I==INDEX_NONE||Grid.Depth[I]<1.e-5) return S;
    S.bWet=L.Z>=Grid.Bed[I]*100; S.SurfaceZ=GetActorLocation().Z+(Grid.Bed[I]+Grid.Depth[I])*100; S.DepthCm=S.SurfaceZ-P.Z;
    S.HydrostaticPressurePa=S.bWet?FMath::Max(0.f,S.DepthCm)*0.01f*FMath::Max(0.f,Water->Volume.Density)*1000*9.81f:0;
    const FVector2D Flow=Grid.Velocity(I);
    S.Velocity=GetActorTransform().TransformVectorNoScale(FVector(Flow.X*100,Flow.Y*100,0));
    return S;
}
double APristineWaterRegion::GetWaterVolumeM3() const {double V=Grid.Volume(); for(const FDrop& D:Drops) V+=D.Volume; return V;}
void APristineWaterRegion::AdvanceSimulation(float Dt)
{
    LastSimulatedSeconds=0; if(!FMath::IsFinite(Dt)||Dt<=0||Grid.N==0) return;
    const double Start=FPlatformTime::Seconds();
    double Remaining=FMath::Min(double(Dt),0.1);
    for(int32 S=0;S<FMath::Clamp(MaxSubsteps,1,128)&&Remaining>1.e-7;++S)
    {
        const double Step=FMath::Min(Remaining,Grid.StableStep());
        Grid.Step(Step,BottomDrag); UpdateDrops(Step); Remaining-=Step; LastSimulatedSeconds+=Step;
    }
    const double AfterSolver=FPlatformTime::Seconds(); MeshElapsed+=LastSimulatedSeconds;
    const bool Update=GetNetMode()!=NM_DedicatedServer&&MeshElapsed>=1/FMath::Clamp(Performance.EffectiveSurfaceHz,1.f,60.f);
    if(Update) {UpdateMesh(); RenderDrops(); MeshElapsed=0;}
    Performance.UnsimulatedSeconds=FMath::Max(0.f,Dt-LastSimulatedSeconds);
    Performance.Observe(Dt,(AfterSolver-Start)*1000,(FPlatformTime::Seconds()-AfterSolver)*1000,Update);
}
void APristineWaterRegion::AddSplash(FVector P,FVector Velocity,float Radius,float Strength)
{
    if(P.ContainsNaN()||Velocity.ContainsNaN()||!FMath::IsFinite(Radius)||!FMath::IsFinite(Strength)) return;
    const FVector L=GetActorTransform().InverseTransformPosition(P); const int32 Home=Cell(L);
    if(Home==INDEX_NONE||Grid.Depth[Home]<1.e-5) return;
    const FVector V=GetActorTransform().InverseTransformVectorNoScale(Velocity).GetClampedToMaxSize(3000);
    Radius=FMath::Clamp(Radius,1.f,10000.f); Strength=FMath::Clamp(Strength,0.f,5.f);
    for(int32 Y=0;Y<Grid.N;++Y) for(int32 X=0;X<Grid.N;++X)
    {
        const FVector2D Delta(Min.X+(X+0.5)*Grid.DX*100-L.X,Min.Y+(Y+0.5)*Grid.DY*100-L.Y);
        const double Weight=FMath::Max(0.0,1-Delta.Size()/Radius)*Strength;
        if(Weight>0) Grid.Impulse(X,Y,(FVector2D(V.X,V.Y)*0.01+Delta.GetSafeNormal()*FMath::Abs(V.Z)*0.003)*Weight);
    }
    const int32 Count=FMath::Min(FMath::Max(0,FMath::Clamp(MaxDroplets,0,1024)-Drops.Num()),FMath::RoundToInt(Strength*16));
    for(int32 I=0;I<Count;++I)
    {
        const double Volume=FMath::Min(0.000008,Grid.Depth[Home]*Grid.DX*Grid.DY*0.002);
        if(Volume<1.e-10) break;
        Grid.RemoveVolume(Home,Volume);
        FDrop D; D.P=FVector(L.X,L.Y,(Grid.Bed[Home]+Grid.Depth[Home])*100+3);
        D.V=V*0.25+FVector(Random.FRandRange(-120,120),Random.FRandRange(-120,120),FMath::Abs(V.Z)*0.3+Random.FRandRange(100,250))*Strength;
        D.Volume=Volume; D.Age=0; D.Home=Home; Drops.Add(D);
    }
}
void APristineWaterRegion::UpdateDrops(float Dt)
{
    for(int32 I=Drops.Num()-1;I>=0;--I)
    {
        FDrop& D=Drops[I]; const FVector Old=D.P; D.V.Z-=981*Dt; D.P+=D.V*Dt; D.Age+=Dt;
        int32 C=Cell(D.P);
        if(C==INDEX_NONE) {D.P.X=Old.X; D.P.Y=Old.Y; D.V.X*=-0.25; D.V.Y*=-0.25; C=Cell(D.P);}
        const bool Return=C!=INDEX_NONE&&D.V.Z<0&&D.P.Z<=(Grid.Bed[C]+Grid.Depth[C])*100;
        if(Return||D.Age>5)
        {
            const int32 Target=C==INDEX_NONE?D.Home:C;
            Grid.AddVolume(Target,D.Volume,FVector2D(D.V.X,D.V.Y)*0.01);
            Drops.RemoveAtSwap(I);
        }
    }
}
void APristineWaterRegion::RenderDrops()
{
    // Render once per visual update, never once per CFL substep; retain instance storage.
    while(Splashes->GetInstanceCount()>Drops.Num()) Splashes->RemoveInstance(Splashes->GetInstanceCount()-1);
    for(int32 I=0;I<Drops.Num();++I)
    {
        const FDrop& D=Drops[I];
        const double Radius=FMath::Pow(D.Volume*3/(4*PI),1.0/3)*100;
        const FTransform T(FQuat::Identity,D.P,FVector(Radius/50));
        if(I<Splashes->GetInstanceCount()) Splashes->UpdateInstanceTransform(I,T,false,I==Drops.Num()-1,true);
        else Splashes->AddInstance(T,false);
    }
}
void APristineWaterRegion::UpdateMesh()
{
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<int32> T; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
    const int32 R=Grid.N;
    if(R==0) return;
    for(int32 Y=0;Y<=R;++Y) for(int32 X=0;X<=R;++X)
    {
        double H=0; int32 Count=0;
        for(int32 B=Y-1;B<=Y;++B) for(int32 A=X-1;A<=X;++A)
            if(A>=0&&B>=0&&A<R&&B<R&&Grid.Active[B*R+A]&&Grid.Depth[B*R+A]>1.e-5) {H+=Grid.Bed[B*R+A]+Grid.Depth[B*R+A]; ++Count;}
        V.Add(FVector(Min.X+X*Grid.DX*100,Min.Y+Y*Grid.DY*100,Count?H/Count*100:WaterLevel)); UV.Add(FVector2D(double(X)/R,double(Y)/R));
        Colors.Add(FLinearColor::Black);
    }
    for(int32 Y=0;Y<R;++Y) for(int32 X=0;X<R;++X)
    {
        const int32 I=Y*R+X; if(!Grid.Active[I]||Grid.Depth[I]<=1.e-5) continue;
        const int32 A=Y*(R+1)+X,B=A+1,C=A+R+1,D=C+1; T.Append({A,C,B,B,C,D});
        const double Speed=Grid.Velocity(I).Size();
        const float Foam=FMath::Clamp((Speed-0.4)*0.2,0.0,1.0);
        for(int32 J:{A,B,C,D}) Colors[J].R=FMath::Max(Colors[J].R,Foam);
    }
    if(T.IsEmpty()) {Surface->ClearAllMeshSections(); LastTriangles.Empty(); return;}
    // The grid already shares vertices. Accumulate area-weighted normals in O(triangles),
    // avoiding the generic library's all-vertex overlap search on every update.
    N.Init(FVector::ZeroVector,V.Num()); Tangents.Reserve(V.Num());
    for(int32 I=0;I<T.Num();I+=3)
    {
        const int32 A=T[I],B=T[I+1],C=T[I+2];
        const FVector Normal=FVector::CrossProduct(V[C]-V[A],V[B]-V[A]);
        N[A]+=Normal; N[B]+=Normal; N[C]+=Normal;
    }
    for(FVector& Normal:N)
    {
        Normal=Normal.GetSafeNormal(SMALL_NUMBER,FVector::UpVector);
        Tangents.Add(FProcMeshTangent(FVector(Normal.Z,0,-Normal.X).GetSafeNormal(),false));
    }
    const FProcMeshSection* Existing=Surface->GetProcMeshSection(0);
    if(Existing&&Existing->ProcVertexBuffer.Num()==V.Num()&&LastTriangles==T)
        Surface->UpdateMeshSection_LinearColor(0,V,N,UV,Colors,Tangents);
    else {Surface->CreateMeshSection_LinearColor(0,V,T,N,UV,Colors,Tangents,false); LastTriangles=MoveTemp(T);}
}



void APristineWaterRegion::ApplyCinematicPreset()
{
    SurfaceMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineCinematic.M_PristineCinematic")));
    if(auto* Material=SurfaceMaterial.LoadSynchronous()) Surface->SetMaterial(0,Material);
    Water->bAssignLiquidMaterialOnApply=false; Water->BindToMesh(Surface); Water->ApplyCinematicLook(true);
    Performance.Preset=EPristinePerformancePreset::Custom; Performance.MaxSurfaceHz=60; Performance.BudgetMs=8; Performance.bAdaptiveVisuals=false;
    ApplyPerformanceSettings();
}
