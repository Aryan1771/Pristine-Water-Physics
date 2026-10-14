#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PristineWaterCharacter.generated.h"

class UCameraComponent;
class UChildActorComponent;
class APristineWaterRegion;
class UMaterialInstanceDynamic;
class USpringArmComponent;
class UAnimSequence;
class USkeletalMeshComponent;
class UMaterialInterface;

// Standalone keyboard/mouse test character. Visual can host an assembled MetaHuman Blueprint.
UCLASS(Blueprintable)
class PRISTINEWATERPHYSICS_API APristineWaterCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    APristineWaterCharacter();
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable,Category="Water") void RefreshWaterRegions();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UChildActorComponent> Visual;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") bool bSwimming=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") bool bUnderwater=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water",meta=(ClampMin="50",ClampMax="600")) float SwimSpeed=180;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water") bool bThirdPerson=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Demo") TArray<FVector> TestLocations;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Animation") TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Animation") TObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Animation") TObjectPtr<UAnimSequence> SwimAnimation;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering") TSoftObjectPtr<UMaterialInterface> UnderwaterEffect;
protected:
    virtual void BeginPlay() override;
private:
    TArray<TWeakObjectPtr<APristineWaterRegion>> Regions;
    float WakeTimer=0;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> UnderwaterMaterial;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> VisualBody;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveAnimation;
};
