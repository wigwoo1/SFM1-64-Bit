#pragma once
#include "render_camera_candidate.h"
// Inversion stage at datamodel_pyd 0x10059750. For a rigid 3x4 matrix,
// transpose the basis and negate its product with the translation.
inline RenderCameraCandidate cameraRigidInverseCandidate(const std::array<std::array<float,4>,3>&matrix){
 for(auto&row:matrix)for(float v:row)if(!std::isfinite(v))throw std::runtime_error("nonfinite camera basis");for(size_t i=0;i<3;++i)for(size_t j=0;j<3;++j){double dot=0;for(size_t k=0;k<3;++k)dot+=double(matrix[k][i])*matrix[k][j];if(std::abs(dot-(i==j?1.0:0.0))>1e-4)throw std::runtime_error("nonrigid camera basis");}
 RenderCameraCandidate result{};for(size_t r=0;r<3;++r){for(size_t c=0;c<3;++c)result.rows[r*4+c]=matrix[c][r];float y=result.rows[r*4+1]*matrix[1][3],x=result.rows[r*4]*matrix[0][3];float sum=y+x;float z=result.rows[r*4+2]*matrix[2][3];result.rows[r*4+3]=-(sum+z);}result.rows[15]=1;return result;
}
