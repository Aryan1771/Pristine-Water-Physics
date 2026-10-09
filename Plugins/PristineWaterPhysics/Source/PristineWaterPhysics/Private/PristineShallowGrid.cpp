#include "PristineShallowGrid.h"

namespace {constexpr double Gravity=9.81,Dry=1.e-8;}
void FPristineShallowGrid::Initialize(int32 Resolution,double WidthM,double LengthM)
{
    N=FMath::Clamp(Resolution,8,128); DX=FMath::Max(0.001,WidthM/N); DY=FMath::Max(0.001,LengthM/N);
    Bed.Init(0,N*N); Depth.Init(0,N*N); Active.Init(1,N*N); MX.Init(0,N*N); MY.Init(0,N*N);
    FacesX.SetNum(N*(N+1)); FacesY.SetNum(N*(N+1));
}
void FPristineShallowGrid::Fill(double LevelM)
{
    for(int I=0;I<Depth.Num();++I) {Depth[I]=Active[I]?FMath::Max(0.0,LevelM-Bed[I]):0; MX[I]=MY[I]=0;}
}
double FPristineShallowGrid::Volume() const
{double V=0; for(double H:Depth) V+=H*DX*DY; return V;}
FVector2D FPristineShallowGrid::Velocity(int32 I) const
{return Depth.IsValidIndex(I)&&Depth[I]>Dry?FVector2D(MX[I],MY[I])/Depth[I]:FVector2D::ZeroVector;}
double FPristineShallowGrid::StableStep() const
{
    double Rate=0;
    for(int I=0;I<Depth.Num();++I) if(Active[I]&&Depth[I]>Dry)
    {
        const auto V=Velocity(I); const double C=FMath::Sqrt(Gravity*Depth[I]);
        // Two-dimensional CFL, including dry-front wave speeds up to u +/- 2c.
        Rate=FMath::Max(Rate,(FMath::Abs(V.X)+2*C)/DX+(FMath::Abs(V.Y)+2*C)/DY);
    }
    return FMath::Min(1.0/60,0.4/FMath::Max(Rate,1.e-9));
}
FPristineShallowGrid::FFace FPristineShallowGrid::Face(int A,int B,bool YAxis) const
{
    FFace Result;
    const bool LiveA=Active.IsValidIndex(A)&&Active[A],LiveB=Active.IsValidIndex(B)&&Active[B];
    if(!LiveA&&!LiveB) return Result;
    const int IA=LiveA?A:B,IB=LiveB?B:A;
    const double HA=Depth[IA],HB=Depth[IB];
    auto VA=Velocity(IA),VB=Velocity(IB);
    // Mirror normal velocity at closed domain/inactive boundaries; preserve tangential slip.
    if(!LiveA) {if(YAxis) VA.Y=-VA.Y; else VA.X=-VA.X;}
    if(!LiveB) {if(YAxis) VB.Y=-VB.Y; else VB.X=-VB.X;}
    const double Top=FMath::Max(Bed[IA],Bed[IB]);
    const double HL=FMath::Max(0.0,Bed[IA]+HA-Top),HR=FMath::Max(0.0,Bed[IB]+HB-Top);
    const double UL=YAxis?VA.Y:VA.X,UR=YAxis?VB.Y:VB.X;
    const double TL=YAxis?VA.X:VA.Y,TR=YAxis?VB.X:VB.Y;
    const FVector L(HL,HL*UL,HL*TL),R(HR,HR*UR,HR*TR);
    const FVector FL(L.Y,L.Y*UL+0.5*Gravity*HL*HL,L.Y*TL);
    const FVector FR(R.Y,R.Y*UR+0.5*Gravity*HR*HR,R.Y*TR);
    const double CL=FMath::Sqrt(Gravity*HL),CR=FMath::Sqrt(Gravity*HR);
    double SL=FMath::Min(UL-CL,UR-CR),SR=FMath::Max(UL+CL,UR+CR);
    if(HL<=Dry) {SL=UR-2*CR; SR=UR+CR;}
    if(HR<=Dry) {SL=UL-CL; SR=UL+2*CL;}
    FVector F=FVector::ZeroVector;
    if(HL>0||HR>0)
        F=SL>=0?FL:SR<=0?FR:(FL*SR-FR*SL+(R-L)*(SL*SR))/FMath::Max(SR-SL,1.e-12);
    // Audusse et al. hydrostatic pressure corrections preserve a lake at rest.
    Result.LeftPressure=0.5*Gravity*(HA*HA-HL*HL);
    Result.RightPressure=0.5*Gravity*(HB*HB-HR*HR);
    Result.Flux=YAxis?FVector(F.X,F.Z,F.Y):F;
    return Result;
}
void FPristineShallowGrid::Step(double Dt,double Drag)
{
    if(N==0||!FMath::IsFinite(Dt)||Dt<=0) return;
    for(int Y=0;Y<N;++Y) for(int X=0;X<=N;++X)
        FacesX[Y*(N+1)+X]=Face(X>0?Y*N+X-1:INDEX_NONE,X<N?Y*N+X:INDEX_NONE,false);
    for(int Y=0;Y<=N;++Y) for(int X=0;X<N;++X)
        FacesY[Y*N+X]=Face(Y>0?(Y-1)*N+X:INDEX_NONE,Y<N?Y*N+X:INDEX_NONE,true);
    DrainScale.Init(1,N*N);
    for(int Y=0;Y<N;++Y) for(int X=0;X<N;++X)
    {
        const double Out=(FMath::Max(0.0,FacesX[Y*(N+1)+X+1].Flux.X)+FMath::Max(0.0,-FacesX[Y*(N+1)+X].Flux.X))/DX
                        +(FMath::Max(0.0,FacesY[(Y+1)*N+X].Flux.X)+FMath::Max(0.0,-FacesY[Y*N+X].Flux.X))/DY;
        if(Out>0) DrainScale[Y*N+X]=FMath::Min(1.0,Depth[Y*N+X]/(Dt*Out));
    }
    // Shared face flux is scaled once by its donor, keeping exchanges conservative.
    for(int Y=0;Y<N;++Y) for(int X=1;X<N;++X)
    {auto& F=FacesX[Y*(N+1)+X].Flux; F*=DrainScale[Y*N+X-(F.X>0?1:0)];}
    for(int Y=1;Y<N;++Y) for(int X=0;X<N;++X)
    {auto& F=FacesY[Y*N+X].Flux; F*=DrainScale[(Y-(F.X>0?1:0))*N+X];}
    const double Damping=FMath::Exp(-FMath::Max(0.0,Drag)*Dt);
    for(int Y=0;Y<N;++Y) for(int X=0;X<N;++X)
    {
        const int I=Y*N+X; if(!Active[I]) continue;
        const auto& L=FacesX[Y*(N+1)+X]; const auto& R=FacesX[Y*(N+1)+X+1];
        const auto& B=FacesY[Y*N+X]; const auto& T=FacesY[(Y+1)*N+X];
        Depth[I]=FMath::Max(0.0,Depth[I]-Dt*((R.Flux.X-L.Flux.X)/DX+(T.Flux.X-B.Flux.X)/DY));
        MX[I]=(MX[I]-Dt*((R.Flux.Y+R.LeftPressure-L.Flux.Y-L.RightPressure)/DX+(T.Flux.Y-B.Flux.Y)/DY))*Damping;
        MY[I]=(MY[I]-Dt*((R.Flux.Z-L.Flux.Z)/DX+(T.Flux.Z+T.LeftPressure-B.Flux.Z-B.RightPressure)/DY))*Damping;
        if(Depth[I]<=Dry) MX[I]=MY[I]=0;
    }
}
void FPristineShallowGrid::Impulse(int X,int Y,FVector2D V)
{
    if(X<0||Y<0||X>=N||Y>=N||!Active[Y*N+X]||V.ContainsNaN()) return;
    const int I=Y*N+X; MX[I]+=V.X*Depth[I]; MY[I]+=V.Y*Depth[I];
}
double FPristineShallowGrid::RemoveVolume(int I,double V)
{
    if(!Depth.IsValidIndex(I)||!Active[I]||!FMath::IsFinite(V)||V<=0) return 0;
    const double Before=Depth[I],Removed=FMath::Min(V,Before*DX*DY);
    Depth[I]=FMath::Max(0.0,Before-Removed/(DX*DY));
    const double Ratio=Before>0?Depth[I]/Before:0; MX[I]*=Ratio; MY[I]*=Ratio;
    return Removed;
}
void FPristineShallowGrid::AddVolume(int I,double V,FVector2D Speed)
{
    if(!Depth.IsValidIndex(I)||!Active[I]||!FMath::IsFinite(V)||V<=0||Speed.ContainsNaN()) return;
    const double H=V/(DX*DY); Depth[I]+=H; MX[I]+=H*Speed.X; MY[I]+=H*Speed.Y;
}
