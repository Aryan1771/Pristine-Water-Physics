#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PristineWaterAssetsCommandlet.generated.h"

UCLASS()
class UPristineWaterAssetsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UPristineWaterAssetsCommandlet();
    virtual int32 Main(const FString& Params) override;
    static bool BuildAssets();
};
