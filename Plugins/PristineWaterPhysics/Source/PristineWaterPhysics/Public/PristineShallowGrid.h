#pragma once
#include "CoreMinimal.h"

// SI units. Cell-centered conserved depth and horizontal momentum per density.
// First-order HLL finite volumes with hydrostatic reconstruction and closed walls.
struct PRISTINEWATERPHYSICS_API FPristineShallowGrid
{
    int32 N=0;
    double DX=1,DY=1;
    TArray<double> Bed,Depth,MX,MY;
    TArray<uint8> Active;
    void Initialize(int32 Resolution,double WidthM,double LengthM);
    void Fill(double LevelM);
    double Volume() const;
    double StableStep() const;
    void Step(double Dt,double Drag);
    void Impulse(int32 X,int32 Y,FVector2D VelocityMps);
    FVector2D Velocity(int32 I) const;
    // Exchange a specified volume with its horizontal momentum. Return actual removed volume.
    double RemoveVolume(int32 I,double VolumeM3);
    void AddVolume(int32 I,double VolumeM3,FVector2D VelocityMps);
private:
    struct FFace {FVector Flux=FVector::ZeroVector; double LeftPressure=0,RightPressure=0;};
    TArray<FFace> FacesX,FacesY;
    TArray<double> DrainScale;
    FFace Face(int32 A,int32 B,bool YAxis) const;
};
