#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PristineShallowGrid.h"
#include "PristineWaterPerformance.h"
#include "PristineWaterRegion.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UPristineWaterComponent;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FPristineRegionSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Water") bool bWet=false;
    UPROPERTY(BlueprintReadOnly,Category="Water") float SurfaceZ=0;
    UPROPERTY(BlueprintReadOnly,Category="Water") float DepthCm=0;
    UPROPERTY(BlueprintReadOnly,Category="Water") FVector Velocity=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly,Category="Water") float HydrostaticPressurePa=0;
};

// Horizontal, unit-scale actor. Spline control points define a closed polygon in local XY.
UCLASS(Blueprintable,ClassGroup=(Water))
class PRISTINEWATERPHYSICS_API APristineWaterRegion : public AActor
{
    GENERATED_BODY()
public:
    APristineWaterRegion();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<USplineComponent> Boundary;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UProceduralMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UInstancedStaticMeshComponent> Splashes;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UPristineWaterComponent> Water;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering") TSoftObjectPtr<UMaterialInterface> SurfaceMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering") TSoftObjectPtr<UMaterialInterface> SplashMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Region",meta=(ClampMin="8",ClampMax="128")) int32 Resolution=48;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Region",meta=(Units="cm")) float WaterLevel=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Region",meta=(ClampMin="1",Units="cm")) float DefaultDepth=200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Landscape",meta=(ClampMin="1",Units="cm")) float TraceAbove=10000;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Landscape",meta=(ClampMin="1",Units="cm")) float TraceBelow=100000;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Landscape") TEnumAsByte<ECollisionChannel> TerrainChannel=ECC_WorldStatic;
    // Optional whitelist: only these Landscape/mesh actors contribute bed collision.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Landscape") TArray<TObjectPtr<AActor>> TerrainActors;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water|Landscape") bool bLandscapeBakeValid=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water|Landscape") int32 LandscapeSampleCount=0;
    // A missing terrain hit is a closed cell, preventing leaks into unsampled space.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Simulation",meta=(ClampMin="0",ClampMax="10")) float BottomDrag=0.35f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Simulation",meta=(ClampMin="1",ClampMax="128")) int32 MaxSubsteps=32;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Simulation") bool bSimulate=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Performance") FPristineWaterPerformance Performance;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Physical Properties",meta=(ClampMin="0.01",ClampMax="80")) float TemperatureC=20;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Physical Properties") void ApplyWaterTemperature();
    UFUNCTION(BlueprintCallable,Category="Water|Physical Properties") void SetWaterTemperature(float Celsius);
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Performance") void ApplyPerformanceSettings();
    UFUNCTION(BlueprintCallable,Category="Water|Performance") void SetPerformancePreset(EPristinePerformancePreset Preset);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Splashes",meta=(ClampMin="0",ClampMax="1024")) int32 MaxDroplets=256;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water|Simulation") float LastSimulatedSeconds=0;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Landscape") void BakeLandscape();
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Landscape") void ClearLandscapeBake();
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Region") void ResetWater();
    UFUNCTION(BlueprintCallable,Category="Water|Region") void SetWaterLevel(float LocalHeightCm);
    UFUNCTION(BlueprintCallable,Category="Water|Simulation") void AdvanceSimulation(float DeltaSeconds);
    // Inject momentum and transfer liquid into ballistic droplets; no water is created.
    UFUNCTION(BlueprintCallable,Category="Water|Splashes") void AddSplash(FVector WorldPosition,FVector VelocityCmPerSecond,float RadiusCm=100,float Strength=1);
    UFUNCTION(BlueprintPure,Category="Water|Simulation") double GetWaterVolumeM3() const;
    UFUNCTION(BlueprintPure,Category="Water|Simulation") FPristineRegionSample SampleRegion(FVector WorldPosition) const;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Water|Cinematic") void ApplyCinematicPreset();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
private:
    friend class FPristineBakeTest;
    UPROPERTY() TArray<double> BakedBed;
    UPROPERTY() TArray<uint8> BakedValid;
    UPROPERTY() int32 BakedResolution=0;
    UPROPERTY() FVector2D BakedMin=FVector2D::ZeroVector;
    UPROPERTY() FVector2D BakedMax=FVector2D::ZeroVector;
    FPristineShallowGrid Grid;
    FVector2D Min,Max;
    struct FDrop {FVector P,V; double Volume; float Age; int32 Home;};
    TArray<FDrop> Drops;
    TArray<int32> LastTriangles;
    FRandomStream Random{4137};
    bool InPolygon(FVector2D P) const;
    int32 Cell(FVector Local) const;
    void UpdateMesh();
    void UpdateDrops(float Dt);
    void RenderDrops();
    float MeshElapsed=0;
};
