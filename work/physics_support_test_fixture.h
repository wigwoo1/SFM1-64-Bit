#pragma once
#include "source_physics_support_candidate.h"
#include <cstring>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
struct PhysicsSupportInputFixture {
    SourcePhysicsSupportContextCandidate context;
    SourcePhysicsSupportCacheCandidate cache;
    std::array<float,3>direction{};
};
template<class T>inline void supportPut(std::ostream&out,const T&value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
template<class T>inline void supportGet(std::istream&in,T&value){if(!in.read(reinterpret_cast<char*>(&value),sizeof(value)))throw std::runtime_error("truncated support fixture");}
inline void supportWriteInput(std::ostream&out,const PhysicsSupportInputFixture&f){
    std::ostringstream payload(std::ios::binary);const auto&c=f.context;
    for(uint32_t value:{c.mode,uint32_t(c.points.size()),uint32_t(c.triangles.size()),c.firstVertex,uint32_t(c.seeded),0u})supportPut(payload,value);
    supportPut(payload,c.inverseBasis);supportPut(payload,c.transform);supportPut(payload,f.direction);supportPut(payload,c.seeds);
    if(c.mode==0){supportPut(payload,f.cache.entries);supportPut(payload,f.cache.generation);}
    for(const auto&p:c.points)supportPut(payload,p);for(const auto&t:c.triangles)supportPut(payload,t);
    const std::string bytes=payload.str();supportPut(out,uint32_t(bytes.size()));out.write(bytes.data(),bytes.size());
}
inline bool supportReadInput(std::istream&in,PhysicsSupportInputFixture&f){
    uint32_t length=0;if(!in.read(reinterpret_cast<char*>(&length),4)){if(in.eof()&&!in.gcount())return false;throw std::runtime_error("truncated support extent");}
    if(length<136||length>4*1024*1024)throw std::runtime_error("support fixture extent");std::string bytes(length,'\0');if(!in.read(bytes.data(),length))throw std::runtime_error("truncated support payload");std::istringstream payload(bytes,std::ios::binary);
    f={};auto&c=f.context;uint32_t points,triangles,seeded,reserved;supportGet(payload,c.mode);supportGet(payload,points);supportGet(payload,triangles);supportGet(payload,c.firstVertex);supportGet(payload,seeded);supportGet(payload,reserved);
    if(c.mode>2||!points||points>32768||triangles>32767||seeded>1||reserved||((c.mode!=0)&&triangles)||((c.mode==0)&&!triangles)||((c.mode==2)&&points%4))throw std::runtime_error("invalid support fixture header");c.seeded=seeded!=0;
    supportGet(payload,c.inverseBasis);supportGet(payload,c.transform);supportGet(payload,f.direction);supportGet(payload,c.seeds);
    if(c.mode==0){supportGet(payload,f.cache.entries);supportGet(payload,f.cache.generation);}c.points.resize(points);c.triangles.resize(triangles);for(auto&p:c.points)supportGet(payload,p);for(auto&t:c.triangles)supportGet(payload,t);
    if(payload.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing support payload");
    for(float x:f.direction)if(!std::isfinite(x))throw std::runtime_error("nonfinite support fixture direction");
    for(const auto&p:c.points)for(float x:p)if(!std::isfinite(x))throw std::runtime_error("nonfinite support fixture point");
    if(c.mode!=2){for(float x:c.inverseBasis)if(!std::isfinite(x))throw std::runtime_error("nonfinite support fixture rotation");for(float x:c.transform)if(!std::isfinite(x))throw std::runtime_error("nonfinite support fixture transform");}
    return true;
}
inline void supportWriteOutput(std::ostream&out,const SourcePhysicsSupportResultCandidate&r,const SourcePhysicsSupportCacheCandidate&cache,uint32_t mode){
    supportPut(out,uint32_t(16+(mode==0?2050:0)));supportPut(out,r.feature);supportPut(out,r.point);if(mode==0){supportPut(out,cache.entries);supportPut(out,cache.generation);}
}
