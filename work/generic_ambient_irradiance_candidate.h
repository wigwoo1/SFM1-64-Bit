#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

using GenericAmbientCubeCandidate = std::array<std::array<float,3>,6>;

// Recovered ambient-only expression in installed generic VS20 static0,
// dynamic2/6/10/18. Face order: +X,-X,+Y,-Y,+Z,-Z (c21..26).
// Input is the shader's ALREADY transformed/normalized world normal. This
// function deliberately does not choose a model origin, sample a BSP, normalize
// a normal, add local/static lights, clamp, apply textures or convert gamma.
// c0.x=0 is an explicit reference input, not an observed live shader constant.
// /fp:strict gives a reproducible non-fused CPU reference. Actual GPU MAD/NRM
// precision and the benchmark's active shader/material path remain unverified.
inline std::array<float,3> genericAmbientIrradianceCandidate(
    const std::array<float,3>&worldNormal,const GenericAmbientCubeCandidate&cube){
    for(float value:worldNormal)if(!std::isfinite(value))throw std::invalid_argument("nonfinite ambient normal");
    for(const auto&face:cube)for(float value:face)if(!std::isfinite(value))throw std::invalid_argument("nonfinite ambient cube");
    const float xx=worldNormal[0]*worldNormal[0];
    const float yy=worldNormal[1]*worldNormal[1];
    const float zz=worldNormal[2]*worldNormal[2];
    const auto&x=cube[worldNormal[0]<0.0f?1:0];
    const auto&y=cube[worldNormal[1]<0.0f?3:2];
    const auto&z=cube[worldNormal[2]<0.0f?5:4];
    std::array<float,3>result{};
    for(unsigned channel=0;channel<3;++channel){
        // Native shader starts Y, then MAD X, then MAD Z. Keep this order.
        const float first=yy*y[channel];
        const float xTerm=xx*x[channel];
        const float second=xTerm+first;
        const float zTerm=zz*z[channel];
        result[channel]=zTerm+second;
        if(!std::isfinite(result[channel]))throw std::overflow_error("ambient irradiance overflow");
    }
    return result;
}
