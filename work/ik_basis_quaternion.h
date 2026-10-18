#pragma once
#include "ik_vector_normalize.h"
#include "ik_matrix_angles.h"
#include "ik_angle_quaternion.h"
#include "quaternion_normalize.h"
inline std::array<float,4> recoveredIkBasisQuaternionCandidate(std::array<float,3>d,
 const std::array<float,3>&guide){
 normalizeRecoveredIkVectorCandidate(d);
 const float dot=(guide[1]*d[1]+guide[0]*d[0])+guide[2]*d[2];
 std::array<float,3>o{};for(unsigned i=0;i<3;++i)o[i]=guide[i]-d[i]*dot;
 normalizeRecoveredIkVectorCandidate(o);
 std::array<float,3>t{o[1]*d[2]-o[2]*d[1],o[2]*d[0]-o[0]*d[2],o[0]*d[1]-o[1]*d[0]};
 normalizeRecoveredIkVectorCandidate(t);
 std::array<float,4>q{t[2]-o[1],o[0]-d[2],d[1]-t[0],((t[1]+d[0])+o[2])+1.0f};
 const float square=((q[0]*q[0]+q[1]*q[1])+q[2]*q[2])+q[3]*q[3];
 if(square>0x1p-23f)return normalizeRecoveredQuaternion(q);
 RecoveredMatrix3x4 m{};for(unsigned i=0;i<3;++i){m[i][0]=d[i];m[i][1]=t[i];m[i][2]=o[i];}
 return recoveredIkAngleQuaternionCandidate(extractRecoveredIkMatrixAnglesCandidate(m));
}
