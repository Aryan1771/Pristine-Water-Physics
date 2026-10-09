#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"
#include "PristineWaterSettings.h"
#include "PristineWaterComponent.generated.h"

class UMeshComponent;

UCLASS(ClassGroup=(Rendering), meta=(BlueprintSpawnableComponent))
class PRISTINEWATERPHYSICS_API UPristineWaterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPristineWaterComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water")
    UMeshComponent* TargetWaterMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water")
    int32 MaterialSlot = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid")
    EPristineLiquidType LiquidType = EPristineLiquidType::Water;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid")
    bool bAssignLiquidMaterialOnApply = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid", meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
    TSoftObjectPtr<UMaterialInterface> WaterMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid", meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
    TSoftObjectPtr<UMaterialInterface> OceanMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid", meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
    TSoftObjectPtr<UMaterialInterface> OilMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water|Liquid", meta=(AllowedClasses="/Script/Engine.MaterialInterface"))
    TSoftObjectPtr<UMaterialInterface> CustomLiquidMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water")
    FPristineWaterOptics Optics;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water")
    FPristineWaterVolume Volume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pristine Water")
    FPristineWaterMotion Motion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category="Pristine Water")
    UMaterialInstanceDynamic* DynamicMaterial = nullptr;

    UFUNCTION(BlueprintCallable, Category="Pristine Water")
    void BindToMesh(UMeshComponent* Mesh, int32 Slot = 0);
    UFUNCTION(BlueprintCallable, Category="Pristine Water") void ApplyCinematicLook(bool bCalm = true);
    UFUNCTION(BlueprintCallable, Category="Pristine Water") void SetRoughness(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water")
    void ApplyWaterSettings();

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Liquid")
    void SetLiquidType(EPristineLiquidType NewLiquidType, bool bApplyPreset = true);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetReflectivity(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetRefractiveIndex(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetTotalInternalReflection(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetFresnelStrength(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetRefractionStrength(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetVolumeDepth(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetAbsorption(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetClarity(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetFoamAmount(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetWaveAmplitude(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetWaveFrequency(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetWaveSpeed(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetRippleStrength(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetFlowDirection(FVector2D Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetDensity(float Value);

    UFUNCTION(BlueprintCallable, Category="Pristine Water|Runtime")
    void SetLiquidColors(FLinearColor Shallow, FLinearColor Deep);

protected:
    virtual void BeginPlay() override;
    virtual void OnRegister() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> AppliedParent;

    void ApplyLiquidPreset(EPristineLiquidType NewLiquidType);
    UMaterialInterface* ResolveLiquidMaterial() const;
    void SetScalar(FName Name, float Value);
    void SetVector(FName Name, const FLinearColor& Value);
};
