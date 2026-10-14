#include "PristineWaterCharacter.h"
#include "PristineWaterRegion.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PristineWaterComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

APristineWaterCharacter::APristineWaterCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(32,88);
    GetCharacterMovement()->MaxWalkSpeed=380;
    GetCharacterMovement()->BrakingDecelerationFlying=240;
    CameraArm=CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(GetRootComponent());
    CameraArm->SetRelativeLocation(FVector(0,0,65));
    CameraArm->TargetArmLength=0;
    CameraArm->bUsePawnControlRotation=true;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->PostProcessBlendWeight=0;
    Visual=CreateDefaultSubobject<UChildActorComponent>(TEXT("CharacterVisual"));
    Visual->SetupAttachment(GetRootComponent());
    Visual->SetRelativeLocation(FVector(0,0,-88));
    bUseControllerRotationYaw=true;
    UnderwaterEffect=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/PristineWaterPhysics/Materials/M_PristineUnderwater.M_PristineUnderwater")));
}
void APristineWaterCharacter::BeginPlay()
{
    Super::BeginPlay();
    RefreshWaterRegions();
    if(AActor* Actor=Visual->GetChildActor())
    {
        Actor->SetActorEnableCollision(false);
        TArray<USkeletalMeshComponent*> Meshes;Actor->GetComponents(Meshes);
        for(auto* BodyMesh:Meshes) if(BodyMesh->GetName().Equals(TEXT("Body"),ESearchCase::IgnoreCase)) {VisualBody=BodyMesh;break;}
    }
    if(auto* Material=UnderwaterEffect.LoadSynchronous())
    {
        UnderwaterMaterial=UMaterialInstanceDynamic::Create(Material,this);
        // Post-process volume blending interpolates material parameters, not final pixels.
        // The parent material is identity at Immersion=0 so exiting water fades cleanly.
        UnderwaterMaterial->SetScalarParameterValue(TEXT("Immersion"),1);
        Camera->AddOrUpdateBlendable(UnderwaterMaterial);
    }
}
void APristineWaterCharacter::RefreshWaterRegions()
{
    Regions.Reset();
    for(TActorIterator<APristineWaterRegion> It(GetWorld());It;++It) Regions.Add(*It);
}
void APristineWaterCharacter::Tick(float Dt)
{
    Super::Tick(Dt);
    APlayerController* PC=Cast<APlayerController>(Controller);
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five};
    for(int32 I=0;PC&&PC->IsLocalController()&&I<5&&I<TestLocations.Num();++I) if(PC->WasInputKeyJustPressed(Keys[I]))
    {
        SetActorLocation(TestLocations[I],false,nullptr,ETeleportType::TeleportPhysics);
        GetCharacterMovement()->StopMovementImmediately();
    }
    const float Half=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Feet=GetActorLocation()-FVector(0,0,Half);
    APristineWaterRegion* Region=nullptr;
    FPristineRegionSample Sample;
    for(auto& Candidate:Regions) if(Candidate.IsValid())
    {
        const auto S=Candidate->SampleRegion(Feet);
        if(S.bWet&&S.DepthCm>0&&(!Region||S.SurfaceZ>Sample.SurfaceZ)) {Region=Candidate.Get(); Sample=S;}
    }
    // Hysteresis prevents rapid mode changes at the walking/swimming boundary.
    const bool Swim=Region&&Sample.DepthCm>(bSwimming?Half*.95f:Half*1.25f);
    auto* Movement=GetCharacterMovement();
    if(Swim!=bSwimming)
    {
        bSwimming=Swim;
        StopJumping();
        // Flying movement supplies swept 3D movement without requiring a rectangular PhysicsVolume.
        Movement->SetMovementMode(Swim?MOVE_Flying:MOVE_Falling);
    }
    Movement->MaxFlySpeed=FMath::Clamp(SwimSpeed,50.f,600.f);
    Movement->MaxAcceleration=bSwimming?400.f:2048.f;
    UAnimSequence* Desired=bSwimming?(SwimAnimation?SwimAnimation.Get():IdleAnimation.Get()):(GetVelocity().Size2D()>20?WalkAnimation.Get():IdleAnimation.Get());
    if(VisualBody&&Desired&&Desired!=ActiveAnimation) {VisualBody->PlayAnimation(Desired,true);ActiveAnimation=Desired;}
    if(VisualBody&&Desired==WalkAnimation&&Desired) VisualBody->SetPlayRate(FMath::Clamp(GetVelocity().Size2D()/160.f,.4f,2.f));
    else if(VisualBody) VisualBody->SetPlayRate(1);
    if(PC&&PC->IsLocalController())
    {
    if(PC->WasInputKeyJustPressed(EKeys::V)) bThirdPerson=!bThirdPerson;
    CameraArm->TargetArmLength=bThirdPerson?260:0;
    Visual->SetVisibility(bThirdPerson,true);
    float MX=0,MY=0;PC->GetInputMouseDelta(MX,MY);
    AddControllerYawInput(MX);AddControllerPitchInput(-MY);
    const FRotator View=PC->GetControlRotation();
    const FRotator Direction(bSwimming?View.Pitch:0,View.Yaw,0);
    const float Forward=float(PC->IsInputKeyDown(EKeys::W))-float(PC->IsInputKeyDown(EKeys::S));
    const float Right=float(PC->IsInputKeyDown(EKeys::D))-float(PC->IsInputKeyDown(EKeys::A));
    AddMovementInput(Direction.Vector(),Forward);
    AddMovementInput(FRotationMatrix(FRotator(0,View.Yaw,0)).GetUnitAxis(EAxis::Y),Right);
    if(bSwimming)
    {
        Movement->Velocity+=Sample.Velocity*FMath::Min(Dt*.6f,.1f);
        const float Up=float(PC->IsInputKeyDown(EKeys::SpaceBar))-float(PC->IsInputKeyDown(EKeys::LeftControl));
        AddMovementInput(FVector::UpVector,Up);
        if(FMath::IsNearlyZero(Up)&&FMath::IsNearlyZero(Forward))
        {
            // Gentle buoyancy at rest; the eye settles above the free surface.
            const float TargetZ=Sample.SurfaceZ-Half*.55f;
            Movement->Velocity.Z=FMath::FInterpTo(Movement->Velocity.Z,FMath::Clamp((TargetZ-GetActorLocation().Z)*2,-40.f,40.f),Dt,2);
        }
    }
    else if(PC->WasInputKeyJustPressed(EKeys::SpaceBar)) Jump();
    if(PC->WasInputKeyJustReleased(EKeys::SpaceBar)) StopJumping();
    }
    WakeTimer+=Dt;
    // The depth-averaged solver has no submerged body wake. Do not eject surface
    // droplets when a diver is metres below the interface.
    const float SurfaceInfluence=FMath::Clamp((Half*2.5f-Sample.DepthCm)/Half,0.f,1.f);
    if(Region&&SurfaceInfluence>0&&WakeTimer>=.16f&&GetVelocity().SizeSquared()>900)
    {
        Region->AddSplash(FVector(Feet.X,Feet.Y,Sample.SurfaceZ),GetVelocity(),bSwimming?65:35,(bSwimming?.18f:.08f)*SurfaceInfluence);
        WakeTimer=0;
    }
    const FVector Eye=Camera->GetComponentLocation();
    const auto EyeSample=Region?Region->SampleRegion(Eye):FPristineRegionSample();
    bUnderwater=EyeSample.bWet&&EyeSample.DepthCm>0;
    if(UnderwaterMaterial&&Region)
    {
        UnderwaterMaterial->SetScalarParameterValue(TEXT("EyeDepth"),FMath::Max(0.f,EyeSample.DepthCm));
        UnderwaterMaterial->SetScalarParameterValue(TEXT("OpticalDistance"),FMath::Max(1.f,Region->Water->Volume.VolumeDepth));
        UnderwaterMaterial->SetScalarParameterValue(TEXT("Absorption"),Region->Water->Volume.Absorption);
        UnderwaterMaterial->SetScalarParameterValue(TEXT("Clarity"),Region->Water->Volume.Clarity);
        UnderwaterMaterial->SetVectorParameterValue(TEXT("Transmission"),Region->Water->Optics.DeepColor);
        UnderwaterMaterial->SetVectorParameterValue(TEXT("ScatterColor"),Region->Water->Optics.ShallowColor);
    }
    Camera->PostProcessBlendWeight=FMath::FInterpTo(Camera->PostProcessBlendWeight,bUnderwater?1.f:0.f,Dt,12);
    auto& PP=Camera->PostProcessSettings;
    PP.bOverride_VignetteIntensity=true; PP.VignetteIntensity=.28;
}
