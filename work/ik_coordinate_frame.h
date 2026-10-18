#pragma once
#include "transform_scale.h"
#include "transform_matrix_inverse.h"
// Native solver 105D0843..105D0914; caller supplies recovered start frame.
inline RecoveredMatrix3x4 recoveredIkCoordinateInverseCandidate(RecoveredMatrix3x4 frame){
 const float scale=recoveredTransformScaleCandidate(frame);
 const float squared=scale*scale;
 const float correction=1.0f/squared;
 // Native unordered comparison takes the multiply branch too.
 if(correction!=1.0f)for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
  frame[r][c]=frame[r][c]*correction;
 return invertRecoveredTransposeTransform(frame);
}
