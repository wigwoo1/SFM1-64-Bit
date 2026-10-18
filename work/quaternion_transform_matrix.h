#pragma once
#include "transform_matrix_composition.h"
inline RecoveredMatrix3x4 recoveredQuaternionTransformMatrix(
    const std::array<float,4>& q, const std::array<float,3>& p) {
    // 1049DFC0 does not normalize its quaternion input.
    const auto product=[&](unsigned a,unsigned b){const float twice=q[a]*2.0f;return twice*q[b];};
    RecoveredMatrix3x4 m{};
    m[0][0]=(1.0f-product(1,1))-product(2,2);
    m[1][0]=product(0,1)+product(3,2);
    m[2][0]=product(0,2)-product(3,1);
    m[0][1]=product(0,1)-product(3,2);
    m[1][1]=(1.0f-product(0,0))-product(2,2);
    m[2][1]=product(1,2)+product(3,0);
    m[0][2]=product(0,2)+product(3,1);
    m[1][2]=product(1,2)-product(3,0);
    m[2][2]=(1.0f-product(0,0))-product(1,1);
    for(unsigned i=0;i<3;++i)m[i][3]=p[i];
    return m;
}
