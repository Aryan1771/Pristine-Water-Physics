#include "PristineWaterSource.h"
#include "PristineWaterComponent.h"
#include "ProceduralMeshComponent.h"
#include "PristineWaterMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

APristineWaterSource::APristineWaterSource()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.2f;
    Surface = CreateDefaultSubobject<UPristineWaterMeshComponent>(TEXT("WaterSurface"));
    SetRootComponent(Surface);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Surface->SetGenerateOverlapEvents(false);
    Surface->SetCastShadow(false);
    Surface->bAffectDistanceFieldLighting = false;
    Surface->bUseAsyncCooking = true;
    Water = CreateDefaultSubobject<UPristineWaterComponent>(TEXT("WaterSettings"));
    Water->bAssignLiquidMaterialOnApply = false;
    LowMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineLow.M_PristineLow")));
    MediumMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineMedium.M_PristineMedium")));
    HighMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineHigh.M_PristineHigh")));
    GeometryMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineFluid.M_PristineFluid")));
}

void APristineWaterSource::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const bool Ocean = BodyType == EPristineWaterBody::Ocean;
    if (Surface->GetNumSections() == 0 || BuiltExtent != HalfExtent || BuiltQuality != Quality || BuiltShape != Shape || bBuiltOcean != Ocean)
        RebuildSurface();
    ApplyAppearance();
}

void APristineWaterSource::RebuildSurface()
{
    if (IsTemplate()) return;
    HalfExtent.X = FMath::Clamp(HalfExtent.X, 10.0, 2000000.0);
    HalfExtent.Y = FMath::Clamp(HalfExtent.Y, 10.0, 2000000.0);
    const int32 N = Quality == EPristineWaterQuality::Low ? 32 : (Quality == EPristineWaterQuality::Medium ? 64 : 128);
    const bool Ocean = BodyType == EPristineWaterBody::Ocean;
    TArray<FVector> Vertices, Normals;
    TArray<FVector2D> UV;
    TArray<int32> Indices;
    Vertices.Reserve((N+1)*(N+1));
    UV.Reserve((N+1)*(N+1));
    for (int32 Y=0; Y<=N; ++Y)
    for (int32 X=0; X<=N; ++X)
    {
        double U = 2.0 * X/N - 1, V = 2.0 * Y/N - 1;
        if (Ocean)
        {
            // Concentrate vertices near the camera while keeping a continuous horizon mesh.
            U = FMath::Sign(U) * FMath::Pow(FMath::Abs(U), 3.0);
            V = FMath::Sign(V) * FMath::Pow(FMath::Abs(V), 3.0);
        }
        else if (Shape == EPristineWaterShape::Ellipse)
        {
            const double OriginalU = U;
            U *= FMath::Sqrt(1.0 - V*V*0.5);
            V *= FMath::Sqrt(1.0 - OriginalU*OriginalU*0.5);
        }
        Vertices.Add(FVector(U*HalfExtent.X, V*HalfExtent.Y, 0));
        UV.Add(FVector2D(U*0.5+0.5, V*0.5+0.5));
    }
    Normals.Init(FVector::UpVector, Vertices.Num());
    Indices.Reserve(N*N*6);
    for (int32 Y=0; Y<N; ++Y)
    for (int32 X=0; X<N; ++X)
    {
        const int32 A=Y*(N+1)+X, B=A+1, C=A+N+1, D=C+1;
        Indices.Append({A,C,B,B,C,D});
    }
    TArray<FLinearColor> Colors; Colors.Init(FLinearColor::Black,Vertices.Num());
    Surface->CreateMeshSection_LinearColor(0, Vertices, Indices, Normals, UV, Colors, TArray<FProcMeshTangent>(), false);
    RestVertices=Vertices; SurfaceUV=UV;
    BuiltExtent = HalfExtent; BuiltQuality = Quality; BuiltShape = Shape; bBuiltOcean = Ocean;
    ApplyAppearance();
}

