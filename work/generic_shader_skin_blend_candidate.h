#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

using GenericShaderSkinMatrixCandidate=std::array<std::array<float,4>,3>;
// CPU reference for installed generic VS20 static0/dynamic6 matrix blending.
// Inputs are ALREADY decoded shader weights and resolved cModel matrices.
// Does not read VVD/VTX, quantize weights, select a shader, or normalize normals.
// MAD uses separate binary32 multiply/add; native GPU rounding is unverified.
inline GenericShaderSkinMatrixCandidate genericShaderSkinBlendCandidate(
    const std::array<GenericShaderSkinMatrixCandidate,3>& matrices,
    const std::array<float,2>& weights){
    for(float w:weights)if(!std::isfinite(w))throw std::invalid_argument("nonfinite shader weight");
    const float sum=weights[1]+weights[0];
    const float residual=1.0f-sum;
    if(!std::isfinite(residual))throw std::overflow_error("shader residual overflow");
    GenericShaderSkinMatrixCandidate result{};
    for(size_t row=0;row<3;++row)for(size_t lane=0;lane<4;++lane){
        for(const auto&m:matrices)if(!std::isfinite(m[row][lane]))throw std::invalid_argument("nonfinite shader matrix");
        const float second=weights[1]*matrices[1][row][lane];
        const float first=matrices[0][row][lane]*weights[0];
        const float pair=first+second;
        const float third=matrices[2][row][lane]*residual;
        result[row][lane]=third+pair;
        if(!std::isfinite(result[row][lane]))throw std::overflow_error("shader blend overflow");
    }
    return result;
}
