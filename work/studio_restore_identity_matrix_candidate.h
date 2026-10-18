#pragma once
#include <array>
inline void studioRestoreIdentityMatrixCandidate(std::array<float,12>& matrix){
 matrix.fill(0.0f);matrix[0]=matrix[5]=matrix[10]=1.0f;
}
