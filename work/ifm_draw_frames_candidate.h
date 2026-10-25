#pragma once
#include "ifm_bounds_frames_candidate.h"
#include "ik_matrix_angles.h"
#include "ik_matrix_vector.h"
#include "transform_scale.h"

struct IfmDecomposedFrameCandidate {
    std::array<float,3>angles{},position{};float scale=1;
};
// Original1023BA60: a geometric-mean scale, not independent axis scales.
// Only the orientation basis is divided by scale; translation is preserved.
// Native legacy pow/atan precision is a separately tested compatibility limit.
inline IfmDecomposedFrameCandidate ifmDecomposeFrameCandidate(RecoveredMatrix3x4 matrix){
    checkIfmMatrixFiniteCandidate(matrix);
    const double wide=recoveredTransformScaleWideCandidate(matrix);
    IfmDecomposedFrameCandidate result;result.scale=float(wide);
    if(!std::isfinite(wide)||!std::isfinite(result.scale))throw std::runtime_error("draw frame scale overflow");
    if(!(std::fabs(wide-1.0)<0x1p-23)){
        if(std::fabs(result.scale)<0x1p-23f)result.scale=result.scale<0?-0x1p-23f:0x1p-23f;
        const float reciprocal=1.0f/result.scale;
        if(reciprocal!=1.0f)
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)matrix[r][c]=matrix[r][c]*reciprocal;
    }
    checkIfmMatrixFiniteCandidate(matrix);
    result.angles=extractRecoveredIkMatrixAnglesCandidate(matrix);
    for(unsigned r=0;r<3;++r){result.position[r]=matrix[r][3];if(!std::isfinite(result.angles[r]))throw std::runtime_error("draw frame angle overflow");}
    return result;
}
// Original102402C0 frame writes on a stable snapshot. The caller supplies raw
// model resolution, typed child0, world matrices and INNER1023F310 bounds.
// The bounds provider observes the intermediate frames: position38 is still
// model translation, while angles44 has already become child-world angles.
// Cache/spatial notification and mutable datamodel lifetime are not implemented
// here. This value-returning helper never mutates a document or renderer.
template<class Services>
IfmBoundsWrapperFramesCandidate ifmDrawFramesCandidate(int32_t runtimeMode,Services&services){
    const auto model=services.resolveDag();
    const auto world=model?services.world(*model):RecoveredMatrix3x4{{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    const auto root=ifmDecomposeFrameCandidate(world);
    IfmBoundsWrapperFramesCandidate frames;frames.position50=root.position;frames.angles5c=root.angles;
    frames.position38=frames.position50;frames.angles44=frames.angles5c;
    const auto resolved=services.resolveDag();
    if(runtimeMode==0||!resolved||services.childCount(*resolved)<=0)return frames;
    const auto child=services.typedChildZero(*resolved);if(!child)return frames;
    frames.angles44=ifmDecomposeFrameCandidate(services.world(*child)).angles;
    const auto bounds=services.innerBounds(frames);checkIfmBoundsFiniteCandidate(bounds);
    const auto parent=services.world(*resolved);checkIfmMatrixFiniteCandidate(parent);
    std::array<float,3>center{};
    for(unsigned i=0;i<3;++i){center[i]=(bounds[1][i]+bounds[0][i])*0.5f;if(!std::isfinite(center[i]))throw std::runtime_error("draw frame midpoint overflow");}
    frames.position38=transformRecoveredIkPointCandidate(center,parent);
    for(float x:frames.position38)if(!std::isfinite(x))throw std::runtime_error("draw frame center transform overflow");
    return frames;
}
