#pragma once
#include <array>
inline std::array<float,4> invertRecoveredQuaternion(const std::array<float,4>&q){
    std::array<float,4> result{-q[0],-q[1],-q[2],q[3]};
    const float yy=q[1]*q[1],xx=q[0]*q[0],zz=q[2]*q[2],ww=q[3]*q[3];
    const float squared=((yy+xx)+zz)+ww;
    if(squared!=0.0f){const float reciprocal=1.0f/squared;for(auto&c:result)c=c*reciprocal;}
    return result;
}
