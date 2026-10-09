#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PristineWaterPerformance.generated.h"

UENUM(BlueprintType)
enum class EPristinePerformancePreset:uint8 { Auto, Low, Balanced, High, Custom };

USTRUCT(BlueprintType)
struct PRISTINEWATERPHYSICS_API FPristineWaterPerformance
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") EPristinePerformancePreset Preset=EPristinePerformancePreset::Auto;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance") bool bAdaptiveVisuals=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance",meta=(ClampMin="15",ClampMax="144")) float TargetFPS=60;
    // Per actor CPU budget: advisory; expensive individual operations cannot be preempted.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance",meta=(ClampMin="0.1",ClampMax="30")) float BudgetMs=3;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance",meta=(ClampMin="1",ClampMax="60")) float MaxSurfaceHz=30;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Performance",meta=(ClampMin="1",ClampMax="30")) float MinSurfaceHz=5;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Performance") float EffectiveSurfaceHz=30;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Performance") float AverageSolverMs=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Performance") float AverageMeshMs=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Performance") float UnsimulatedSeconds=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Performance") bool bSolverOverBudget=false;
    void Observe(float FrameSeconds,float SolverMs,float MeshMs,bool bUpdatedMesh);
    void ResetMeasurements();
private:
    float ObservationTime=0;
};

USTRUCT(BlueprintType)
struct PRISTINEWATERPHYSICS_API FPristineWaterHardware
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Hardware") int32 PhysicalCores=0;
    UPROPERTY(BlueprintReadOnly,Category="Hardware") float RAMGiB=0;
    UPROPERTY(BlueprintReadOnly,Category="Hardware") FString GPU;
    UPROPERTY(BlueprintReadOnly,Category="Hardware") bool bRayTracingSupported=false;
    UPROPERTY(BlueprintReadOnly,Category="Hardware") EPristinePerformancePreset RecommendedPreset=EPristinePerformancePreset::Low;
};

UCLASS()
class PRISTINEWATERPHYSICS_API UPristineWaterPerformanceLibrary:public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure,Category="Water|Performance") static FPristineWaterHardware GetWaterHardware();
    static EPristinePerformancePreset Resolve(EPristinePerformancePreset Preset);
    static void Apply(FPristineWaterPerformance& Settings);
};
