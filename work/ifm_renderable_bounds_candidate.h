#pragma once
#include "ifm_animated_hitbox_bounds_candidate.h"
#include "quaternion_transform_matrix.h"
#include "transform_matrix_inverse.h"
struct IfmStudioBoundsCandidate {bool ready=true;IfmBoundsCandidate hull{},view{};std::vector<IfmBoundsCandidate>sequences;};
struct IfmFirstBonePoseCandidate {std::array<float,3>position{};std::array<float,4>orientation{0,0,0,1};};
struct IfmRenderableBoundsStateCandidate {
    bool modelPresent=false,rawModelResolved=false,gameModelTyped=false,computeBounds=false;
    uint8_t renderableFlags=0;int32_t category=0,sequence=-1,runtimeMode=0,childCount=0;
};
// Native1011C4F0/1011C550 calls use incoming as argument1 and current as argument2.
// Equal values retain current, unlike the inline animated-hitbox reduction.
inline IfmBoundsCandidate ifmExpandBoundsCandidate(IfmBoundsCandidate current,const IfmBoundsCandidate&incoming){
    checkIfmBoundsFiniteCandidate(current);checkIfmBoundsFiniteCandidate(incoming);
    for(unsigned i=0;i<3;++i){if(incoming[0][i]<current[0][i])current[0][i]=incoming[0][i];if(incoming[1][i]>current[1][i])current[1][i]=incoming[1][i];}return current;
}
// Original1023F310, on a stable post-synchronization snapshot. Providers preserve
// lazy acquisition and unused-input bypass. Native cache mutation, live category
// binding, animated setup and child-frame selection are provider contracts.
template<class Services>
IfmBoundsCandidate ifmRenderableBoundsCandidate(const IfmRenderableBoundsStateCandidate&s,Services&services){
    IfmBoundsCandidate result{{{-16,-16,0},{16,16,72}}};
    if(!s.modelPresent)return result;
    auto adjustChild=[&](IfmBoundsCandidate bounds){const auto matrix=services.childTransform();return matrix?ifmTransformBoundsCandidate(*matrix,bounds):bounds;};
    auto fallback=[&]{auto bounds=services.modelBounds();checkIfmBoundsFiniteCandidate(bounds);return adjustChild(bounds);};
    if(s.category==1)return fallback();if(s.category!=3)return result;
    services.synchronize();const bool game=s.rawModelResolved&&s.gameModelTyped;
    const bool compute=game&&(s.computeBounds||(s.renderableFlags&0x20));
    if(compute){const auto bounds=services.animatedBounds();if(bounds){checkIfmBoundsFiniteCandidate(*bounds);return *bounds;}}
    const auto*studio=services.studio();if(!studio||!studio->ready||s.sequence==-1)return fallback();
    bool zero=true;for(const auto&v:studio->hull)for(float x:v)zero=zero&&(x==0.0f);
    result=zero?studio->view:studio->hull;checkIfmBoundsFiniteCandidate(result);
    if(studio->sequences.empty())throw std::runtime_error("IFM studio bounds need sequence zero");
    const size_t sequence=s.sequence<0||uint64_t(s.sequence)>=studio->sequences.size()?0:size_t(s.sequence);
    result=ifmExpandBoundsCandidate(result,studio->sequences[sequence]);result=adjustChild(result);
    // This extra union is only on the studio/sequence path. It does not apply
    // after the engine-model fallback or a successful animated-hitbox result.
    if(s.runtimeMode==0&&compute){const auto bone=services.firstBonePose();if(bone){
        for(float x:bone->position)if(!std::isfinite(x))throw std::runtime_error("nonfinite first bone position");for(float x:bone->orientation)if(!std::isfinite(x))throw std::runtime_error("nonfinite first bone orientation");
        // Native computes an inverse into EBP-64, but the AABB call at1023F621
        // receives the original EBP-34 matrix. Do not substitute that inverse.
        const auto transform=recoveredQuaternionTransformMatrix(bone->orientation,bone->position);result=ifmExpandBoundsCandidate(result,ifmTransformBoundsCandidate(transform,result));
    }}return result;
}
// Original10240520 wrapper: this gate uses a raw resolved DAG and positive child
// count; it does not repeat the GameModel IsA predicate from the dispatcher.
template<class Services>
IfmBoundsCandidate ifmRenderableBoundsWrapperCandidate(const IfmRenderableBoundsStateCandidate&s,Services&services){
    auto result=ifmRenderableBoundsCandidate(s,services);
    if(s.runtimeMode!=0&&s.rawModelResolved&&s.childCount>0)result=ifmTransformBoundsCandidate(services.wrapperTransform(),result);
    return result;
}
inline IfmStudioBoundsCandidate readIfmMdlStudioBoundsCandidate(const std::vector<uint8_t>&bytes){
    auto span=[&](uint64_t offset,uint64_t count){if(offset>bytes.size()||count>bytes.size()-offset)throw std::runtime_error("MDL studio bounds span");};
    auto integer=[&](size_t offset){span(offset,4);int32_t value;std::memcpy(&value,bytes.data()+offset,4);return value;};
    span(0,196);if(integer(0)!=0x54534449||integer(4)!=49||integer(76)<0||size_t(integer(76))!=bytes.size())throw std::runtime_error("MDL49 studio bounds header");
    IfmStudioBoundsCandidate out;std::memcpy(out.view.data(),bytes.data()+104,24);std::memcpy(out.hull.data(),bytes.data()+128,24);checkIfmBoundsFiniteCandidate(out.view);checkIfmBoundsFiniteCandidate(out.hull);
    const int32_t count=integer(188),offset=integer(192);if(count<0||offset<0)throw std::runtime_error("MDL sequence bounds count/offset");span(uint32_t(offset),uint64_t(count)*212);out.sequences.resize(size_t(count));
    for(size_t i=0;i<out.sequences.size();++i){std::memcpy(out.sequences[i].data(),bytes.data()+size_t(offset)+i*212+32,24);checkIfmBoundsFiniteCandidate(out.sequences[i]);}return out;
}
