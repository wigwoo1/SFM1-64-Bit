#pragma once
#include <array>
inline std::array<float,3> blendRecoveredPointConstraintPosition(
    const std::array<float,3>& average,float influence,const std::array<float,3>& slaveWorld) {
    auto result=average;
    if(influence<1.0f){const float remainder=1.0f-influence;
        for(unsigned c=0;c<3;++c){const float base=slaveWorld[c]*remainder;
            const float weighted=average[c]*influence;result[c]=weighted+base;}}
    return result;
}
