#include "PristineWaterProperties.h"
// NIST Chemistry WebBook, water at 1 bar. Samples: 0.01 + 2*i degrees C.
// Density kg/m3 and dynamic viscosity microPa*s. See Tools/ReferenceWater.tsv.
namespace { const double Reference[][2]={
    {999.8431, 1791.135},
    {999.9427, 1672.958},
    {999.9742, 1566.790},
    {999.9420, 1471.023},
    {999.8498, 1384.313},
    {999.7010, 1305.525},
    {999.4986, 1233.701},
    {999.2454, 1168.024},
    {998.9438, 1107.793},
    {998.5962, 1052.409},
    {998.2045, 1001.351},
    {997.7706, 954.1696},
    {997.2963, 910.4715},
    {996.7831, 869.9140},
    {996.2325, 832.1959},
    {995.6458, 797.0521},
    {995.0243, 764.2483},
    {994.3692, 733.5767},
    {993.6814, 704.8526},
    {992.9620, 677.9115},
    {992.2119, 652.6059},
    {991.4321, 628.8038},
    {990.6232, 606.3866},
    {989.7861, 585.2471},
    {988.9215, 565.2889},
    {988.0299, 546.4243},
    {987.1122, 528.5739},
    {986.1688, 511.6657},
    {985.2003, 495.6339},
    {984.2073, 480.4184},
    {983.1901, 465.9643},
    {982.1493, 452.2214},
    {981.0854, 439.1435},
    {979.9986, 426.6883},
    {978.8895, 414.8166},
    {977.7583, 403.4925},
    {976.6055, 392.6828},
    {975.4313, 382.3567},
    {974.2361, 372.4859},
    {973.0201, 363.0438},
    {971.7836, 354.0061}
}; }
FPristineFreshWaterProperties UPristineWaterProperties::FreshWaterAtTemperature(float Celsius)
{
    FPristineFreshWaterProperties P;
    const double T=FMath::Clamp(FMath::IsFinite(Celsius)?double(Celsius):20.0,0.01,80.0);
    const double Index=(T-0.01)/2; const int32 I=FMath::Clamp(FMath::FloorToInt(Index),0,39);
    P.TemperatureC=T;
    P.DensityKgM3=FMath::Lerp(Reference[I][0],Reference[I+1][0],Index-I);
    P.DynamicViscosityPaS=FMath::Lerp(Reference[I][1],Reference[I+1][1],Index-I)*1.e-6;
    P.KinematicViscosityM2S=P.DynamicViscosityPaS/P.DensityKgM3;
    // IAPWS R1-76(2014), saturated pure-water surface tension, N/m.
    const double Tau=1-(T+273.15)/647.096;
    P.SurfaceTensionNm=0.2358*FMath::Pow(Tau,1.256)*(1-0.625*Tau);
    return P;
}
