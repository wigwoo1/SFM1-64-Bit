#pragma once
#include "transform_matrix_composition.h"
#include <cmath>
inline double recoveredTransformScaleWideCandidate(const RecoveredMatrix3x4& m) {
    const float a0=m[0][0]*m[0][0], a2=m[2][0]*m[2][0], a1=m[1][0]*m[1][0];
    const float b2=m[2][1]*m[2][1], b1=m[1][1]*m[1][1], b0=m[0][1]*m[0][1];
    const float c1=m[1][2]*m[1][2], c2=m[2][2]*m[2][2], c0=m[0][2]*m[0][2];
    const float al=std::sqrt((a0+a2)+a1);
    const float bl=std::sqrt((b2+b1)+b0);
    const float cl=std::sqrt((c1+c2)+c0);
    // Exact products of two binary32 operands fit in binary64. The
    // third product's native x87 precision still needs wider coverage.
    const double product=(double(bl)*double(al))*double(cl);
    return std::pow(product,0.3333333432674408);
}
inline float recoveredTransformScaleCandidate(const RecoveredMatrix3x4& m) {
    return float(recoveredTransformScaleWideCandidate(m));
}
