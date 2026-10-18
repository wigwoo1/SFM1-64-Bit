#pragma once
#include "ik_matrix_vector.h"
#include "transform_scale.h"
// Native null-pvTarget branch; double intermediates candidate for x87.
inline std::array<float,3> recoveredIkSavedPoleCandidate(const std::array<float,3>&pole,
 const RecoveredMatrix3x4&inverseFrame){
 auto out=rotateRecoveredIkVectorCandidate(pole,inverseFrame);
 const double reciprocal=1.0/recoveredTransformScaleWideCandidate(inverseFrame);
 for(auto&component:out)component=float(double(component)*reciprocal);
 return out;
}
