#include "PristineSmallFluid.h"
#include "Components/BoxComponent.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "PristineWaterProperties.h"
#include "HAL/PlatformTime.h"

namespace {FIntVector Bucket(FVector P,double H){return FIntVector(FMath::FloorToInt(P.X/H),FMath::FloorToInt(P.Y/H),FMath::FloorToInt(P.Z/H));}}
APristineSmallFluid::APristineSmallFluid()
{
    PrimaryActorTick.bCanEverTick=true;
    Container=CreateDefaultSubobject<UBoxComponent>(TEXT("ClosedContainer")); SetRootComponent(Container);
    Container->SetCollisionEnabled(ECollisionEnabled::NoCollision); Container->SetBoxExtent(HalfExtent);
    Surface=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ReconstructedLiquid")); Surface->SetupAttachment(Container);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LiquidMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineVolume.M_PristineVolume")));
}
void APristineSmallFluid::OnConstruction(const FTransform& T){Super::OnConstruction(T); ResetFluid();}
void APristineSmallFluid::BeginPlay(){Super::BeginPlay(); ApplyPerformanceSettings(); ResetFluid();}
void APristineSmallFluid::ApplyWaterTemperature()
{
    const auto P=UPristineWaterProperties::FreshWaterAtTemperature(TemperatureC);
    TemperatureC=P.TemperatureC; RestDensity=P.DensityKgM3; Density0=RestDensity;
    KinematicViscosity=P.KinematicViscosityM2S; SurfaceTension=P.SurfaceTensionNm;
    // Existing particle masses are retained: changing temperature must not create liquid.
}
void APristineSmallFluid::SetWaterTemperature(float T){TemperatureC=T; ApplyWaterTemperature();}
void APristineSmallFluid::SetFluidProperties(float Rho,float Nu,float Sigma,float Friction)
{
    if(!FMath::IsFinite(Rho)||!FMath::IsFinite(Nu)||!FMath::IsFinite(Sigma)||!FMath::IsFinite(Friction)) return;
    RestDensity=FMath::Clamp(Rho,100.f,2000.f); Density0=RestDensity;
    KinematicViscosity=FMath::Clamp(Nu,0.f,0.01f); SurfaceTension=FMath::Clamp(Sigma,0.f,1.f); WallFriction=FMath::Clamp(Friction,0.f,100.f);
}
void APristineSmallFluid::SetPerformancePreset(EPristinePerformancePreset P){Performance.Preset=P; ApplyPerformanceSettings();}
void APristineSmallFluid::ApplyPerformanceSettings()
{
    UPristineWaterPerformanceLibrary::Apply(Performance);
    const auto P=UPristineWaterPerformanceLibrary::Resolve(Performance.Preset);
    if(P!=EPristinePerformancePreset::Custom)
    {
        SurfaceResolution=P==EPristinePerformancePreset::Low?20:P==EPristinePerformancePreset::Balanced?28:40;
        SurfaceUpdatesPerSecond=Performance.MaxSurfaceHz;
    }
}
void APristineSmallFluid::Tick(float Dt){Super::Tick(Dt); if(bSimulate) AdvanceSimulation(Dt);}
void APristineSmallFluid::ResetFluid()
{
    if(IsTemplate()) return;
    ParticleRadius=FMath::Clamp(ParticleRadius,0.25f,5.f); ParticlesPerAxis=FMath::Clamp(ParticlesPerAxis,2,10);
    const double Minimum=ParticleRadius*(ParticlesPerAxis+2);
    HalfExtent=HalfExtent.ComponentMax(FVector(Minimum)); Container->SetBoxExtent(HalfExtent);
    Limits=HalfExtent*0.01; RadiusM=ParticleRadius*0.01; H=RadiusM*4;
    Density0=FMath::Clamp(RestDensity,100.f,2000.f); ParticleMass=Density0*FMath::Pow(2*RadiusM,3);
    Positions.Empty(); Velocities.Empty(); const int32 N=ParticlesPerAxis;
    for(int32 Z=0;Z<N;++Z) for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
        Positions.Add(FVector((X-(N-1)*0.5)*2*RadiusM,(Y-(N-1)*0.5)*2*RadiusM,(Z-(N-1)*0.5)*2*RadiusM));
    Velocities.Init(FVector::ZeroVector,Positions.Num());
    if(UMaterialInterface* M=LiquidMaterial.LoadSynchronous()) Surface->SetMaterial(0,M);
    BuildHash(); UpdateSurface();
}
void APristineSmallFluid::BuildHash()
{
    Hash.Reset();
    for(int32 I=0;I<Positions.Num();++I)
    {
        Hash.FindOrAdd(Bucket(Positions[I],H)).Add(I);
    }
}
void APristineSmallFluid::Neighbors(FVector P,TArray<int32>& Out) const
{
    Out.Reset(); const FIntVector B=Bucket(P,H);
    for(int32 Z=-1;Z<=1;++Z) for(int32 Y=-1;Y<=1;++Y) for(int32 X=-1;X<=1;++X)
        if(const auto* Found=Hash.Find(B+FIntVector(X,Y,Z)))
            Out.Append(*Found);
}
void APristineSmallFluid::AddImpulse(FVector V)
{
    if(V.ContainsNaN()) return;
    V=GetActorTransform().InverseTransformVectorNoScale(V).GetClampedToMaxSize(1000)*0.01;
    for(FVector& U:Velocities) U+=V;
}
void APristineSmallFluid::AdvanceSimulation(float Dt)
{
    LastSimulatedSeconds=0; if(!FMath::IsFinite(Dt)||Dt<=0||Positions.IsEmpty()) return;
    const double Start=FPlatformTime::Seconds();
    double Remaining=FMath::Min(double(Dt),0.1);
    for(int32 I=0;I<FMath::Clamp(MaxSubsteps,1,128)&&Remaining>1.e-7;++I)
    {
        double Speed=0; for(FVector V:Velocities) Speed=FMath::Max(Speed,V.Size());
        const double Acoustic=0.2*H/(FMath::Clamp(SoundSpeed,2.f,50.f)+Speed);
        const double Viscous=0.1*H*H/FMath::Max(double(KinematicViscosity),1.e-9);
        const double StepDt=FMath::Min3(Remaining,Acoustic,FMath::Min(Viscous,1.0/240));
        Step(StepDt); Remaining-=StepDt; LastSimulatedSeconds+=StepDt;
    }
    const double AfterSolver=FPlatformTime::Seconds();
    MeshElapsed+=LastSimulatedSeconds;
    const float Hz=FMath::Min(FMath::Clamp(SurfaceUpdatesPerSecond,1.f,60.f),FMath::Clamp(Performance.EffectiveSurfaceHz,1.f,60.f));
    const bool Update=GetNetMode()!=NM_DedicatedServer&&MeshElapsed>=1/Hz;
    if(Update) {BuildHash(); UpdateSurface(); MeshElapsed=0;}
    Performance.UnsimulatedSeconds=FMath::Max(0.f,Dt-LastSimulatedSeconds);
    Performance.Observe(Dt,(AfterSolver-Start)*1000,(FPlatformTime::Seconds()-AfterSolver)*1000,Update);
}
void APristineSmallFluid::Step(double Dt)
{
    BuildHash(); auto& Density=DensityScratch; auto& Pressure=PressureScratch;
    Density.SetNumUninitialized(Positions.Num()); Pressure.SetNumUninitialized(Positions.Num());
    auto& Near=NearScratch; Pairs.Reset();
    const double Poly=315/(64*PI*FMath::Pow(H,9)),Spiky=45/(PI*FMath::Pow(H,6));
    const double C=FMath::Clamp(SoundSpeed,2.f,50.f);
    const double Self=ParticleMass*Poly*FMath::Pow(H,6);
    for(double& Rho:Density) Rho=Self;
    for(int32 I=0;I<Positions.Num();++I)
    {
        Neighbors(Positions[I],Near);
        for(int32 J:Near)
        {
            if(J<=I) continue;
            const FVector D=Positions[I]-Positions[J]; const double R2=D.SizeSquared(),Q=H*H-R2;
            if(Q<=0) continue;
            const double Contribution=ParticleMass*Poly*Q*Q*Q;
            Density[I]+=Contribution; Density[J]+=Contribution;
            if(R2>1.e-16) Pairs.Add({I,J,D,FMath::Sqrt(R2)});
        }
    }
    for(int32 I=0;I<Positions.Num();++I)
    {Pressure[I]=C*C*FMath::Max(0.0,Density[I]-Density0); Density[I]=FMath::Max(Density[I],Density0*0.1);}
    auto& A=AccelerationScratch; A.Init(FVector(0,0,-9.81),Positions.Num());
    for(const FNeighborPair& PairData:Pairs)
    {
            const int32 I=PairData.I,J=PairData.J; const FVector D=PairData.Delta; const double R=PairData.Distance;
            const FVector Direction=D/R;
            FVector Pair=Direction*(ParticleMass*(Pressure[I]/FMath::Square(Density[I])+Pressure[J]/FMath::Square(Density[J]))*Spiky*FMath::Square(H-R));
            Pair+=(Velocities[J]-Velocities[I])*(FMath::Clamp(double(KinematicViscosity),0.0,0.01)*2*ParticleMass/(Density[I]+Density[J])*Spiky*(H-R));
            // Symmetric cohesion approximation. Preserves pair momentum; does not model contact angle.
            Pair-=Direction*(FMath::Clamp(double(SurfaceTension),0.0,1.0)/(Density0*RadiusM*RadiusM)*FMath::Pow(1-R/H,3)*0.05);
            A[I]+=Pair; A[J]-=Pair;
    }
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PristineSmallFluid),false,this);
    for(int32 I=0;I<Positions.Num();++I)
    {
        const FVector Old=Positions[I]; Velocities[I]+=A[I]*Dt; Positions[I]+=Velocities[I]*Dt;
        if(bSceneCollision&&GetWorld())
        {
            FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,GetActorTransform().TransformPosition(Old*100),GetActorTransform().TransformPosition(Positions[I]*100),FQuat::Identity,ECC_WorldStatic,FCollisionShape::MakeSphere(RadiusM*100),Params))
            {
                const FVector Normal=GetActorTransform().InverseTransformVectorNoScale(Hit.Normal);
                Positions[I]=GetActorTransform().InverseTransformPosition(Hit.Location+Hit.Normal*0.01)*0.01;
                const double VN=FVector::DotProduct(Velocities[I],Normal); if(VN<0) Velocities[I]-=Normal*VN;
                const FVector Tangential=Velocities[I]-Normal*FVector::DotProduct(Velocities[I],Normal);
                Velocities[I]-=Tangential*(1-FMath::Exp(-FMath::Clamp(WallFriction,0.f,100.f)*Dt));
            }
        }
        for(int32 Axis=0;Axis<3;++Axis)
        {
            const double Limit=Limits[Axis]-RadiusM;
            const bool Contact=FMath::Abs(Positions[I][Axis])>=Limit;
            if(Positions[I][Axis]<-Limit) {Positions[I][Axis]=-Limit; Velocities[I][Axis]=FMath::Max(0.0,Velocities[I][Axis]);}
            if(Positions[I][Axis]>Limit) {Positions[I][Axis]=Limit; Velocities[I][Axis]=FMath::Min(0.0,Velocities[I][Axis]);}
            if(Contact) for(int32 Tangent=0;Tangent<3;++Tangent) if(Tangent!=Axis) Velocities[I][Tangent]*=FMath::Exp(-FMath::Clamp(WallFriction,0.f,100.f)*Dt);
        }
    }
}
bool APristineSmallFluid::IsFluidStateValid() const
{
    for(int32 I=0;I<Positions.Num();++I)
        if(Positions[I].ContainsNaN()||Velocities[I].ContainsNaN()||FMath::Abs(Positions[I].X)>Limits.X||FMath::Abs(Positions[I].Y)>Limits.Y||FMath::Abs(Positions[I].Z)>Limits.Z) return false;
    return Positions.Num()==Velocities.Num();
}
void APristineSmallFluid::UpdateSurface()
{
    // Marching tetrahedra over a compact particle kernel. This is a continuous liquid skin,
    // including overhangs and detached drops, rather than a stack of rendered spheres.
    const int32 R=FMath::Clamp(SurfaceResolution,12,56), S=R+1;
    FBox Bounds(ForceInit); for(FVector P:Positions) Bounds+=P;
    Bounds=Bounds.ExpandBy(RadiusM*3);
    const FVector MeshMin=Bounds.Min.ComponentMax(-Limits),MeshMax=Bounds.Max.ComponentMin(Limits);
    const FVector CellSize=(MeshMax-MeshMin)/R;
    auto& Field=FieldScratch; Field.Init(0,S*S*S); TArray<int32> Near;
    const double Support=RadiusM*3;
    auto Point=[&](int32 X,int32 Y,int32 Z){return MeshMin+FVector(X*CellSize.X,Y*CellSize.Y,Z*CellSize.Z);};
    // Scatter each compact kernel only to samples inside its support. This avoids
    // 27 hash lookups per grid sample, including large empty parts of the box.
    const double InvSupport2=1/(Support*Support);
    for(FVector Particle:Positions)
    {
        FIntVector Lo,Hi;
        for(int Axis=0;Axis<3;++Axis)
        {
            Lo[Axis]=FMath::Clamp(FMath::CeilToInt((Particle[Axis]-Support-MeshMin[Axis])/CellSize[Axis]),1,R-1);
            Hi[Axis]=FMath::Clamp(FMath::FloorToInt((Particle[Axis]+Support-MeshMin[Axis])/CellSize[Axis]),1,R-1);
        }
        for(int Z=Lo.Z;Z<=Hi.Z;++Z) for(int Y=Lo.Y;Y<=Hi.Y;++Y) for(int X=Lo.X;X<=Hi.X;++X)
        {
            const double Q=1-FVector::DistSquared(Point(X,Y,Z),Particle)*InvSupport2;
            if(Q>0) Field[(Z*S+Y)*S+X]+=Q*Q*Q;
        }
    }
    for(int32 Z=0;Z<S;++Z) for(int32 Y=0;Y<S;++Y) for(int32 X=0;X<S;++X)
    {
        // Clip the reconstructed skin to the closed container.
        Field[(Z*S+Y)*S+X]=(X==0||Y==0||Z==0||X==R||Y==R||Z==R)?-0.55:Field[(Z*S+Y)*S+X]-0.55;
    }
    TArray<FVector> V,N; TArray<FVector2D> UV; TArray<int32> T; TArray<FProcMeshTangent> Tangents;
    const FIntVector Corners[8]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
    const int32 Tets[6][4]={{0,5,1,6},{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6}};
    for(int32 Z=0;Z<R;++Z) for(int32 Y=0;Y<R;++Y) for(int32 X=0;X<R;++X)
    {
        FVector P[8]; double F[8]; bool Positive=false,Negative=false;
        for(int32 I=0;I<8;++I) {const FIntVector C=Corners[I]+FIntVector(X,Y,Z); P[I]=Point(C.X,C.Y,C.Z); F[I]=Field[(C.Z*S+C.Y)*S+C.X]; Positive|=F[I]>0; Negative|=F[I]<=0;}
        if(!Positive||!Negative) continue;
        auto Edge=[&](int32 A,int32 B){return FMath::Lerp(P[A],P[B],F[A]/(F[A]-F[B]));};
        auto Triangle=[&](FVector A,FVector B,FVector C,FVector Outside)
        {
            // Unreal's clockwise winding points opposite Cross(B-A,C-A).
            if(FVector::DotProduct(FVector::CrossProduct(B-A,C-A),Outside)>0) Swap(B,C);
            const int32 Base=V.Num(); V.Append({A*100,B*100,C*100}); T.Append({Base,Base+1,Base+2});
            UV.Append({FVector2D(A.X,A.Y),FVector2D(B.X,B.Y),FVector2D(C.X,C.Y)});
        };
        for(const auto& Tet:Tets)
        {
            int32 In[4],Out[4],NI=0,NO=0; for(int32 I:Tet) {if(F[I]>0) In[NI++]=I; else Out[NO++]=I;}
            if(NI==0||NI==4) continue;
            FVector Direction=FVector::ZeroVector; for(int32 I=0;I<NO;++I) Direction+=P[Out[I]]/NO; for(int32 I=0;I<NI;++I) Direction-=P[In[I]]/NI;
            if(NI==1) Triangle(Edge(In[0],Out[0]),Edge(In[0],Out[1]),Edge(In[0],Out[2]),Direction);
            else if(NI==3) Triangle(Edge(Out[0],In[0]),Edge(Out[0],In[1]),Edge(Out[0],In[2]),Direction);
            else {const FVector A=Edge(In[0],Out[0]),B=Edge(In[0],Out[1]),C=Edge(In[1],Out[0]),D=Edge(In[1],Out[1]); Triangle(A,B,C,Direction); Triangle(B,D,C,Direction);}
        }
    }
    if(T.IsEmpty()) {Surface->ClearAllMeshSections(); return;}
    // Analytical kernel gradient gives smooth normals across tetrahedron boundaries.
    for(FVector Vertex:V)
    {
        const FVector P=Vertex*0.01; Neighbors(P,Near); FVector Gradient=FVector::ZeroVector;
        for(int32 I:Near) {const FVector D=P-Positions[I]; const double Q=1-D.SizeSquared()/(Support*Support); if(Q>0) Gradient+=D*(Q*Q);}
        N.Add(Gradient.GetSafeNormal(SMALL_NUMBER,FVector::UpVector));
    }
    Surface->CreateMeshSection_LinearColor(0,V,T,N,UV,TArray<FLinearColor>(),Tangents,false);
}
