#pragma once
#include "transform_matrix_composition.h"
#include "quaternion_normalize.h"
inline std::array<float,4> extractRecoveredMatrixQuaternion(const RecoveredMatrix3x4&m){
    std::array<float,4> q{};const float a=m[0][0],b=m[1][1],c=m[2][2];
    const float trace=((a+b)+c)+1.0f;
    if(trace>0x1.000002p0f)q={m[2][1]-m[1][2],m[0][2]-m[2][0],m[1][0]-m[0][1],trace};
    else if(a>b&&a>c)q={((a+1.0f)-b)-c,m[1][0]+m[0][1],m[2][0]+m[0][2],m[2][1]-m[1][2]};
    else if(b>c)q={m[1][0]+m[0][1],((b+1.0f)-a)-c,m[2][1]+m[1][2],m[0][2]-m[2][0]};
    else q={m[2][0]+m[0][2],m[2][1]+m[1][2],((c+1.0f)-a)-b,m[1][0]-m[0][1]};
    return normalizeRecoveredQuaternion(q);
}
