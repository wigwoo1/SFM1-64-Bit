#pragma once
#include "matrix_quaternion.h"
#include "transform_scale.h"
inline std::array<float,4> extractRecoveredTransformOrientationCandidate(RecoveredMatrix3x4 matrix){
    const double wideScale=recoveredTransformScaleWideCandidate(matrix);
    float scale=float(wideScale);
    if(!(std::fabs(wideScale-1.0)<0x1p-23)){
        if(std::fabs(scale)<0x1p-23f)scale=scale<0?-0x1p-23f:0x1p-23f;
        const float reciprocal=1.0f/scale;
        if(!std::isnan(reciprocal)&&reciprocal!=1.0f)
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
                matrix[row][col]=matrix[row][col]*reciprocal;
    }
    return extractRecoveredMatrixQuaternion(matrix);
}
