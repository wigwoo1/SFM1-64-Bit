#pragma once
#include "quaternion_position_matrix_candidate.h"
// Requires ProjectedLightSavedFieldsCandidate. Local saved matrix only;
// separate position/rotation frame lookup and animated evaluation are not included.
inline std::array<uint32_t,12> projectedLightLocalMatrixCandidate(const ProjectedLightSavedFieldsCandidate& snapshot){
    auto matrix=quaternionPositionMatrixCandidate(snapshot.localOrientationWords,snapshot.localPositionWords);
    const auto word=snapshot.localScaleWord;
    // Recovered UCOMISS/LAHF branch skips only ordered scale equal to one.
    // This bit test preserves results; it does not reproduce FP exception flags.
    if(snapshot.hasLocalScale&&word!=0x3f800000u){
        float scale;std::memcpy(&scale,&word,4);
        for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){
            const unsigned index=row*4+column;float value;std::memcpy(&value,&matrix[index],4);
            _mm_store_ss(&value,_mm_mul_ss(_mm_set_ss(value),_mm_set_ss(scale)));
            std::memcpy(&matrix[index],&value,4);
        }
    }
    return matrix;
}