void APristineWaterSource::ApplyAppearance()
{
    UMaterialInterface* Parent = MaterialOverride.IsNull() ?
        (Quality == EPristineWaterQuality::Low ? LowMaterial.LoadSynchronous() :
        Quality == EPristineWaterQuality::Medium ? MediumMaterial.LoadSynchronous() : HighMaterial.LoadSynchronous()) : MaterialOverride.LoadSynchronous();
    if(bRayTracingWaveGeometry && MaterialOverride.IsNull() && Quality!=EPristineWaterQuality::Low)
        if(UMaterialInterface* Fluid=GeometryMaterial.LoadSynchronous()) Parent=Fluid;
    if (Parent && (!Water->DynamicMaterial || Water->DynamicMaterial->Parent != Parent || Surface->GetMaterial(0) != Water->DynamicMaterial))
        Surface->SetMaterial(0, Parent);
    Water->bAssignLiquidMaterialOnApply = false;
    Water->BindToMesh(Surface);
    if (UMaterialInstanceDynamic* MID = Water->DynamicMaterial)
    {
        MID->SetScalarParameterValue(TEXT("Roughness"), FMath::Clamp(Roughness, 0.001f, 1.0f));
        MID->SetScalarParameterValue(TEXT("NormalTiling"), FMath::Max(10.0f, NormalTiling));
        MID->SetScalarParameterValue(TEXT("ShoreFoam"), FMath::Clamp(ShoreFoam, 0.0f, 1.0f));
        MID->SetScalarParameterValue(TEXT("ShoreWidth"), FMath::Max(1.0f, ShoreWidth));
        if(bRayTracingWaveGeometry) MID->SetScalarParameterValue(TEXT("WaveAmplitude"),0);
    }
    Surface->SetCullDistance(MaxDrawDistance);
    SetActorTickInterval(bRayTracingWaveGeometry?1.f/FMath::Clamp(Performance.EffectiveSurfaceHz,1.f,60.f):0.2f);
    SetActorTickEnabled((bRayTracingWaveGeometry || (BodyType == EPristineWaterBody::Ocean && bFollowCamera)) && GetNetMode() != NM_DedicatedServer);
    if(bRayTracingWaveGeometry) UpdateWaveGeometry();
    else if(!RestVertices.IsEmpty())
    {
        TArray<FVector> Normals; Normals.Init(FVector::UpVector,RestVertices.Num());
        Surface->UpdateMeshSection_LinearColor(0,RestVertices,Normals,SurfaceUV,TArray<FLinearColor>(),TArray<FProcMeshTangent>());
    }
}

void APristineWaterSource::ApplyBodyPreset()
{
    if (BodyType==EPristineWaterBody::Custom) return;
    NormalTiling=500; ShoreFoam=0.3f; Roughness=0.075f;
    Water->SetLiquidType(BodyType == EPristineWaterBody::Ocean || BodyType == EPristineWaterBody::Sea ? EPristineLiquidType::Ocean : EPristineLiquidType::Water);
    Shape = EPristineWaterShape::Ellipse;
    switch (BodyType)
    {
    case EPristineWaterBody::Puddle: HalfExtent=FVector2D(150,100); GameplayDepth=4; Water->Motion.WaveAmplitude=0.15f; Water->Volume.VolumeDepth=20; NormalTiling=120; ShoreFoam=0; break;
    case EPristineWaterBody::Pond: HalfExtent=FVector2D(1500,1000); GameplayDepth=180; Water->Motion.WaveAmplitude=1.5f; NormalTiling=250; break;
    case EPristineWaterBody::Lake: HalfExtent=FVector2D(10000,7000); GameplayDepth=1500; Water->Motion.WaveAmplitude=8; NormalTiling=500; break;
    case EPristineWaterBody::Sea: HalfExtent=FVector2D(100000,100000); GameplayDepth=10000; Shape=EPristineWaterShape::Rectangle; Water->Motion.WaveAmplitude=45; NormalTiling=700; break;
    case EPristineWaterBody::Ocean: HalfExtent=FVector2D(1000000,1000000); GameplayDepth=100000; Shape=EPristineWaterShape::Rectangle; Water->Motion.WaveAmplitude=80; Water->Motion.WaveFrequency=0.35f; NormalTiling=900; break;
    default: break;
    }
    RebuildSurface();
}

