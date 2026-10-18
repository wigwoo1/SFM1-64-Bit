#pragma once
#include "source_collision_surface_candidate.h"
#include <cstring>
#include <stdexcept>
#include <vector>
// Owns low16 material flags from checked BSP texinfo and a caller-provided
// texture-info -> runtime-surface binding. Shared surfaces OR texinfo flags,
// matching original1014AC60. Does not load VMT surface properties,
// infer runtime name-pointer identity or build the engine material registry.
class SourceBspTraceSurfaceFlagsCandidate {
    std::vector<uint16_t>runtimeFlags;
    std::vector<bool>assigned;
    std::vector<uint32_t>mapping;
public:
    std::vector<uint16_t>textureFlags;
    static constexpr uint32_t defaultId=UINT32_MAX,displacementId=UINT32_MAX-1;
    explicit SourceBspTraceSurfaceFlagsCandidate(const std::vector<uint8_t>&b,const std::vector<uint32_t>&textureToSurface):mapping(textureToSurface){
        if(b.size()<1036||std::memcmp(b.data(),"VBSP",4))throw std::runtime_error("surface flags BSP header");
        auto u32=[&](size_t at){uint32_t v;std::memcpy(&v,b.data()+at,4);return v;};
        const size_t header=8+6*16;const uint32_t offset=u32(header),length=u32(header+4);
        if(offset>b.size()||length>b.size()-offset||(length&&offset<1036)||length%72||u32(header+8)||u32(header+12)||mapping.size()!=length/72)throw std::runtime_error("surface flags texinfo lump/binding");
        assigned.resize(65535);runtimeFlags.resize(65535);
        for(size_t i=0;i<mapping.size();++i){const uint32_t id=mapping[i];if(id>65535)throw std::runtime_error("runtime surface id outside16bits");const uint16_t flags=uint16_t(u32(offset+i*72+64));textureFlags.push_back(flags);if(id==65535)continue;assigned[id]=true;runtimeFlags[id]|=flags;}
    }
    uint32_t forTexture(int32_t texture)const{if(texture<0)return 65535;if(size_t(texture)>=mapping.size())throw std::runtime_error("surface texinfo outside binding");return mapping[size_t(texture)];}
    uint32_t identity(const SourceCollisionSurfaceCandidate&s)const{
        switch(s.kind){
        case SourceCollisionSurfaceKindCandidate::Default:return defaultId;
        case SourceCollisionSurfaceKindCandidate::Displacement:return displacementId;
        case SourceCollisionSurfaceKindCandidate::TextureInfo:{if(s.index>INT32_MAX)throw std::runtime_error("surface texinfo index overflow");const auto id=forTexture(int32_t(s.index));return id==65535?defaultId:id;}
        case SourceCollisionSurfaceKindCandidate::RuntimeSurface:if(s.index>=65535||!assigned[s.index])throw std::runtime_error("surface runtime index unbound");return s.index;
        default:throw std::runtime_error("unresolved box-face surface");
        }
    }
    uint16_t flags(const SourceCollisionSurfaceCandidate&s)const{const auto id=identity(s);return id==defaultId||id==displacementId?0:runtimeFlags.at(id);}
};
