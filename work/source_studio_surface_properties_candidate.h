#pragma once
#include "source_studio_physics_contents_candidate.h"
#include <string>
// Original datacache10005870 resolves model + bone surface-name strings through
// VPhysicsSurfaceProps slot3, replacing runtime caches. Never trust disk caches.
// Owned result leaves original MDL bytes untouched; unknown lookup -1 is a
// completed resolution, distinct from an absent/uninitialized provider.
struct SourceStudioSurfacePropertiesCandidate {
    int32_t model=-1;
    std::vector<int32_t>bones;
    uint16_t traceSurfaceProperty()const{return uint16_t(uint32_t(model));}
};
template<class Provider>SourceStudioSurfacePropertiesCandidate sourceStudioResolveSurfacePropertiesCandidate(
    const std::vector<uint8_t>&mdl,const Provider&provider){
    const SourceStudioPhysicsContentsCandidate checked(mdl);(void)checked;
    auto word=[&](size_t p){uint32_t value;std::memcpy(&value,mdl.data()+p,4);return value;};
    const size_t length=word(76);const auto name=[&](size_t base,size_t field){
        const uint32_t relative=word(field);if(relative>length-base)throw std::runtime_error("studio surface name offset outside model");const size_t at=base+relative;if(at>=length)throw std::runtime_error("studio surface name outside model");size_t end=at;while(end<length&&mdl[end])++end;if(end==length||end-at>1023)throw std::runtime_error("studio surface name extent");return std::string(reinterpret_cast<const char*>(mdl.data()+at),end-at);
    };
    SourceStudioSurfacePropertiesCandidate out;out.model=provider.lookup(name(0,308));const size_t count=word(156),offset=word(160);out.bones.reserve(count);
    for(size_t i=0;i<count;++i){const size_t base=offset+i*216;out.bones.push_back(provider.lookup(name(base,base+176)));}
    return out;
}
