#pragma once
#include "model_skinning_geometry.h"
#include "model_eyeball_metadata_candidate.h"
struct ModelEyeWorldCandidate {std::array<float,3>origin,up,forward;float radius,irisScale,zoffset;};
inline ModelEyeWorldCandidate modelEyeWorldCandidate(const ModelEyeballCandidate&eye,const std::vector<GeometryMatrixCandidate>&globals,const GeometryMatrixCandidate&placement){
 if(eye.bone>=globals.size())throw std::runtime_error("eye world bone index");auto matrix=geometryComposeCandidate(placement,globals[eye.bone]);
 auto transform=[&](const std::array<float,3>&input,bool point){std::array<float,3>out{};for(size_t r=0;r<3;++r){out[r]=point?matrix[r*4+3]:0;for(size_t k=0;k<3;++k)out[r]+=matrix[r*4+k]*input[k];if(!std::isfinite(out[r]))throw std::runtime_error("eye world transform overflow");}return out;};
 return {transform(eye.origin,true),transform(eye.up,false),transform(eye.forward,false),eye.radius,eye.irisScale,eye.zoffset};
}
