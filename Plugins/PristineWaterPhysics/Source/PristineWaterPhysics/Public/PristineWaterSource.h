#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PristineWaterPerformance.h"
#include "PristineWaterSource.generated.h"

class UProceduralMeshComponent;
class UPristineWaterComponent;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EPristineWaterBody : uint8 { Puddle, Pond, Lake, Sea, Ocean, Custom };

UENUM(BlueprintType)
enum class EPristineWaterQuality : uint8 { Low, Medium, High };

UENUM(BlueprintType)
enum class EPristineWaterShape : uint8 { Rectangle, Ellipse };

USTRUCT(BlueprintType)
struct PRISTINEWATERPHYSICS_API FPristineWaterSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Water") bool bInside = false;
    UPROPERTY(BlueprintReadOnly, Category="Water") float SurfaceZ = 0;
    UPROPERTY(BlueprintReadOnly, Category="Water") FVector Normal = FVector::UpVector;
    UPROPERTY(BlueprintReadOnly, Category="Water") float Depth = 0;
    UPROPERTY(BlueprintReadOnly, Category="Water") float DensityKgM3 = 1000;
};

UCLASS(Blueprintable, ClassGroup=(Rendering))
class PRISTINEWATERPHYSICS_API APristineWaterSource : public AActor
{
    GENERATED_BODY()
public:
    APristineWaterSource();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Water") TObjectPtr<UProceduralMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Water") TObjectPtr<UPristineWaterComponent> Water;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water|Source") EPristineWaterBody BodyType = EPristineWaterBody::Lake;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Water|Source") EPristineWaterQuality Quality = EPristineWaterQuality::Medium;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Source") EPristineWaterShape Shape = EPristineWaterShape::Ellipse;
    // Half width and half length in centimeters. Use these instead of actor scale.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Source", meta=(ClampMin="10", ClampMax="2000000", Units="cm")) FVector2D HalfExtent = FVector2D(10000, 10000);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Source", meta=(ClampMin="1", ClampMax="100000", Units="cm")) float GameplayDepth = 1000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Appearance", meta=(ClampMin="0.001", ClampMax="1")) float Roughness = 0.075f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Appearance", meta=(ClampMin="10", ClampMax="10000", Units="cm")) float NormalTiling = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Appearance", meta=(ClampMin="0", ClampMax="1")) float ShoreFoam = 0.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Appearance", meta=(ClampMin="1", ClampMax="2000", Units="cm")) float ShoreWidth = 100;
    // Ocean only: recenter a fixed-size surface on local player zero at 5 Hz.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Optimization") bool bFollowCamera = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Optimization") FPristineWaterPerformance Performance;
    // Actual wave geometry for ray-traced intersections, updated at the selected visual rate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Optimization") bool bRayTracingWaveGeometry = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Optimization", meta=(ClampMin="0", Units="cm")) float MaxDrawDistance = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Materials") TSoftObjectPtr<UMaterialInterface> MaterialOverride;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Materials") TSoftObjectPtr<UMaterialInterface> LowMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Materials") TSoftObjectPtr<UMaterialInterface> MediumMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Materials") TSoftObjectPtr<UMaterialInterface> HighMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water|Materials") TSoftObjectPtr<UMaterialInterface> GeometryMaterial;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water") void RebuildSurface();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water") void ApplyAppearance();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water") void ApplyBodyPreset();
    UFUNCTION(BlueprintCallable, Category="Water") void SetBodyType(EPristineWaterBody NewType);
    UFUNCTION(BlueprintCallable, Category="Water") void SetQuality(EPristineWaterQuality NewQuality);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water|Optimization") void ApplyPerformanceSettings();
    UFUNCTION(BlueprintCallable, Category="Water|Optimization") void SetPerformancePreset(EPristinePerformancePreset Preset);
    // High tier: four four-second ripples, using geometry when ray-tracing wave geometry is enabled.
    UFUNCTION(BlueprintCallable, Category="Water") void AddRipple(FVector WorldPosition, float Strength = 1.0f);
    UFUNCTION(BlueprintPure, Category="Water") FPristineWaterSample SampleWater(FVector WorldPosition) const;
    UFUNCTION(BlueprintPure, Category="Water") FVector GetSphereBuoyancyForce(FVector Center, float RadiusCm) const;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water|Cinematic") void ApplyCinematicPreset();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
private:
    int32 RippleIndex = 0;
    FVector4 GeometryRipples[4] = {FVector4(0,0,-100,0),FVector4(0,0,-100,0),FVector4(0,0,-100,0),FVector4(0,0,-100,0)};
    FVector2D BuiltExtent = FVector2D::ZeroVector;
    EPristineWaterQuality BuiltQuality = EPristineWaterQuality::Low;
    EPristineWaterShape BuiltShape = EPristineWaterShape::Rectangle;
    bool bBuiltOcean = false;
    TArray<FVector> RestVertices;
    TArray<FVector2D> SurfaceUV;
    void UpdateWaveGeometry();
};