void APristineWaterSource::SetBodyType(EPristineWaterBody NewType) { BodyType=NewType; ApplyBodyPreset(); }
void APristineWaterSource::SetQuality(EPristineWaterQuality NewQuality) { Quality=NewQuality; RebuildSurface(); }

void APristineWaterSource::SetPerformancePreset(EPristinePerformancePreset P)
{ Performance.Preset=P; ApplyPerformanceSettings(); }
void APristineWaterSource::ApplyPerformanceSettings()
{
    UPristineWaterPerformanceLibrary::Apply(Performance);
    const auto P=UPristineWaterPerformanceLibrary::Resolve(Performance.Preset);
    if(P!=EPristinePerformancePreset::Custom)
        Quality=P==EPristinePerformancePreset::Low?EPristineWaterQuality::Low:P==EPristinePerformancePreset::Balanced?EPristineWaterQuality::Medium:EPristineWaterQuality::High;
    RebuildSurface();
}

void APristineWaterSource::BeginPlay()
{
    Super::BeginPlay();
    ApplyPerformanceSettings();
}

void APristineWaterSource::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (BodyType == EPristineWaterBody::Ocean && bFollowCamera)
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this,0))
    {
        const FVector P=Camera->GetCameraLocation();
        SetActorLocation(FVector(FMath::GridSnap(P.X, 100.0), FMath::GridSnap(P.Y, 100.0), GetActorLocation().Z));
    }
    if(bRayTracingWaveGeometry)
    {
        const double Start=FPlatformTime::Seconds();
        UpdateWaveGeometry();
        Performance.Observe(DeltaSeconds,0,(FPlatformTime::Seconds()-Start)*1000,true);
        SetActorTickInterval(1.f/FMath::Clamp(Performance.EffectiveSurfaceHz,1.f,60.f));
    }
}

void APristineWaterSource::UpdateWaveGeometry()
{
    if(RestVertices.IsEmpty()) return;
    TArray<FVector> Vertices=RestVertices,Normals; TArray<FProcMeshTangent> Tangents;
    Normals.Reserve(Vertices.Num()); Tangents.Reserve(Vertices.Num());
    for(FVector& P:Vertices)
    {
        const auto S=SampleWater(GetActorTransform().TransformPosition(P));
        P.Z=S.SurfaceZ-GetActorLocation().Z;
        Normals.Add(GetActorTransform().InverseTransformVectorNoScale(S.Normal));
        const FVector Normal=Normals.Last(); Tangents.Add(FProcMeshTangent(FVector(Normal.Z,0,-Normal.X).GetSafeNormal(),false));
    }
    Surface->UpdateMeshSection_LinearColor(0,Vertices,Normals,SurfaceUV,TArray<FLinearColor>(),Tangents);
    if(Water->DynamicMaterial) Water->DynamicMaterial->SetScalarParameterValue(TEXT("WaveAmplitude"),0);
}

void APristineWaterSource::AddRipple(FVector Position, float Strength)
{
    if (!GetWorld() || Quality!=EPristineWaterQuality::High) return;
    const FName Name(*FString::Printf(TEXT("Interaction%d"), RippleIndex));
    const float BoundedStrength=FMath::Clamp(Strength,0.0f,2.0f);
    GeometryRipples[RippleIndex]=FVector4(Position.X,Position.Y,GetWorld()->GetTimeSeconds(),BoundedStrength);
    if(Water->DynamicMaterial) Water->DynamicMaterial->SetVectorParameterValue(Name,FLinearColor(Position.X,Position.Y,GetWorld()->GetTimeSeconds(),BoundedStrength));
    RippleIndex=(RippleIndex+1)%4;
}

