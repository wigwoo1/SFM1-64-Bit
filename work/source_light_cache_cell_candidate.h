#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
// Native 10192950 cell bounds only. Collision/leaf-aware centering is separate.
inline std::array<float,6> sourceLightCacheCellCandidate(const std::array<float,3>&point){
 std::array<float,6>out{};
 for(size_t axis=0;axis<3;++axis){
  const float value=point[axis];
  if(!std::isfinite(value)||std::fabs(value)>32768.0f)throw std::runtime_error("cache-cell query outside checked Source coordinate domain");
  const int32_t width=axis==2?128:32;
  const int32_t magnitude=int32_t(std::fabs(value))/width;
  const int32_t cell=value>=0.0f?magnitude:-1-magnitude;
  out[axis]=float(cell*width);out[axis+3]=out[axis]+float(width);
 }
 return out;
}
