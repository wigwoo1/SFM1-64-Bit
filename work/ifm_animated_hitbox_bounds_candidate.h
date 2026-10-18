#pragma once
#include "ik_coordinate_frame.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>
using IfmBoundsCandidate=std::array<std::array<float,3>,2>;
struct IfmHitboxCandidate {int32_t bone=0;IfmBoundsCandidate bounds{};};
static_assert(sizeof(IfmHitboxCandidate)==28,"test semantic record size");
inline void checkIfmBoundsFiniteCandidate(const IfmBoundsCandidate&bounds){for(const auto&v:bounds)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite IFM bounds");}
inline void checkIfmMatrixFiniteCandidate(const RecoveredMatrix3x4&matrix){for(const auto&r:matrix)for(float x:r)if(!std::isfinite(x))throw std::runtime_error("nonfinite IFM bounds transform");}
// Original1049E820. Preserve its center/extent evaluation and summation order;
// transforming just min/max, or enumerating corners with a different rounding
// order, is not equivalent to this helper.
inline IfmBoundsCandidate ifmTransformBoundsCandidate(const RecoveredMatrix3x4&matrix,const IfmBoundsCandidate&bounds){
    checkIfmMatrixFiniteCandidate(matrix);checkIfmBoundsFiniteCandidate(bounds);
    std::array<float,3>center{},extent{};for(unsigned i=0;i<3;++i){const float sum=bounds[0][i]+bounds[1][i];center[i]=sum*0.5f;extent[i]=bounds[1][i]-center[i];}
    IfmBoundsCandidate out{};
    for(unsigned r=0;r<3;++r){
        const float x=matrix[r][0]*center[0],y=matrix[r][1]*center[1],z=matrix[r][2]*center[2];const float xy=x+y;const float xyz=xy+z;const float c=xyz+matrix[r][3];
        const float ey=std::fabs(matrix[r][1]*extent[1]),ex=std::fabs(matrix[r][0]*extent[0]),ez=std::fabs(matrix[r][2]*extent[2]);const float yx=ey+ex;const float e=yx+ez;
        out[0][r]=c-e;out[1][r]=c+e;
    }checkIfmBoundsFiniteCandidate(out);return out;
}
// Original1023EF30: first hitbox set only; setup request max256/mask100. The
// setup return boolean is ignored by native code. A provider must fill every
// referenced matrix or throw. Bone evaluation and live model binding are separate.
template<class SetupBones,class ModelWorld,class LocalScale>
std::optional<IfmBoundsCandidate> ifmAnimatedHitboxBoundsCandidate(
    const std::vector<IfmHitboxCandidate>*hitboxes,bool hasClock,int32_t clockTick,
    SetupBones setupBones,ModelWorld modelWorld,LocalScale localScale){
    if(!hitboxes||hitboxes->empty())return {};
    for(const auto&box:*hitboxes){if(box.bone<0||box.bone>=256)throw std::runtime_error("IFM hitbox bone exceeds palette");checkIfmBoundsFiniteCandidate(box.bounds);}
    const float time=hasClock?static_cast<float>(clockTick)*0.0001f:0.0f;
    std::array<RecoveredMatrix3x4,256>palette{};for(auto&m:palette)for(auto&r:m)r.fill(std::numeric_limits<float>::quiet_NaN());
    (void)setupBones(palette,256u,0x100u,time);
    auto world=modelWorld();checkIfmMatrixFiniteCandidate(world);const float scale=localScale();if(!std::isfinite(scale)||scale==0)throw std::runtime_error("invalid IFM model scale");const float reciprocal=1.0f/scale;if(!std::isfinite(reciprocal))throw std::runtime_error("IFM reciprocal scale overflow");
    if(reciprocal!=1.0f)for(auto&r:world)for(unsigned c=0;c<3;++c)r[c]*=reciprocal;
    checkIfmMatrixFiniteCandidate(world);
    // Original1008BAC0 is a geometric-mean scale correction followed by a
    // transpose inverse, NOT a general affine inverse for arbitrary shear.
    const auto inverse=recoveredIkCoordinateInverseCandidate(world);checkIfmMatrixFiniteCandidate(inverse);
    IfmBoundsCandidate out{};out[0].fill(std::numeric_limits<float>::max());out[1].fill(-std::numeric_limits<float>::max());
    for(const auto&box:*hitboxes){const auto&bone=palette[size_t(box.bone)];checkIfmMatrixFiniteCandidate(bone);const auto frame=composeRecoveredTransforms(inverse,bone);const auto bounds=ifmTransformBoundsCandidate(frame,box.bounds);
        // Native COMISS branches select the new value on equal values too.
        for(unsigned axis=0;axis<3;++axis){if(!(bounds[0][axis]>out[0][axis]))out[0][axis]=bounds[0][axis];if(!(out[1][axis]>bounds[1][axis]))out[1][axis]=bounds[1][axis];}
    }return out;
}
struct IfmMdlHitboxesCandidate {uint32_t boneCount=0;std::vector<IfmHitboxCandidate>firstSet;};
inline IfmMdlHitboxesCandidate readIfmMdlHitboxesCandidate(const std::vector<uint8_t>&bytes){
    auto span=[&](uint64_t offset,uint64_t count){if(offset>bytes.size()||count>bytes.size()-offset)throw std::runtime_error("MDL hitbox span");};
    auto integer=[&](size_t offset){span(offset,4);int32_t v;std::memcpy(&v,bytes.data()+offset,4);return v;};
    span(0,180);if(integer(0)!=0x54534449||integer(4)!=49)throw std::runtime_error("MDL49 hitboxes required");if(integer(76)<0||size_t(integer(76))!=bytes.size())throw std::runtime_error("MDL hitbox declared size");const int32_t bones=integer(156),sets=integer(172),setOffset=integer(176);if(bones<0||sets<0||setOffset<0)throw std::runtime_error("MDL hitbox count/offset");
    IfmMdlHitboxesCandidate out;out.boneCount=uint32_t(bones);if(!sets)return out;span(uint32_t(setOffset),uint64_t(sets)*12);const int32_t count=integer(size_t(setOffset)+4),relative=integer(size_t(setOffset)+8);const int64_t start=int64_t(setOffset)+relative;if(count<0||start<0)throw std::runtime_error("MDL first hitbox set");span(uint64_t(start),uint64_t(count)*68);
    out.firstSet.reserve(size_t(count));for(int32_t i=0;i<count;++i){const size_t offset=size_t(start)+size_t(i)*68;IfmHitboxCandidate box;box.bone=integer(offset);if(box.bone<0||box.bone>=bones||box.bone>=256)throw std::runtime_error("MDL hitbox bone");std::memcpy(box.bounds[0].data(),bytes.data()+offset+8,12);std::memcpy(box.bounds[1].data(),bytes.data()+offset+20,12);checkIfmBoundsFiniteCandidate(box.bounds);for(unsigned axis=0;axis<3;++axis)if(box.bounds[0][axis]>box.bounds[1][axis])throw std::runtime_error("inverted MDL hitbox");out.firstSet.push_back(box);}
    return out;
}
