#pragma once
#include "transform_matrix_composition.h"
inline RecoveredMatrix3x4 convertRecoveredLocalToWorld(
    const RecoveredMatrix3x4& local, const RecoveredMatrix3x4& positionParent,
    const RecoveredMatrix3x4& rotationParent, bool sameParent) {
    auto result=composeRecoveredTransforms(rotationParent,local);
    if(!sameParent)for(unsigned row=0;row<3;++row){
        const float y=positionParent[row][1]*local[1][3];
        const float x=positionParent[row][0]*local[0][3];
        const float yx=y+x;
        const float z=positionParent[row][2]*local[2][3];
        result[row][3]=(yx+z)+positionParent[row][3];
    }
    return result;
}
