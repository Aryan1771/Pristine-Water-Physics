#pragma once
#include "CoreMinimal.h"

// SI units. Staggered face discharge, hydrostatic pressure, donor-cell positivity limiter.
// Closed inactive cells and domain boundaries exchange no volume.
struct PRISTINEWATERPHYSICS_API FPristineShallowGrid
{
    int32 N = 0;
    double DX = 1, DY = 1;
    TArray<double> Bed, Depth, QX, QY, Scale;
    TArray<uint8> Active;
    void Initialize(int32 Resolution, double WidthM, double LengthM);
    void Fill(double LevelM);
    double Volume() const;
    double StableStep() const;
    void Step(double Dt, double Drag);
    void Impulse(int32 X, int32 Y, FVector2D VelocityMps);
};
