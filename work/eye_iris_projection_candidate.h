#pragma once
#include "model_eye_world_candidate.h"
// Authored forward-direction candidate. Gaze, zoffset and native vector
// generation have not been recovered; this is not full Source eye parity.
struct EyeIrisProjectionCandidate {std::array<float,4>u,v;};
inline EyeIrisProjectionCandidate eyeIrisProjectionCandidate(const ModelEyeWorldCandidate&eye){
 auto unit=[](std::array<float,3>a){float length=0;for(auto value:a){if(!std::isfinite(value))throw std::runtime_error("nonfinite iris basis");length+=value*value;}if(!(length>1e-12f)||!std::isfinite(length))throw std::runtime_error("degenerate iris basis");length=std::sqrt(length);for(auto&value:a)value/=length;return a;};
 auto forward=unit(eye.forward);auto up=unit(eye.up);std::array<float,3>right={up[1]*forward[2]-up[2]*forward[1],up[2]*forward[0]-up[0]*forward[2],up[0]*forward[1]-up[1]*forward[0]};right=unit(right);
 up={forward[1]*right[2]-forward[2]*right[1],forward[2]*right[0]-forward[0]*right[2],forward[0]*right[1]-forward[1]*right[0]};up=unit(up);
 if(!(eye.irisScale>0)||!std::isfinite(eye.irisScale))throw std::runtime_error("invalid iris scale");EyeIrisProjectionCandidate result{};result.u[3]=result.v[3]=.5f;
 for(size_t n=0;n<3;++n){if(!std::isfinite(eye.origin[n]))throw std::runtime_error("nonfinite eye origin");result.u[n]=right[n]*eye.irisScale;result.v[n]=up[n]*eye.irisScale;result.u[3]-=result.u[n]*eye.origin[n];result.v[3]-=result.v[n]*eye.origin[n];}
 return result;
}
