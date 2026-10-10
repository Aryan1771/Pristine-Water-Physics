#include "PristineSceneTools.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

AActor* UPristineSceneTools::CreateMeshActor(UObject* WorldContext, const FString& Label,
    const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
    UMaterialInterface* Material, bool Collision)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
    if (!World || World->IsGameWorld() || Vertices.IsEmpty() || Triangles.Num()%3 != 0) return nullptr;
    for (int32 Index : Triangles) if (!Vertices.IsValidIndex(Index)) return nullptr;
    for (const FVector& V : Vertices) if (V.ContainsNaN()) return nullptr;
    TArray<FVector> Normals;
    Normals.Init(FVector::ZeroVector, Vertices.Num());
    TArray<FVector2D> UVs;
    UVs.Reserve(Vertices.Num());
    for (const FVector& V : Vertices) UVs.Add(FVector2D(V.X,V.Y)/100.0);
    for (int32 I=0; I<Triangles.Num(); I+=3)
    {
        const int32 A=Triangles[I], B=Triangles[I+1], C=Triangles[I+2];
        const FVector N=FVector::CrossProduct(Vertices[C]-Vertices[A],Vertices[B]-Vertices[A]);
        Normals[A]+=N; Normals[B]+=N; Normals[C]+=N;
    }
    for (FVector& N : Normals) N=N.GetSafeNormal();
    AActor* Actor=World->SpawnActor<AActor>();
    if (!Actor) return nullptr;
    Actor->SetFlags(RF_Transactional);
    Actor->SetActorLabel(Label);
    auto* Mesh=NewObject<UProceduralMeshComponent>(Actor, NAME_None, RF_Transactional);
    Actor->AddInstanceComponent(Mesh);
    Actor->SetRootComponent(Mesh);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->bUseAsyncCooking=false;
    Mesh->bUseComplexAsSimpleCollision=true;
    Mesh->SetCollisionObjectType(ECC_WorldStatic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Mesh->RegisterComponent();
    Mesh->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UVs,TArray<FLinearColor>(),TArray<FProcMeshTangent>(),Collision);
    Mesh->SetMaterial(0,Material);
    return Actor;
}
