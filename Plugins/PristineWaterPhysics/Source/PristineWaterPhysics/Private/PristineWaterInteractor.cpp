#include "PristineWaterInteractor.h"
#include "PristineWaterRegion.h"
#include "PristineWaterComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UPristineWaterInteractor::UPristineWaterInteractor(){PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PrePhysics;}
void UPristineWaterInteractor::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Dt,Type,Tick);
    if(!IsValid(Region)||Dt<=0) {bWasSubmerged=false; return;}
    if(!PhysicsBody&&GetOwner()) PhysicsBody=Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
    if(!IsValid(PhysicsBody)||!PhysicsBody->IsSimulatingPhysics()) return;
    const FVector P=PhysicsBody->GetComponentTransform().TransformPosition(LocalCenter);
    const FPristineRegionSample S=Region->SampleRegion(P); const double R=FMath::Max(0.1f,Radius);
    const double H=S.bWet?FMath::Clamp(double(S.DepthCm)+R,0.0,2*R):0;
    const FVector V=PhysicsBody->GetPhysicsLinearVelocityAtPoint(P);
    if(H>0)
    {
        const double Rho=FMath::Max(0.f,Region->Water->Volume.Density)*1000;
        const double VolumeM3=PI*H*H*(R-H/3)*1.e-6;
        const FVector Relative=(V-S.Velocity)*0.01;
        FVector Drag=-Relative*Relative.Size()*(0.5*Rho*FMath::Max(0.f,DragCoefficient)*PI*R*R*1.e-4*(H/(2*R))*100);
        // Do not reverse velocity in one explicit drag step.
        Drag=Drag.GetClampedToMaxSize(PhysicsBody->GetMass()*Relative.Size()*100/Dt);
        PhysicsBody->AddForceAtLocation(Drag+FVector(0,0,VolumeM3*Rho*FMath::Abs(GetWorld()->GetGravityZ())),P);
        if(!bWasSubmerged&&V.Size()>30) Region->AddSplash(P,V,R*3,FMath::Clamp(float(V.Size()/300),0.f,3.f));
    }
    bWasSubmerged=H>0;
}
