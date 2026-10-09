#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PristineWaterMeshComponent.generated.h"

UCLASS()
class PRISTINEWATERPHYSICS_API UPristineWaterMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()
public:
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override
    {
        const FBoxSphereBounds Base=Super::CalcBounds(LocalToWorld);
        // Multiplying flat mesh bounds cannot account for vertex shader displacement.
        return FBoxSphereBounds(Base.GetBox().ExpandBy(FVector(0,0,1000)));
    }
};
