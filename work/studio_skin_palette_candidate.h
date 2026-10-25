#pragma once
#include "ifm_bone_world_palette_candidate.h"
#include <algorithm>
struct StudioSkinPaletteMetadataCandidate {
    bool staticModel=false,linear=false;std::vector<uint32_t>flags;std::vector<RecoveredMatrix3x4>inverseBind;
};
// Original studiorender10001D90 selects header2 linear-bone flags/poseToBone
// when available, otherwise legacy216-byte bone descriptors. Prepared IFM
// matrices are already bone-to-world: never apply model placement again.
inline StudioSkinPaletteMetadataCandidate readStudioSkinPaletteMetadataCandidate(const std::vector<uint8_t>&mdl){
    auto span=[&](uint64_t at,uint64_t n){if(at>mdl.size()||n>mdl.size()-at)throw std::runtime_error("studio skin MDL span");};
    auto word=[&](size_t at){span(at,4);uint32_t v;std::memcpy(&v,mdl.data()+at,4);return v;};
    if(word(0)!=0x54534449||word(4)!=49||word(76)!=mdl.size())throw std::runtime_error("studio skin MDL49 identity");
    StudioSkinPaletteMetadataCandidate result;result.staticModel=(word(152)&0x10)!=0;if(result.staticModel)return result;
    const auto count=word(156);if(count>256)throw std::runtime_error("studio skin bone count");result.flags.resize(count);result.inverseBind.resize(count);
    const auto header2=word(400);uint32_t linear=0;
    if(header2){const auto relative=word(size_t(header2)+16);if(relative){const auto offset=uint64_t(header2)+relative;span(offset,28);linear=uint32_t(offset);if(word(linear)!=count)throw std::runtime_error("studio linear bone count mismatch");}}
    result.linear=linear!=0;
    if(linear){const auto flags=uint64_t(linear)+word(size_t(linear)+4),matrices=uint64_t(linear)+word(size_t(linear)+24);span(flags,uint64_t(count)*4);span(matrices,uint64_t(count)*48);
        for(size_t i=0;i<count;++i){result.flags[i]=word(size_t(flags)+i*4);std::memcpy(result.inverseBind[i].data(),mdl.data()+size_t(matrices)+i*48,48);}}
    else{const auto start=word(160);span(start,uint64_t(count)*216);for(size_t i=0;i<count;++i){result.flags[i]=word(size_t(start)+i*216+160);std::memcpy(result.inverseBind[i].data(),mdl.data()+size_t(start)+i*216+96,48);}}
    return result;
}
inline std::vector<RecoveredMatrix3x4>studioSkinPaletteCandidate(
    const StudioSkinPaletteMetadataCandidate&metadata,const std::vector<RecoveredMatrix3x4>&world,
    uint32_t mask,std::vector<RecoveredMatrix3x4>output){
    if(metadata.staticModel){if(world.empty()||output.empty())throw std::runtime_error("static studio skin slot");checkIfmMatrixFiniteCandidate(world[0]);output[0]=world[0];return output;}
    if(metadata.flags.size()!=metadata.inverseBind.size()||metadata.flags.size()>256||world.size()<metadata.flags.size()||output.size()<metadata.flags.size())throw std::runtime_error("studio skin palette span");
    for(size_t i=0;i<metadata.flags.size();++i)if(metadata.flags[i]&mask){checkIfmMatrixFiniteCandidate(world[i]);checkIfmMatrixFiniteCandidate(metadata.inverseBind[i]);output[i]=composeRecoveredTransforms(world[i],metadata.inverseBind[i]);checkIfmMatrixFiniteCandidate(output[i]);}
    return output;
}
