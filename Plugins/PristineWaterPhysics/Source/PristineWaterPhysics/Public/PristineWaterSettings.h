#pragma once

#include "CoreMinimal.h"
#include "PristineWaterSettings.generated.h"

UENUM(BlueprintType)
enum class EPristineLiquidType : uint8
{
    Water UMETA(DisplayName="Water"),
    Ocean UMETA(DisplayName="Ocean / Deep Water"),
    Oil UMETA(DisplayName="Oil"),
    Custom UMETA(DisplayName="Custom / Other Liquid")
};

USTRUCT(BlueprintType)
struct FPristineWaterOptics
{
    GENERATED_BODY()

    // IOR of water is about 1.333. Higher values bend light more.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="1.0", ClampMax="2.5", UIMin="1.0", UIMax="1.8"))
    float RefractiveIndex = 1.333f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Reflectivity = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.001", ClampMax="1", UIMin="0.001", UIMax="0.2"))
    float Roughness = 0.075f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0", ClampMax="1"))
    float ShoreFoam = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(DisplayName="Fresnel Exponent", ClampMin="0.1", ClampMax="10.0"))
    float FresnelPower = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", ClampMax="2.0"))
    float FresnelStrength = 1.0f;

    // Artistic boost for total internal reflection look. Real TIR is shader/view dependent.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(DisplayName="Artistic TIR Boost", ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="1.0"))
    float TotalInternalReflection = 0.35f;

    // Screen-space refraction distortion strength.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics", meta=(ClampMin="0.0", ClampMax="2.0", UIMin="0.0", UIMax="1.0"))
    float RefractionStrength = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics")
    FLinearColor ShallowColor = FLinearColor(0.12f, 0.55f, 0.75f, 0.55f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Optics")
    FLinearColor DeepColor = FLinearColor(0.01f, 0.06f, 0.12f, 0.85f);
};

USTRUCT(BlueprintType)
struct FPristineWaterVolume
{
    GENERATED_BODY()

    // Artistic depth/volume scale. In material this should drive depth fade and absorption.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Volume", meta=(ClampMin="0.0", ClampMax="10000.0", UIMin="0.0", UIMax="2000.0"))
    float VolumeDepth = 350.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Volume", meta=(ClampMin="0.0", ClampMax="20.0", UIMin="0.0", UIMax="5.0"))
    float Absorption = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Volume", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Clarity = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Volume", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float FoamAmount = 0.15f;

    // Relative liquid density in g/cm3. Fresh water is about 1.0; many oils are 0.8-0.95.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Volume", meta=(ClampMin="0.0", ClampMax="3.0", UIMin="0.0", UIMax="2.0"))
    float Density = 1.0f;
};

USTRUCT(BlueprintType)
struct FPristineWaterMotion
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion", meta=(ClampMin="0.0", ClampMax="500.0", UIMin="0.0", UIMax="100.0"))
    float WaveAmplitude = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion", meta=(ClampMin="0.0", ClampMax="20.0", UIMin="0.0", UIMax="5.0"))
    float WaveFrequency = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion", meta=(ClampMin="0.0", ClampMax="20.0", UIMin="0.0", UIMax="5.0"))
    float WaveSpeed = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float RippleStrength = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion")
    FVector2D FlowDirection = FVector2D(1.0f, 0.0f);

    // World-space repetition length for the broad normal layer; finer bands follow it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Motion", meta=(ClampMin="10", ClampMax="10000", Units="cm"))
    float SurfaceDetailScaleCm = 500;
};
