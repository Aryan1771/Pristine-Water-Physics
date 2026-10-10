#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PristineSceneTools.generated.h"

class UMaterialInterface;

/** Editor authoring helpers. Generated actors contain ordinary serializable mesh components. */
UCLASS()
class PRISTINEWATEREDITOR_API UPristineSceneTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Pristine|Scene")
    static AActor* CreateMeshActor(UObject* WorldContext, const FString& Label,
        const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
        UMaterialInterface* Material, bool Collision);
};
