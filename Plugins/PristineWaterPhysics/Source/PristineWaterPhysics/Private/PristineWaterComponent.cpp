#include "PristineWaterComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

UPristineWaterComponent::UPristineWaterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPristineWaterComponent::OnRegister()
{
    Super::OnRegister();

    if (!TargetWaterMesh && GetOwner())
    {
        TargetWaterMesh = GetOwner()->FindComponentByClass<UMeshComponent>();
    }

    ApplyWaterSettings();
}

void UPristineWaterComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyWaterSettings();
}

void UPristineWaterComponent::BindToMesh(UMeshComponent* Mesh, int32 Slot)
{
    TargetWaterMesh = Mesh;
    MaterialSlot = Slot;
    ApplyWaterSettings();
}

void UPristineWaterComponent::ApplyWaterSettings()
{
    if (IsTemplate() || !IsValid(TargetWaterMesh) || MaterialSlot < 0 || MaterialSlot >= TargetWaterMesh->GetNumMaterials()) return;

    UMaterialInterface* Current = TargetWaterMesh->GetMaterial(MaterialSlot);
    if (Current != DynamicMaterial) DynamicMaterial = nullptr;

    if (bAssignLiquidMaterialOnApply)
    {
        if (UMaterialInterface* LiquidMaterial = ResolveLiquidMaterial())
        {
            if (AppliedParent != LiquidMaterial || !DynamicMaterial)
            {
                TargetWaterMesh->SetMaterial(MaterialSlot, LiquidMaterial);
                DynamicMaterial = nullptr;
                AppliedParent = LiquidMaterial;
            }
        }
    }

    if (!DynamicMaterial)
    {
        DynamicMaterial = TargetWaterMesh->CreateDynamicMaterialInstance(MaterialSlot);
    }

    if (!DynamicMaterial) return;

    SetScalar(TEXT("RefractiveIndex"), Optics.RefractiveIndex);
    SetScalar(TEXT("Reflectivity"), Optics.Reflectivity);
    SetScalar(TEXT("FresnelPower"), Optics.FresnelPower);
    SetScalar(TEXT("FresnelStrength"), Optics.FresnelStrength);
    SetScalar(TEXT("TotalInternalReflection"), Optics.TotalInternalReflection);
    SetScalar(TEXT("RefractionStrength"), Optics.RefractionStrength);
    SetVector(TEXT("ShallowColor"), Optics.ShallowColor);
    SetVector(TEXT("DeepColor"), Optics.DeepColor);

    SetScalar(TEXT("VolumeDepth"), Volume.VolumeDepth);
    SetScalar(TEXT("Absorption"), Volume.Absorption);
    SetScalar(TEXT("Clarity"), Volume.Clarity);
    SetScalar(TEXT("FoamAmount"), Volume.FoamAmount);
    SetScalar(TEXT("Density"), Volume.Density);
    SetScalar(TEXT("LiquidType"), static_cast<float>(LiquidType));

    SetScalar(TEXT("WaveAmplitude"), Motion.WaveAmplitude);
    SetScalar(TEXT("WaveFrequency"), Motion.WaveFrequency);
    SetScalar(TEXT("WaveSpeed"), Motion.WaveSpeed);
    SetScalar(TEXT("RippleStrength"), Motion.RippleStrength);
    SetScalar(TEXT("FlowDirectionX"), Motion.FlowDirection.X);
    SetScalar(TEXT("FlowDirectionY"), Motion.FlowDirection.Y);
    SetVector(TEXT("FlowDirection"), FLinearColor(Motion.FlowDirection.X, Motion.FlowDirection.Y, 0.0f, 0.0f));
}

void UPristineWaterComponent::SetLiquidType(EPristineLiquidType NewLiquidType, bool bApplyPreset)
{
    LiquidType = NewLiquidType;

    if (bApplyPreset)
    {
        ApplyLiquidPreset(NewLiquidType);
    }

    ApplyWaterSettings();
}

void UPristineWaterComponent::SetReflectivity(float Value)
{
    Optics.Reflectivity = FMath::Clamp(Value, 0.0f, 1.0f);
    SetScalar(TEXT("Reflectivity"), Optics.Reflectivity);
}

