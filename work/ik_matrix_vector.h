#pragma once
#include "transform_matrix_composition.h"
// Native 104984F0 (direction) and 10498390 (point), pending direct probe.
inline std::array<float,3> rotateRecoveredIkVectorCandidate(const std::array<float,3>&v,
 const RecoveredMatrix3x4&m){
 std::array<float,3>out{};
 for(unsigned r=0;r<3;++r){const float y=m[r][1]*v[1],x=m[r][0]*v[0],z=m[r][2]*v[2];
  const float xy=y+x;out[r]=xy+z;}
 return out;
}
inline std::array<float,3> transformRecoveredIkPointCandidate(const std::array<float,3>&v,
 const RecoveredMatrix3x4&m){
 auto out=rotateRecoveredIkVectorCandidate(v,m);
 for(unsigned r=0;r<3;++r)out[r]=out[r]+m[r][3];
 return out;
}
