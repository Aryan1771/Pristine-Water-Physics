#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PristineShallowGrid.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineDamReference,"PristineWater.Physics.RitterDamBreak",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineDamReference::RunTest(const FString&)
{
    auto Error=[&](int N)
    {
        FPristineShallowGrid G; G.Initialize(N,40,40);
        for(int Y=0;Y<N;++Y) for(int X=0;X<N;++X) G.Depth[Y*N+X]=X<N/2?1:0;
        const double V=G.Volume(); double Time=0;
        while(Time<1) {const double Dt=FMath::Min(G.StableStep(),1-Time); G.Step(Dt,0); Time+=Dt;}
        double E=0; const double C=FMath::Sqrt(9.81);
        for(int X=0;X<N;++X)
        {
            const double Position=(X+0.5)*G.DX-20;
            const double Exact=Position<-C?1:Position>2*C?0:FMath::Square(2*C-Position)/(9*9.81);
            E+=FMath::Abs(G.Depth[(N/2)*N+X]-Exact)*G.DX;
        }
        TestTrue(TEXT("Analytical dam break conserves volume"),FMath::Abs(G.Volume()-V)<1.e-7);
        bool Valid=true; for(int I=0;I<N*N;++I) Valid&=FMath::IsFinite(G.Depth[I])&&G.Depth[I]>=0&&FMath::IsFinite(G.MX[I])&&FMath::IsFinite(G.MY[I]);
        TestTrue(TEXT("Depth and momentum stay finite through dry-front propagation"),Valid);
        return E;
    };
    const double Coarse=Error(64),Fine=Error(128);
    AddInfo(FString::Printf(TEXT("Ritter depth L1 integral: 64 cells %.9f m2, 128 cells %.9f m2"),Coarse,Fine));
    TestTrue(TEXT("Refinement approaches analytical depth solution"),Fine<Coarse*0.9);
    TestTrue(TEXT("Fine-grid normalized mean depth error below 2 percent"),Fine/40<0.02);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineMomentumTest,"PristineWater.Physics.MomentumTransport",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineMomentumTest::RunTest(const FString&)
{
    FPristineShallowGrid G; G.Initialize(128,32,32); G.Fill(1);
    for(int Y=0;Y<G.N;++Y) for(int X=0;X<G.N;++X)
    {
        const int I=Y*G.N+X; const double Position=(X+0.5)*G.DX;
        G.MX[I]=1; G.MY[I]=0.1*FMath::Exp(-FMath::Square(Position-12));
    }
    auto Center=[&]()
    {
        double Sum=0,Moment=0;
        for(int X=24;X<88;++X) {const double Q=G.MY[(G.N/2)*G.N+X]; Sum+=Q; Moment+=Q*(X+0.5)*G.DX;}
        return Moment/Sum;
    };
    const double Before=Center(); double Time=0;
    while(Time<0.5) {const double Dt=FMath::Min(G.StableStep(),0.5-Time); G.Step(Dt,0); Time+=Dt;}
    const double Distance=Center()-Before;
    AddInfo(FString::Printf(TEXT("Transverse momentum transported %.9f m; expected 0.5 m"),Distance));
    TestTrue(TEXT("Flow transports transverse momentum at its advective speed"),FMath::Abs(Distance-0.5)<0.03);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPristineVolumeExchange,"PristineWater.Physics.VolumeWeightedMomentum",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPristineVolumeExchange::RunTest(const FString&)
{
    FPristineShallowGrid G; G.Initialize(8,8,8); G.Fill(1); const int I=27;
    G.Impulse(3,3,FVector2D(2,1));
    const double Taken=G.RemoveVolume(I,0.01);
    TestTrue(TEXT("Removing liquid carries proportional momentum"),FMath::Abs(G.MX[I]-1.98)<1.e-12&&FMath::Abs(G.MY[I]-0.99)<1.e-12);
    G.AddVolume(I,Taken,FVector2D(10,0));
    TestTrue(TEXT("Landing momentum is weighted by incoming volume"),FMath::Abs(G.MX[I]-2.08)<1.e-12&&FMath::Abs(G.MY[I]-0.99)<1.e-12);
    TestTrue(TEXT("Volume exchange restores initial depth"),FMath::Abs(G.Depth[I]-1)<1.e-12);
    const double Removed=G.RemoveVolume(I,100);
    TestEqual(TEXT("Removal cannot exceed available water"),Removed,1.0);
    TestEqual(TEXT("Dry cell has zero momentum"),G.MX[I],0.0);
    return true;
}
#endif
