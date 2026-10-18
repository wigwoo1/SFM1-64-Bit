#pragma once
#include <array>
#include <cmath>
// 10498270: unlike quaternion normalization, adds epsilon to length.
// Pending direct native differential validation.
inline float normalizeRecoveredIkVectorCandidate(std::array<float,3>&v){
    const float xx=v[0]*v[0],yy=v[1]*v[1],zz=v[2]*v[2];
    const float xy=xx+yy;
    const float length=std::sqrt(xy+zz);
    const float inverse=1.0f/(length+0x1p-23f);
    for(auto& component:v)component=component*inverse;
    return length;
}
