#include "PristineSceneTools.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"

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

UAnimSequence* UPristineSceneTools::CreateTreadingAnimation(UAnimSequence* Idle,const FString& PackagePath)
{
    if(!Idle||!Idle->GetSkeleton()||!FPackageName::IsValidLongPackageName(PackagePath)||!PackagePath.StartsWith(TEXT("/Game/"))) return nullptr;
    if(auto* Existing=LoadObject<UAnimSequence>(nullptr,*PackagePath)) return Existing;
    const IAnimationDataModel* Model=Idle->GetDataModel();
    if(!Model) return nullptr;
    TArray<FName> Bones;Model->GetBoneTrackNames(Bones);
    if(Bones.IsEmpty()) return nullptr;
    auto* Animation=NewObject<UAnimSequence>(CreatePackage(*PackagePath),*FPackageName::GetLongPackageAssetName(PackagePath),RF_Public|RF_Standalone);
    Animation->SetSkeleton(Idle->GetSkeleton());
    auto& Controller=Animation->GetController();
    Controller.OpenBracket(FText::FromString(TEXT("Create water treading preview")),false);
    Controller.InitializeModel();Controller.SetFrameRate(FFrameRate(30,1),false);Controller.SetNumberOfFrames(60,false);
    for(FName Bone:Bones)
    {
        const FTransform Base=Model->GetBoneTrackTransform(Bone,FFrameNumber(0));
        TArray<FVector3f> Positions,Scales;TArray<FQuat4f> Rotations;
        const FString Name=Bone.ToString();const float Side=Name.EndsWith(TEXT("_l"))?1.f:-1.f;
        for(int32 Frame=0;Frame<=60;++Frame)
        {
            const float Phase=2*PI*Frame/60.f;
            FQuat Offset=FQuat::Identity;
            if(Name==TEXT("upperarm_l")||Name==TEXT("upperarm_r")) Offset=FRotator(0,Side*(12+12*FMath::Sin(Phase)),Side*12).Quaternion();
            else if(Name==TEXT("lowerarm_l")||Name==TEXT("lowerarm_r")) Offset=FRotator(0,Side*(20+15*FMath::Cos(Phase)),0).Quaternion();
            else if(Name==TEXT("thigh_l")||Name==TEXT("thigh_r")) Offset=FRotator(12*FMath::Sin(Phase+Side*PI/2),0,0).Quaternion();
            else if(Name==TEXT("calf_l")||Name==TEXT("calf_r")) Offset=FRotator(10+10*FMath::Sin(Phase+Side*PI/2),0,0).Quaternion();
            Positions.Add(FVector3f(Base.GetTranslation()));Scales.Add(FVector3f(Base.GetScale3D()));Rotations.Add(FQuat4f((Base.GetRotation()*Offset).GetNormalized()));
        }
        Controller.AddBoneCurve(Bone,false);Controller.SetBoneTrackKeys(Bone,Positions,Rotations,Scales,false);
    }
    Controller.NotifyPopulated();Controller.CloseBracket(false);
    Animation->MarkPackageDirty();FAssetRegistryModule::AssetCreated(Animation);return Animation;
}