void UPristineWaterComponent::SetRefractiveIndex(float Value)
{
    Optics.RefractiveIndex = FMath::Clamp(Value, 1.0f, 2.5f);
    SetScalar(TEXT("RefractiveIndex"), Optics.RefractiveIndex);
}

void UPristineWaterComponent::SetTotalInternalReflection(float Value)
{
    Optics.TotalInternalReflection = FMath::Clamp(Value, 0.0f, 2.0f);
    SetScalar(TEXT("TotalInternalReflection"), Optics.TotalInternalReflection);
}

void UPristineWaterComponent::SetFresnelStrength(float Value)
{
    Optics.FresnelStrength = FMath::Clamp(Value, 0.0f, 2.0f);
    SetScalar(TEXT("FresnelStrength"), Optics.FresnelStrength);
}

void UPristineWaterComponent::SetRefractionStrength(float Value)
{
    Optics.RefractionStrength = FMath::Clamp(Value, 0.0f, 2.0f);
    SetScalar(TEXT("RefractionStrength"), Optics.RefractionStrength);
}

void UPristineWaterComponent::SetVolumeDepth(float Value)
{
    Volume.VolumeDepth = FMath::Max(0.0f, Value);
    SetScalar(TEXT("VolumeDepth"), Volume.VolumeDepth);
}

void UPristineWaterComponent::SetAbsorption(float Value)
{
    Volume.Absorption = FMath::Clamp(Value, 0.0f, 20.0f);
    SetScalar(TEXT("Absorption"), Volume.Absorption);
}

void UPristineWaterComponent::SetClarity(float Value)
{
    Volume.Clarity = FMath::Clamp(Value, 0.0f, 1.0f);
    SetScalar(TEXT("Clarity"), Volume.Clarity);
}

void UPristineWaterComponent::SetFoamAmount(float Value)
{
    Volume.FoamAmount = FMath::Clamp(Value, 0.0f, 1.0f);
    SetScalar(TEXT("FoamAmount"), Volume.FoamAmount);
}

void UPristineWaterComponent::SetWaveAmplitude(float Value)
{
    Motion.WaveAmplitude = FMath::Clamp(Value, 0.0f, 500.0f);
    SetScalar(TEXT("WaveAmplitude"), Motion.WaveAmplitude);
}

void UPristineWaterComponent::SetWaveFrequency(float Value)
{
    Motion.WaveFrequency = FMath::Clamp(Value, 0.0f, 20.0f);
    SetScalar(TEXT("WaveFrequency"), Motion.WaveFrequency);
}

void UPristineWaterComponent::SetWaveSpeed(float Value)
{
    Motion.WaveSpeed = FMath::Clamp(Value, 0.0f, 20.0f);
    SetScalar(TEXT("WaveSpeed"), Motion.WaveSpeed);
}

void UPristineWaterComponent::SetRippleStrength(float Value)
{
    Motion.RippleStrength = FMath::Clamp(Value, 0.0f, 1.0f);
    SetScalar(TEXT("RippleStrength"), Motion.RippleStrength);
}

void UPristineWaterComponent::SetFlowDirection(FVector2D Value)
{
    Motion.FlowDirection = Value.GetSafeNormal();
    SetScalar(TEXT("FlowDirectionX"), Motion.FlowDirection.X);
    SetScalar(TEXT("FlowDirectionY"), Motion.FlowDirection.Y);
    SetVector(TEXT("FlowDirection"), FLinearColor(Motion.FlowDirection.X, Motion.FlowDirection.Y, 0.0f, 0.0f));
}

void UPristineWaterComponent::SetDensity(float Value)
{
    Volume.Density = FMath::Clamp(Value, 0.0f, 3.0f);
    SetScalar(TEXT("Density"), Volume.Density);
}

void UPristineWaterComponent::SetLiquidColors(FLinearColor Shallow, FLinearColor Deep)
{
    Optics.ShallowColor = Shallow;
    Optics.DeepColor = Deep;
    SetVector(TEXT("ShallowColor"), Optics.ShallowColor);
    SetVector(TEXT("DeepColor"), Optics.DeepColor);
}

