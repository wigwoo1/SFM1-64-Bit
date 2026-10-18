#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
// Recovered 10058A60 scalar SSE order; build with /fp:strict.
inline float softwareFlexNormalizeCandidate(std::array<float,3>&value){
 for(float x:value)if(!std::isfinite(x))throw std::runtime_error("nonfinite flex normal");
 float squared=value[0]*value[0];squared=squared+value[1]*value[1];squared=squared+value[2]*value[2];
 float length=std::sqrt(squared);if(!std::isfinite(length))throw std::runtime_error("flex normal length overflow");
 float inverse=1.0f/(length+0x1p-23f);for(float&x:value)x=x*inverse;return length;
}
