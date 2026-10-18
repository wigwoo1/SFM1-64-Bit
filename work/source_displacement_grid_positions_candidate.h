#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

using SourceDisplacementGridPointCandidate = std::array<float,3>;
struct SourceDisplacementGridOffsetCandidate {
 SourceDisplacementGridPointCandidate direction{};
 float distance=0;
};

// Bounded reconstruction of CoreDisp position generator 101415D0.
// Caller supplies already ordered corners. Elevation and subdivision offsets
// are explicitly zero in this API; it is not the complete BSP mesh loader.
// Preserve the original single-precision operation order; build /fp:strict.
inline std::vector<SourceDisplacementGridPointCandidate>
sourceDisplacementGridPositionsCandidate(uint32_t power,
 const std::array<SourceDisplacementGridPointCandidate,4>&corners,
 const std::vector<SourceDisplacementGridOffsetCandidate>&offsets){
 if(power<1||power>4)throw std::runtime_error("displacement grid power outside supported range");
 const uint32_t width=(1u<<power)+1;
 if(offsets.size()!=size_t(width)*width)throw std::runtime_error("displacement grid offset count mismatch");
 for(const auto&point:corners)for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement corner");
 for(const auto&offset:offsets){for(float v:offset.direction)if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement direction");if(!std::isfinite(offset.distance))throw std::runtime_error("nonfinite displacement distance");}
 const float reciprocal=1.f/float(width-1);
 SourceDisplacementGridPointCandidate leftStep{},rightStep{};
 for(size_t a=0;a<3;++a){
  const float leftDifference=corners[1][a]-corners[0][a];
  const float rightDifference=corners[2][a]-corners[3][a];
  leftStep[a]=leftDifference*reciprocal;rightStep[a]=rightDifference*reciprocal;
 }
 std::vector<SourceDisplacementGridPointCandidate>positions(size_t(width)*width);
 for(uint32_t row=0;row<width;++row){
  SourceDisplacementGridPointCandidate left{},right{},columnStep{};
  for(size_t a=0;a<3;++a){
   const float leftProduct=leftStep[a]*float(row),rightProduct=rightStep[a]*float(row);
   left[a]=leftProduct+corners[0][a];right[a]=rightProduct+corners[3][a];
   const float difference=right[a]-left[a];columnStep[a]=difference*reciprocal;
  }
  for(uint32_t column=0;column<width;++column){
   const size_t index=size_t(row)*width+column;
   for(size_t a=0;a<3;++a){
    const float product=columnStep[a]*float(column);
    const float base=product+left[a];
    // Original unconditionally adds its (zero here) subdivision offset +1Ch.
    // This addition matters for signed zero and must not be optimized away.
    const float subdivided=base+0.f;
    const float displacement=offsets[index].direction[a]*offsets[index].distance;
    const float position=subdivided+displacement;
    if(!std::isfinite(position))throw std::runtime_error("nonfinite generated displacement position");
    positions[index][a]=position;
   }
  }
 }
 return positions;
}
