#pragma once
#include "ik_matrix_vector.h"
#include <cmath>
#include <cstdint>
#include <stdexcept>

inline std::array<float,3> sourceLightingTransformPointCandidate(
 const std::array<float,3>&point,const RecoveredMatrix3x4&matrix){
 for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite model illumination point");
 for(const auto&row:matrix)for(float v:row)if(!std::isfinite(v))throw std::runtime_error("nonfinite selected lighting matrix");
 const auto result=transformRecoveredIkPointCandidate(point,matrix);
 for(float v:result)if(!std::isfinite(v))throw std::runtime_error("model illumination transform overflow");return result;
}

// Original complete100D2780: studio header+5C point, header2+8 optional
// attachment index. A positive index tries the renderable's matrix query;
// failed queries and nonpositive indices use the supplied default transform.
template<class GetAttachment>
std::array<float,3> sourceModelIlluminationPointCandidate(
 const std::array<float,3>&localPoint,const RecoveredMatrix3x4&modelToWorld,
 int32_t attachmentIndex,GetAttachment getAttachment){
 RecoveredMatrix3x4 attachment{};
 if(attachmentIndex>0&&getAttachment(attachmentIndex,attachment))return sourceLightingTransformPointCandidate(localPoint,attachment);
 return sourceLightingTransformPointCandidate(localPoint,modelToWorld);
}

// Original100D5760..100D57D2: an explicit point bypasses both the default
// point and lighting-offset matrix. Only selected inputs are validated.
inline std::array<float,3> sourceModelLightingOriginCandidate(
 const std::array<float,3>&defaultPoint,const RecoveredMatrix3x4*lightingOffset,
 const std::array<float,3>*explicitPoint){
 if(explicitPoint){for(float v:*explicitPoint)if(!std::isfinite(v))throw std::runtime_error("nonfinite explicit lighting point");return *explicitPoint;}
 if(lightingOffset)return sourceLightingTransformPointCandidate(defaultPoint,*lightingOffset);
 for(float v:defaultPoint)if(!std::isfinite(v))throw std::runtime_error("nonfinite default lighting point");return defaultPoint;
}
