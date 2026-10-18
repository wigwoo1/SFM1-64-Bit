#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <stdexcept>
// Collision leaf contains two triangles; packed vertex indices use low9bits.
inline std::array<float,6> sourceDisplacementPairBoundsCandidate(const std::vector<std::array<float,3>>&vertices,
 const std::array<uint16_t,6>&packed){
 std::array<float,6>bounds;for(size_t i=0;i<3;++i){bounds[i]=std::numeric_limits<float>::max();bounds[i+3]=-std::numeric_limits<float>::max();}
 for(auto value:packed){const auto&point=vertices.at(value&511);for(size_t axis=0;axis<3;++axis){if(!std::isfinite(point[axis]))throw std::runtime_error("nonfinite collision vertex");if(point[axis]<bounds[axis])bounds[axis]=point[axis];if(point[axis]>bounds[axis+3])bounds[axis+3]=point[axis];}}
 return bounds;
}
