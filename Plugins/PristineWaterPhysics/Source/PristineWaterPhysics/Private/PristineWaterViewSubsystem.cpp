#include "PristineWaterViewSubsystem.h"
#include "PristineWaterRegion.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

class FPristineWaterViewExtension final : public FWorldSceneViewExtension
{
public:
    FPristineWaterViewExtension(const FAutoRegister& AutoRegister,UWorld* InWorld)
        : FWorldSceneViewExtension(AutoRegister,InWorld) {}
    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily&,FSceneView& View) override
    {
        if(const UWorld* ViewWorld=GetWorld())
        {
            if(const auto* Water=ViewWorld->GetSubsystem<UPristineWaterViewSubsystem>())
            {
                const float Depth=Water->GetImmersionDepth(View.ViewMatrices.GetViewOrigin());
                if(Depth>0)
                {
                    // This selects Unreal's underwater pass ordering, including the sky
                    // behind Single Layer Water. A camera post-process alone is insufficient.
                    View.UnderwaterDepth=FMath::Max(View.UnderwaterDepth,Depth);
                    View.WaterIntersection=EViewWaterIntersection::InsideWater;
                }
            }
        }
    }
};

void UPristineWaterViewSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ViewExtension=FSceneViewExtensions::NewExtension<FPristineWaterViewExtension>(GetWorld());
}

void UPristineWaterViewSubsystem::Deinitialize()
{
    ViewExtension.Reset();
    Super::Deinitialize();
}

bool UPristineWaterViewSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type==EWorldType::Game||Type==EWorldType::PIE||Type==EWorldType::Editor;
}

float UPristineWaterViewSubsystem::GetImmersionDepth(const FVector& Position) const
{
    float Depth=-1;
    for(TActorIterator<APristineWaterRegion> It(GetWorld());It;++It)
    {
        const auto Sample=It->SampleRegion(Position);
        if(Sample.bWet&&Sample.DepthCm>0) Depth=FMath::Max(Depth,Sample.DepthCm);
    }
    return Depth;
}
