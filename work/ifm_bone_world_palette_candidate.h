#pragma once
#include "ifm_animated_hitbox_bounds_candidate.h"
#include "quaternion_transform_matrix.h"
struct IfmBoneLocalPoseCandidate {
    int32_t parent=-1;uint32_t flags=0;
    std::array<float,3>position{};std::array<float,4>orientation{0,0,0,1};float scale=1;
};
// Procedural evaluation disabled path of IFM102411B0. An optional prepared
// prepass palette may suppress local evaluation. Output is bone-to-world,
// NOT skin matrices: no inverse bind.
// Inputs are a stable prepared pose, not the native animation evaluator/cache.
// Native visits MDL order. Refuse a requested child without a computed earlier
// parent rather than silently using an uninitialized native cache matrix.
inline std::vector<std::optional<RecoveredMatrix3x4>>ifmBoneWorldPaletteCandidate(
    const std::vector<IfmBoneLocalPoseCandidate>&bones,const RecoveredMatrix3x4&root,uint32_t mask,
    const std::vector<std::optional<RecoveredMatrix3x4>>&precomputed){
    if(bones.size()>256)throw std::runtime_error("IFM bone palette exceeds256");
    if(precomputed.size()!=bones.size())throw std::runtime_error("IFM precomputed palette size");
    auto out=precomputed;for(const auto&m:out)if(m)checkIfmMatrixFiniteCandidate(*m);
    // Original prepass writes overrides even outside mask. Their computed bits
    // suppress local evaluation; requested children may use those matrices.
    for(size_t i=0;i<bones.size();++i){const auto&b=bones[i];if(!(b.flags&mask)||out[i])continue;
        if(b.parent< -1||(b.parent>=0&&(size_t(b.parent)>=i||!out[size_t(b.parent)])))throw std::runtime_error("IFM requested bone parent unavailable");
        for(float v:b.position)if(!std::isfinite(v))throw std::runtime_error("IFM nonfinite bone position");
        for(float v:b.orientation)if(!std::isfinite(v))throw std::runtime_error("IFM nonfinite bone orientation");
        if(!std::isfinite(b.scale))throw std::runtime_error("IFM nonfinite bone scale");
        auto local=recoveredQuaternionTransformMatrix(b.orientation,b.position);
        if(b.scale!=1.0f)for(auto&r:local)for(unsigned c=0;c<3;++c)r[c]*=b.scale;
        checkIfmMatrixFiniteCandidate(local);
        const auto&parent=b.parent==-1?root:*out[size_t(b.parent)];checkIfmMatrixFiniteCandidate(parent);
        auto world=composeRecoveredTransforms(parent,local);checkIfmMatrixFiniteCandidate(world);out[i]=world;
    }return out;
}
// Preserve the original no-precomputed API and behavior for existing callers.
inline std::vector<std::optional<RecoveredMatrix3x4>>ifmBoneWorldPaletteCandidate(
    const std::vector<IfmBoneLocalPoseCandidate>&bones,const RecoveredMatrix3x4&root,uint32_t mask){
    return ifmBoneWorldPaletteCandidate(bones,root,mask,std::vector<std::optional<RecoveredMatrix3x4>>(bones.size()));
}
