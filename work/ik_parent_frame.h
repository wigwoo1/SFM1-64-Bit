#pragma once
#include "ik_coordinate_frame.h"
#include "quaternion_transform_matrix.h"
#include "transform_orientation.h"
#include "quaternion_product.h"
// Native 104C6100 mixed override math; graph selection handled separately.
inline RecoveredMatrix3x4 recoveredIkRotationOnlyParentFrameCandidate(
 const RecoveredMatrix3x4&ordinary,const std::array<float,4>&overrideWorldOrientation){
 return recoveredQuaternionTransformMatrix(overrideWorldOrientation,
  {ordinary[0][3],ordinary[1][3],ordinary[2][3]});
}
inline RecoveredMatrix3x4 recoveredIkPositionOnlyParentFrameCandidate(
 const RecoveredMatrix3x4&ordinary,const std::array<float,3>&overrideWorldPosition,
 const std::array<float,3>&childPosition,const std::array<float,4>&childOrientation){
 const auto orientation=multiplyRecoveredAlignedQuaternions(
  extractRecoveredTransformOrientationCandidate(ordinary),childOrientation);
 std::array<float,3>position{};for(unsigned i=0;i<3;++i)position[i]=childPosition[i]+overrideWorldPosition[i];
 const auto desired=recoveredQuaternionTransformMatrix(orientation,position);
 const auto local=recoveredQuaternionTransformMatrix(childOrientation,childPosition);
 return composeRecoveredTransforms(desired,recoveredIkCoordinateInverseCandidate(local));
}
