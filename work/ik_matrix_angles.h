#pragma once
#include "transform_matrix_composition.h"
#include <array>
#include <cmath>
// 1049E4D0 angle extraction; legacy atan2 precision still needs native tests.
inline std::array<float,3> extractRecoveredIkMatrixAnglesCandidate(const RecoveredMatrix3x4&m){
 const float yy=m[1][0]*m[1][0],xx=m[0][0]*m[0][0];
 const float horizontal=std::sqrt(yy+xx);
 constexpr float degrees=0x1.ca5dc2p+5f;
 const auto angle=[](float y,float x){return float(std::atan2(double(y),double(x)));};
 const float pitch=angle(-m[2][0],horizontal)*degrees;
 if(horizontal>0x1.0624dep-10f)
  return {pitch,angle(m[1][0],m[0][0])*degrees,angle(m[2][1],m[2][2])*degrees};
 return {pitch,angle(-m[0][1],m[1][1])*degrees,0.0f};
}
