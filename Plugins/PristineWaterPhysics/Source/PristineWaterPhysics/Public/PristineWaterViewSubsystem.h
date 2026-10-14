#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PristineWaterViewSubsystem.generated.h"

class FSceneViewExtensionBase;

// Inform Unreal's water rendering passes about cameras inside our custom regions.
UCLASS()
class PRISTINEWATERPHYSICS_API UPristineWaterViewSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    float GetImmersionDepth(const FVector& Position) const;
private:
    TSharedPtr<FSceneViewExtensionBase,ESPMode::ThreadSafe> ViewExtension;
};