#if WITH_EDITOR
void UPristineWaterComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName ChangedPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(UPristineWaterComponent, LiquidType))
    {
        ApplyLiquidPreset(LiquidType);
    }

    ApplyWaterSettings();
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UPristineWaterComponent::ApplyLiquidPreset(EPristineLiquidType NewLiquidType)
{
    switch (NewLiquidType)
    {
    case EPristineLiquidType::Ocean:
        Optics.RefractiveIndex = 1.333f;
        Optics.Reflectivity = 0.72f;
        Optics.FresnelPower = 5.5f;
        Optics.TotalInternalReflection = 0.45f;
        Optics.RefractionStrength = 0.16f;
        Optics.ShallowColor = FLinearColor(0.05f, 0.42f, 0.62f, 0.65f);
        Optics.DeepColor = FLinearColor(0.002f, 0.02f, 0.06f, 0.95f);
        Volume.VolumeDepth = 1200.0f;
        Volume.Absorption = 2.25f;
        Volume.Clarity = 0.62f;
        Volume.FoamAmount = 0.28f;
        Volume.Density = 1.025f;
        Motion.WaveAmplitude = 24.0f;
        Motion.WaveFrequency = 0.85f;
        Motion.WaveSpeed = 1.1f;
        Motion.RippleStrength = 0.35f;
        break;
    case EPristineLiquidType::Oil:
        Optics.RefractiveIndex = 1.47f;
        Optics.Reflectivity = 0.82f;
        Optics.FresnelPower = 4.0f;
        Optics.TotalInternalReflection = 0.75f;
        Optics.RefractionStrength = 0.09f;
        Optics.ShallowColor = FLinearColor(0.70f, 0.54f, 0.18f, 0.82f);
        Optics.DeepColor = FLinearColor(0.12f, 0.075f, 0.025f, 0.98f);
        Volume.VolumeDepth = 120.0f;
        Volume.Absorption = 5.5f;
        Volume.Clarity = 0.38f;
        Volume.FoamAmount = 0.03f;
        Volume.Density = 0.9f;
        Motion.WaveAmplitude = 2.0f;
        Motion.WaveFrequency = 0.45f;
        Motion.WaveSpeed = 0.18f;
        Motion.RippleStrength = 0.08f;
        break;
    case EPristineLiquidType::Custom:
        break;
    case EPristineLiquidType::Water:
    default:
        Optics.RefractiveIndex = 1.333f;
        Optics.Reflectivity = 0.65f;
        Optics.FresnelPower = 5.0f;
        Optics.TotalInternalReflection = 0.35f;
        Optics.RefractionStrength = 0.18f;
        Optics.ShallowColor = FLinearColor(0.12f, 0.55f, 0.75f, 0.55f);
        Optics.DeepColor = FLinearColor(0.01f, 0.06f, 0.12f, 0.85f);
        Volume.VolumeDepth = 350.0f;
        Volume.Absorption = 1.15f;
        Volume.Clarity = 0.75f;
        Volume.FoamAmount = 0.15f;
        Volume.Density = 1.0f;
        Motion.WaveAmplitude = 8.0f;
        Motion.WaveFrequency = 1.2f;
        Motion.WaveSpeed = 0.7f;
        Motion.RippleStrength = 0.25f;
        break;
    }
}

UMaterialInterface* UPristineWaterComponent::ResolveLiquidMaterial() const
{
    const TSoftObjectPtr<UMaterialInterface>* MaterialAsset = nullptr;

    switch (LiquidType)
    {
    case EPristineLiquidType::Ocean:
        MaterialAsset = &OceanMaterial;
        break;
    case EPristineLiquidType::Oil:
        MaterialAsset = &OilMaterial;
        break;
    case EPristineLiquidType::Custom:
        MaterialAsset = &CustomLiquidMaterial;
        break;
    case EPristineLiquidType::Water:
    default:
        MaterialAsset = &WaterMaterial;
        break;
    }

    return MaterialAsset && !MaterialAsset->IsNull() ? MaterialAsset->LoadSynchronous() : nullptr;
}

void UPristineWaterComponent::SetScalar(FName Name, float Value)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetScalarParameterValue(Name, Value);
    }
}

void UPristineWaterComponent::SetVector(FName Name, const FLinearColor& Value)
{
    if (DynamicMaterial)
    {
        DynamicMaterial->SetVectorParameterValue(Name, Value);
    }
}
