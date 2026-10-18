#pragma once
#include "ik_two_bone_geometry.h"
#include "ik_basis_quaternion.h"
#include "quaternion_product.h"
#include "quaternion_inverse.h"
struct RecoveredIkPreparedSolutionCandidate{
 std::array<float,4>start,mid;std::array<float,3>bendPosition,cappedTarget;
};
// Caller provides root, target and pole in the recovered solver frame.
inline RecoveredIkPreparedSolutionCandidate recoveredIkPreparedSolutionCandidate(
 const std::array<float,3>&root,const std::array<float,3>&target,
 const std::array<float,3>&pole,float upperLength,float lowerLength,
 const std::array<float,4>&startOffset,const std::array<float,4>&midOffset){
 std::array<float,3>direction{};for(unsigned i=0;i<3;++i)direction[i]=target[i]-root[i];
 float distance=normalizeRecoveredIkVectorCandidate(direction);
 const float reach=lowerLength+upperLength;
 if(!(reach>distance))distance=reach;
 std::array<float,3>capped{};for(unsigned i=0;i<3;++i)capped[i]=direction[i]*distance+root[i];
 const auto bend=recoveredIkBendCandidate(direction,pole,upperLength,lowerLength,distance);
 const auto start=multiplyRecoveredAlignedQuaternions(recoveredIkBasisQuaternionCandidate(bend.offset,bend.plane),startOffset);
 std::array<float,3>bendPosition{},second{};for(unsigned i=0;i<3;++i){
  bendPosition[i]=bend.offset[i]+root[i];second[i]=capped[i]-bendPosition[i];}
 const auto secondWorld=recoveredIkBasisQuaternionCandidate(second,bend.plane);
 const auto relative=multiplyRecoveredAlignedQuaternions(invertRecoveredQuaternion(start),secondWorld);
 return {start,multiplyRecoveredAlignedQuaternions(relative,midOffset),bendPosition,capped};
}
