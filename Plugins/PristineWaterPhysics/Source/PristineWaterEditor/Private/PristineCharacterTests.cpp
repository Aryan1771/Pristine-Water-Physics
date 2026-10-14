#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PristineWaterCharacter.h"
#include "PristineWaterRegion.h"
#include "PristineWaterViewSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Editor.h"
#include "Engine/World.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineCharacterTest,"PristineWater.Interaction.CharacterTransitions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineCharacterTest::RunTest(const FString&)
{
    auto* W=GEditor->GetEditorWorldContext().World();if(!W) return false;
    auto* R=W->SpawnActor<APristineWaterRegion>();R->SetActorLocation(FVector(50000,50000,0));R->DefaultDepth=500;R->ResetWater();
    auto* C=W->SpawnActor<APristineWaterCharacter>();
    auto* PC=W->SpawnActor<APlayerController>();PC->Possess(C);C->RefreshWaterRegions();
    TestTrue(TEXT("Water does not block diving through its surface"),R->Surface->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
    const double Volume=R->GetWaterVolumeM3();
    const auto* WaterView=W->GetSubsystem<UPristineWaterViewSubsystem>();
    TestNotNull(TEXT("Water renderer world integration exists"),WaterView);
    if(WaterView)
    {
        TestEqual(TEXT("Renderer receives the submerged view depth"),WaterView->GetImmersionDepth(FVector(50000,50000,-120)),120.f);
        TestTrue(TEXT("Renderer leaves an above-water view outside"),WaterView->GetImmersionDepth(FVector(50000,50000,10))<0);
        TestTrue(TEXT("Renderer leaves below-bed views outside"),WaterView->GetImmersionDepth(FVector(50000,50000,-600))<0);
    }
    C->SetActorLocation(FVector(50000,50000,-120));C->Tick(1.f/60);
    TestTrue(TEXT("Deep water enables swimming"),C->bSwimming);
    TestTrue(TEXT("Camera below surface enables underwater optics"),C->bUnderwater);
    C->SetActorLocation(FVector(50000,50000,-300));
    C->GetCharacterMovement()->Velocity=FVector(150,0,0);C->Tick(.2f);R->AdvanceSimulation(.02f);
    TestEqual(TEXT("Deep divers do not eject droplets at the distant surface"),R->Splashes->GetInstanceCount(),0);
    C->GetCharacterMovement()->StopMovementImmediately();
    TestNotNull(TEXT("Underwater material resolves from the character asset reference"),C->UnderwaterEffect.LoadSynchronous());
    if(auto* Underwater=C->UnderwaterEffect.LoadSynchronous())
    {
        float Immersion=-1;
        TestTrue(TEXT("Underwater material exposes its blend control"),Underwater->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Immersion")),Immersion));
        TestEqual(TEXT("Underwater material defaults to identity for clean camera blending"),Immersion,0.f);
    }
    C->SetActorLocation(FVector(50000,50000,75));C->Tick(1.f/60);
    TestFalse(TEXT("Shallow wading exits swimming"),C->bSwimming);
    TestFalse(TEXT("Above-water camera disables underwater optics"),C->bUnderwater);
    C->SetActorLocation(FVector(60000,60000,200));C->Tick(1.f/60);
    TestTrue(TEXT("Leaving water restores gravity movement"),C->GetCharacterMovement()->MovementMode==MOVE_Falling);
    TestEqual(TEXT("Stationary entry/exit does not create water"),R->GetWaterVolumeM3(),Volume);
    // A capsule rests about 2 cm above the floor: its bottom is still in a 4 cm puddle.
    R->DefaultDepth=4;R->ResetWater();
    C->SetActorLocation(FVector(50000,50000,86.2));
    C->GetCharacterMovement()->Velocity=FVector(150,0,0);C->Tick(.2f);R->AdvanceSimulation(.04f);
    TestFalse(TEXT("A shallow puddle keeps walking movement"),C->bSwimming);
    TestTrue(TEXT("Walking through a 4 cm puddle creates a surface splash"),R->Splashes->GetInstanceCount()>0);
    PC->UnPossess();PC->Destroy();C->Destroy();R->Destroy();return true;
}
#endif
