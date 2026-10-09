#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PristineWaterPerformance.h"
#include "PristineSmallFluid.generated.h"

class UProceduralMeshComponent;
class UBoxComponent;
class UMaterialInterface;
UCLASS(Blueprintable,ClassGroup=(Water))
class PRISTINEWATERPHYSICS_API APristineSmallFluid : public AActor
{
    GENERATED_BODY()
public:
    APristineSmallFluid();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UBoxComponent> Container;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water") TObjectPtr<UProceduralMeshComponent> Surface;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering") TSoftObjectPtr<UMaterialInterface> LiquidMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Container",meta=(ClampMin="5",Units="cm")) FVector HalfExtent=FVector(12,12,18);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="0.25",ClampMax="5",Units="cm")) float ParticleRadius=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="2",ClampMax="10")) int32 ParticlesPerAxis=6;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="100",ClampMax="2000")) float RestDensity=998.2f;
    // Artificial acoustic speed for weakly-compressible SPH, not the physical sound speed of water.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Solver",meta=(ClampMin="2",ClampMax="50")) float SoundSpeed=12;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="0",ClampMax="0.01")) float KinematicViscosity=0.000001004f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="0",ClampMax="1")) float SurfaceTension=0.0728f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="0.01",ClampMax="80",UIMin="0.01",UIMax="80")) float TemperatureC=20;
    // Tangential wall damping per second. Zero gives free slip, larger values approximate no-slip.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Fluid",meta=(ClampMin="0",ClampMax="100")) float WallFriction=10;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Performance") FPristineWaterPerformance Performance;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Physical Properties") void ApplyWaterTemperature();
    UFUNCTION(BlueprintCallable,Category="Water|Physical Properties") void SetWaterTemperature(float Celsius);
    // Live controls preserve existing particle mass. Reset Fluid explicitly reseeds liquid.
    UFUNCTION(BlueprintCallable,Category="Water|Physical Properties") void SetFluidProperties(float DensityKgM3,float ViscosityM2S,float TensionNm,float WallDamping);
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Performance") void ApplyPerformanceSettings();
    UFUNCTION(BlueprintCallable,Category="Water|Performance") void SetPerformancePreset(EPristinePerformancePreset Preset);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Solver",meta=(ClampMin="1",ClampMax="128")) int32 MaxSubsteps=64;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering",meta=(ClampMin="12",ClampMax="56")) int32 SurfaceResolution=32;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Rendering",meta=(ClampMin="1",ClampMax="60")) float SurfaceUpdatesPerSecond=20;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Solver") bool bSceneCollision=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Water|Solver") bool bSimulate=true;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Water|Solver") float LastSimulatedSeconds=0;
    UFUNCTION(BlueprintCallable,CallInEditor,Category="Water|Fluid") void ResetFluid();
    UFUNCTION(BlueprintCallable,Category="Water|Fluid") void AddImpulse(FVector VelocityCmPerSecond);
    UFUNCTION(BlueprintCallable,Category="Water|Fluid") void AdvanceSimulation(float DeltaSeconds);
    UFUNCTION(BlueprintPure,Category="Water|Fluid") int32 GetParticleCount() const {return Positions.Num();}
    UFUNCTION(BlueprintPure,Category="Water|Fluid") double GetLiquidMassKg() const {return Positions.Num()*ParticleMass;}
    UFUNCTION(BlueprintPure,Category="Water|Fluid") bool IsFluidStateValid() const;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void BeginPlay() override;
private:
    TArray<FVector> Positions,Velocities;
    TArray<double> DensityScratch,PressureScratch,FieldScratch;
    TArray<FVector> AccelerationScratch;
    TArray<int32> NearScratch;
    struct FNeighborPair {int32 I,J; FVector Delta; double Distance;};
    TArray<FNeighborPair> Pairs;
    TMap<FIntVector,TArray<int32>> Hash;
    FVector Limits;
    double H=0.04,RadiusM=0.01,ParticleMass=0,Density0=998.2;
    float MeshElapsed=0;
    void BuildHash();
    void Neighbors(FVector P,TArray<int32>& Out) const;
    void Step(double Dt);
    void UpdateSurface();
};
