#pragma once
#include "matrix_compose_candidate.h"
#include "vector_transform_candidate.h"
// Caller supplies evaluated frame matrices and the original frame identity relation.
// This helper does not resolve parents, overrides, cycles or timeline values.
inline std::array<uint32_t,12> splitFrameMatrixCandidate(const std::array<uint32_t,12>& local,const std::array<uint32_t,12>& positionFrame,const std::array<uint32_t,12>& rotationFrame,bool sameFrame){
    auto result=matrixComposeCandidate(rotationFrame,local);
    if(!sameFrame){
        const std::array<uint32_t,3> position{{local[3],local[7],local[11]}};
        const auto transformed=vectorTransformCandidate(position,positionFrame);
        for(unsigned row=0;row<3;++row)result[row*4+3]=transformed[row];
    }
    return result;
}
