#include "PristineShallowGrid.h"

void FPristineShallowGrid::Initialize(int32 Resolution, double WidthM, double LengthM)
{
    N=FMath::Clamp(Resolution,8,128); DX=FMath::Max(0.001,WidthM/N); DY=FMath::Max(0.001,LengthM/N);
    Bed.Init(0,N*N); Depth.Init(0,N*N); Active.Init(1,N*N);
    QX.Init(0,(N+1)*N); QY.Init(0,N*(N+1));
}
void FPristineShallowGrid::Fill(double LevelM)
{
    for(int32 I=0;I<Depth.Num();++I) Depth[I]=Active[I]?FMath::Max(0.0,LevelM-Bed[I]):0;
    for(double& Q:QX) Q=0;
    for(double& Q:QY) Q=0;
}
double FPristineShallowGrid::Volume() const
{
    double V=0; for(double H:Depth) V+=H*DX*DY; return V;
}
double FPristineShallowGrid::StableStep() const
{
    double Speed=0.01;
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        const int32 I=Y*N+X; const double H=Depth[I]; if(H<1.e-6) continue;
        const double U=(FMath::Abs(QX[Y*(N+1)+X])+FMath::Abs(QX[Y*(N+1)+X+1]))/(2*H);
        const double V=(FMath::Abs(QY[Y*N+X])+FMath::Abs(QY[(Y+1)*N+X]))/(2*H);
        Speed=FMath::Max(Speed,U+V+2*FMath::Sqrt(9.81*H));
    }
    return FMath::Min(1.0/60,0.4*FMath::Min(DX,DY)/Speed);
}
void FPristineShallowGrid::Step(double Dt, double Drag)
{
    if(N==0 || Dt<=0) return;
    const double Damping=FMath::Exp(-FMath::Max(0.0,Drag)*Dt);
    auto Pressure=[&](int32 A,int32 B,double& Q,double Spacing)
    {
        if(!Active[A] || !Active[B]) {Q=0; return;}
        const double EA=Bed[A]+Depth[A], EB=Bed[B]+Depth[B];
        const double H=FMath::Max(0.0,FMath::Max(EA,EB)-FMath::Max(Bed[A],Bed[B]));
        Q=H>1.e-6 ? (Q-9.81*H*(EB-EA)*Dt/Spacing)*Damping : 0;
    };
    for(int32 Y=0;Y<N;++Y) for(int32 X=1;X<N;++X) Pressure(Y*N+X-1,Y*N+X,QX[Y*(N+1)+X],DX);
    for(int32 Y=1;Y<N;++Y) for(int32 X=0;X<N;++X) Pressure((Y-1)*N+X,Y*N+X,QY[Y*N+X],DY);
    Scale.Init(1,N*N);
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        const double Out=(FMath::Max(0.0,QX[Y*(N+1)+X+1])+FMath::Max(0.0,-QX[Y*(N+1)+X]))/DX
                        +(FMath::Max(0.0,QY[(Y+1)*N+X])+FMath::Max(0.0,-QY[Y*N+X]))/DY;
        if(Out>0) Scale[Y*N+X]=FMath::Min(1.0,Depth[Y*N+X]/(Dt*Out));
    }
    for(int32 Y=0;Y<N;++Y) for(int32 X=1;X<N;++X)
    {double& Q=QX[Y*(N+1)+X]; Q*=Scale[Y*N+X-(Q>0?1:0)];}
    for(int32 Y=1;Y<N;++Y) for(int32 X=0;X<N;++X)
    {double& Q=QY[Y*N+X]; Q*=Scale[(Y-(Q>0?1:0))*N+X];}
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        double& H=Depth[Y*N+X];
        H=FMath::Max(0.0,H-Dt*((QX[Y*(N+1)+X+1]-QX[Y*(N+1)+X])/DX+(QY[(Y+1)*N+X]-QY[Y*N+X])/DY));
    }
}
void FPristineShallowGrid::Impulse(int32 X,int32 Y,FVector2D V)
{
    if(X<0||Y<0||X>=N||Y>=N||!Active[Y*N+X]) return;
    const double H=Depth[Y*N+X];
    if(X>0&&Active[Y*N+X-1]) QX[Y*(N+1)+X]+=V.X*H*0.5;
    if(X+1<N&&Active[Y*N+X+1]) QX[Y*(N+1)+X+1]+=V.X*H*0.5;
    if(Y>0&&Active[(Y-1)*N+X]) QY[Y*N+X]+=V.Y*H*0.5;
    if(Y+1<N&&Active[(Y+1)*N+X]) QY[(Y+1)*N+X]+=V.Y*H*0.5;
}
