#include "PristineWaterPerformance.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "RHI.h"

FPristineWaterHardware UPristineWaterPerformanceLibrary::GetWaterHardware()
{
    FPristineWaterHardware H;
    H.PhysicalCores=FPlatformMisc::NumberOfCores();
    H.RAMGiB=double(FPlatformMemory::GetConstants().TotalPhysical)/(1024*1024*1024);
    H.GPU=GRHIAdapterName; H.bRayTracingSupported=GRHISupportsRayTracing;
    // A conservative initial heuristic, NOT a GPU benchmark. Measured adaptation follows.
    H.RecommendedPreset=H.PhysicalCores>=8&&H.RAMGiB>=24&&H.bRayTracingSupported?EPristinePerformancePreset::High:
        H.PhysicalCores>=4&&H.RAMGiB>=12?EPristinePerformancePreset::Balanced:EPristinePerformancePreset::Low;
    return H;
}
EPristinePerformancePreset UPristineWaterPerformanceLibrary::Resolve(EPristinePerformancePreset P)
{return P==EPristinePerformancePreset::Auto?GetWaterHardware().RecommendedPreset:P;}
void UPristineWaterPerformanceLibrary::Apply(FPristineWaterPerformance& S)
{
    const auto P=Resolve(S.Preset);
    if(P!=EPristinePerformancePreset::Custom)
    {
        S.MaxSurfaceHz=P==EPristinePerformancePreset::Low?12:P==EPristinePerformancePreset::Balanced?24:40;
        S.BudgetMs=P==EPristinePerformancePreset::Low?2:P==EPristinePerformancePreset::Balanced?3:5;
    }
    S.ResetMeasurements();
}
void FPristineWaterPerformance::ResetMeasurements()
{
    EffectiveSurfaceHz=FMath::Clamp(MaxSurfaceHz,1.f,60.f);
    AverageSolverMs=AverageMeshMs=UnsimulatedSeconds=ObservationTime=0; bSolverOverBudget=false;
}
void FPristineWaterPerformance::Observe(float Dt,float Solver,float Mesh,bool Updated)
{
    if(!FMath::IsFinite(Dt)||Dt<=0) return;
    const float Alpha=1-FMath::Exp(-FMath::Min(Dt,0.1f)/0.5f);
    AverageSolverMs=FMath::Lerp(AverageSolverMs,FMath::Max(0.f,Solver),Alpha);
    if(Updated) AverageMeshMs=AverageMeshMs>0?FMath::Lerp(AverageMeshMs,FMath::Max(0.f,Mesh),0.2f):FMath::Max(0.f,Mesh);
    const float Budget=FMath::Clamp(BudgetMs,0.1f,30.f),FPS=FMath::Clamp(TargetFPS,15.f,144.f);
    bSolverOverBudget=AverageSolverMs>Budget;
    const float MaxHz=FMath::Min(FMath::Clamp(MaxSurfaceHz,1.f,60.f),FPS);
    const float MinHz=FMath::Min(FMath::Clamp(MinSurfaceHz,1.f,30.f),MaxHz);
    if(!bAdaptiveVisuals) {EffectiveSurfaceHz=MaxHz; return;}
    ObservationTime+=Dt;
    if(ObservationTime<0.5f) return; // Avoid oscillating on individual frames.
    ObservationTime=0;
    const float Wanted=FMath::Clamp((Budget-AverageSolverMs)*FPS/FMath::Max(0.01f,AverageMeshMs),MinHz,MaxHz);
    if(Wanted<EffectiveSurfaceHz*0.85f) EffectiveSurfaceHz=FMath::Max(Wanted,EffectiveSurfaceHz*0.75f);
    else if(Wanted>EffectiveSurfaceHz*1.2f) EffectiveSurfaceHz=FMath::Min(Wanted,EffectiveSurfaceHz+2);
    EffectiveSurfaceHz=FMath::Clamp(EffectiveSurfaceHz,MinHz,MaxHz);
}
