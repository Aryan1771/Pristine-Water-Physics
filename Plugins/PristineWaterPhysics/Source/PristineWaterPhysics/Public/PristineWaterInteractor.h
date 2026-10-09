#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PristineWaterInteractor.generated.h"
class APristineWaterRegion;
class UPrimitiveComponent;

// Attach to a physics actor. Assign the region explicitly (no per-frame world searches).
UCLASS(ClassGroup=(Water),meta=(BlueprintSpawnableComponent))
class PRISTINEWATERPHYSICS_API UPristineWaterInteractor : public UActorComponent
{
    GENERATED_BODY()
public:
    UPristineWaterInteractor();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water") TObjectPtr<APristineWaterRegion> Region;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water") TObjectPtr<UPrimitiveComponent> PhysicsBody;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water",meta=(ClampMin="0.1",Units="cm")) float Radius=25;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water",meta=(ClampMin="0",ClampMax="5")) float DragCoefficient=0.47f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water") FVector LocalCenter=FVector::ZeroVector;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
private:
    bool bWasSubmerged=false;
};
