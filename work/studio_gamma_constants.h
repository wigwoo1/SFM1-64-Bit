#pragma once
struct StudioGammaConstants {
float gammaLimit;
double oneDouble;
float oneFloat;
float thresholdLow;
float thresholdHigh;
double thresholdBase;
double brightnessSquareScale;
float byteScale;
float inverse255;
double inverse1023;
double upperSlope;
double fixedExponent0454;
double fixedExponent22;
double inverseScale;
float overbright2;
float overbright4;
double inverse1024;
};
static constexpr StudioGammaConstants studioGammaConstants={3.0f,1,1.0f,0.125f,0.05000000074505806f,0.125,0.074999999999999997,255.0f,0.0039215688593685627f,0.00097751710654936461,0.875,0.45454543828964233,2.2000000476837158,255,0.5f,0.25f,0.0009765625};
