#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PristineWaterProperties.generated.h"

USTRUCT(BlueprintType)
struct PRISTINEWATERPHYSICS_API FPristineFreshWaterProperties
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="Water") float TemperatureC=20;
    UPROPERTY(BlueprintReadOnly,Category="Water") float DensityKgM3=998.2;
    UPROPERTY(BlueprintReadOnly,Category="Water") float DynamicViscosityPaS=0.001002;
    UPROPERTY(BlueprintReadOnly,Category="Water") float KinematicViscosityM2S=1.004e-6;
    UPROPERTY(BlueprintReadOnly,Category="Water") float SurfaceTensionNm=0.0728;
};

UCLASS()
class PRISTINEWATERPHYSICS_API UPristineWaterProperties:public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Uniform fresh liquid water at 1 bar, 0.01 to 80 C. Not a heat/phase-change solver.
    UFUNCTION(BlueprintPure,Category="Water|Physical Properties") static FPristineFreshWaterProperties FreshWaterAtTemperature(float Celsius);
};
