#pragma once
#include "ifm_bounds_frames_candidate.h"
#include "source_model_lighting_point_candidate.h"
// Semantic owned packet, NOT a packed copy of the 68-byte original x86 ABI.
// Identity handles retain native pointer width; matrix and optional point own
// their storage and remain valid when the packet is copied or moved.
struct IfmModelDrawPacketCandidate {
    std::array<float,3>position{},angles{};
    RecoveredMatrix3x4 modelToWorld{};
    std::optional<std::array<float,3>>lightingOrigin;
    uintptr_t renderable=0,model=0;
    uint32_t flags=0;int32_t entityIndex=-1,skin=0,body=0,hitboxSet=0;
    uint16_t instance=0;
};
// Original10243A57..10243B73: Euler matrix, local uniform scale of its nine
// basis entries, render parameters. Lighting offset is always null here.
inline IfmModelDrawPacketCandidate ifmModelDrawPacketCandidate(
    const std::array<float,3>&position,const std::array<float,3>&angles,float localScale,
    uintptr_t renderable,uintptr_t model,uint32_t flags,int32_t skin,int32_t body,
    uint16_t instance,const std::optional<std::array<float,3>>&origin){
    if(!std::isfinite(localScale))throw std::runtime_error("nonfinite IFM draw scale");
    IfmModelDrawPacketCandidate out;out.position=position;out.angles=angles;
    out.modelToWorld=ifmBoundsAngleMatrixCandidate(angles,position);
    if(localScale!=1.0f)for(auto&r:out.modelToWorld)for(unsigned c=0;c<3;++c)r[c]*=localScale;
    checkIfmMatrixFiniteCandidate(out.modelToWorld);
    if(origin)for(float x:*origin)if(!std::isfinite(x))throw std::runtime_error("nonfinite IFM draw lighting origin");
    out.lightingOrigin=origin;out.renderable=renderable;out.model=model;out.flags=flags;out.skin=skin;out.body=body;out.instance=instance;return out;
}
// Original Source caller obtains its default point before100D5760 consumes the
// explicit-origin field28. The IFM packet supplies no lighting-offset field24.
template<class Attachment>
std::array<float,3>ifmModelDrawLightingPointCandidate(const IfmModelDrawPacketCandidate&packet,
    const std::array<float,3>&localIlluminationPoint,int32_t attachmentIndex,Attachment attachment){
    const auto point=sourceModelIlluminationPointCandidate(localIlluminationPoint,packet.modelToWorld,attachmentIndex,attachment);
    return sourceModelLightingOriginCandidate(point,nullptr,packet.lightingOrigin?&*packet.lightingOrigin:nullptr);
}
