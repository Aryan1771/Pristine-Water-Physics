#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PristineSceneTools.generated.h"

class UMaterialInterface;
class UAnimSequence;

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
    // Generates a simple looping treading preview from a licensed character's idle pose.
    UFUNCTION(BlueprintCallable, Category="Pristine|Scene")
    static UAnimSequence* CreateTreadingAnimation(UAnimSequence* Idle, const FString& PackagePath);
};
