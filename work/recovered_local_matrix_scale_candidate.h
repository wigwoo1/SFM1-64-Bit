#pragma once
#include "model_skinning_geometry.h"
// datamodel_pyd 0x100775C0 applies the float attribute to the nine basis
// entries, leaving translation offsets 0x0c,0x1c,0x2c untouched.
// Native object identity and a runtime oracle still require verification.
inline GeometryMatrixCandidate recoveredLocalMatrixScaleCandidate(GeometryMatrixCandidate matrix,float scale){
 if(!std::isfinite(scale))throw std::runtime_error("nonfinite local scale");
 for(size_t row=0;row<3;++row)for(size_t column=0;column<3;++column){auto&value=matrix[row*4+column];value*=scale;if(!std::isfinite(value))throw std::runtime_error("local scale overflow");}
 return matrix;
}