FPristineWaterSample APristineWaterSource::SampleWater(FVector P) const
{
    FPristineWaterSample Result;
    const FVector Local=GetActorTransform().InverseTransformPosition(P);
    const double U=Local.X/FMath::Max(10.0,HalfExtent.X), V=Local.Y/FMath::Max(10.0,HalfExtent.Y);
    const bool Inside = (Shape==EPristineWaterShape::Ellipse && BodyType!=EPristineWaterBody::Ocean ? U*U+V*V<=1 : FMath::Abs(U)<=1 && FMath::Abs(V)<=1);
    const double T=GetWorld() ? GetWorld()->GetTimeSeconds() : 0;
    FVector2D D=Water->Motion.FlowDirection.GetSafeNormal();
    if (D.IsNearlyZero()) D=FVector2D(1,0);
    const FVector2D D2(-D.Y*0.8+D.X*0.6,D.X*0.8+D.Y*0.6);
    const double K=FMath::Max(0.0f,Water->Motion.WaveFrequency)*0.0062831853;
    const double Speed=Water->Motion.WaveSpeed;
    const double A=Water->Motion.WaveAmplitude;
    const double P1=(P.X*D.X+P.Y*D.Y)*K-T*Speed;
    const double P2=(P.X*D2.X+P.Y*D2.Y)*K*1.73-T*Speed*1.31;
    Result.SurfaceZ=GetActorLocation().Z+A*(FMath::Sin(P1)*0.65+FMath::Sin(P2)*0.35);
    FVector2D Gradient=A*K*(D*FMath::Cos(P1)*0.65+D2*FMath::Cos(P2)*0.35*1.73);
    if(bRayTracingWaveGeometry && Quality==EPristineWaterQuality::High)
    for(const FVector4& Ripple:GeometryRipples)
    {
        const double Age=T-Ripple.Z; if(Age<0||Age>=4||Ripple.W<=0) continue;
        const FVector2D Delta(P.X-Ripple.X,P.Y-Ripple.Y); const double Distance=Delta.Size(), Ring=Distance-Age*120;
        const double Envelope=2*Ripple.W*(1-Age/4)*FMath::Exp(-Ring*Ring/1800);
        Result.SurfaceZ+=Envelope*FMath::Sin(Ring*0.12);
        Gradient+=Delta.GetSafeNormal()*Envelope*(0.12*FMath::Cos(Ring*0.12)-Ring/900*FMath::Sin(Ring*0.12));
    }
    Result.Normal=FVector(-Gradient.X,-Gradient.Y,1).GetSafeNormal();
    Result.Depth=Result.SurfaceZ-P.Z;
    Result.bInside=Inside && P.Z>=GetActorLocation().Z-GameplayDepth;
    Result.DensityKgM3=FMath::Max(0.0f,Water->Volume.Density)*1000;
    return Result;
}

FVector APristineWaterSource::GetSphereBuoyancyForce(FVector Center,float RadiusCm) const
{
    const FPristineWaterSample S=SampleWater(Center);
    if (!S.bInside || RadiusCm<=0 || !GetWorld()) return FVector::ZeroVector;
    const double H=FMath::Clamp(double(S.Depth)+RadiusCm,0.0,2.0*RadiusCm);
    const double VolumeCm3=PI*H*H*(RadiusCm-H/3.0);
    return FVector(0,0,VolumeCm3*S.DensityKgM3*1.e-6*FMath::Abs(GetWorld()->GetGravityZ()));
}

#if WITH_EDITOR
void APristineWaterSource::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    if (Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(APristineWaterSource,BodyType)) ApplyBodyPreset();
    Super::PostEditChangeProperty(Event);
}
#endif

void APristineWaterSource::ApplyCinematicPreset()
{
    MaterialOverride=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineCinematic.M_PristineCinematic")));
    bRayTracingWaveGeometry=true; Quality=EPristineWaterQuality::High; Roughness=0.008f; ShoreFoam=0;
    Water->ApplyCinematicLook(true);
    Performance.Preset=EPristinePerformancePreset::Custom; Performance.MaxSurfaceHz=60; Performance.BudgetMs=8; Performance.bAdaptiveVisuals=false;
    ApplyPerformanceSettings();
}
