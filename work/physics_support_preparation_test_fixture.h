#pragma once
#include "source_physics_support_preparation_candidate.h"
#include "physics_support_test_fixture.h"
struct PhysicsSupportPreparationInputFixture {
    SourcePhysicsSupportMetadataCandidate metadata;
    bool blocked=false;
    std::vector<std::array<float,4>>points;
    std::array<float,12>matrix{};
    SourcePhysicsPackedSupportCandidate cache;
    std::array<float,3>direction{1,2,-3};
};
struct PhysicsSupportPreparationOutputFixture {
    bool built=false;
    SourcePhysicsPackedSupportCandidate cache;
    SourcePhysicsSupportResultCandidate selected;
};
inline void preparationWriteInput(std::ostream&out,const PhysicsSupportPreparationInputFixture&f){
    std::ostringstream payload(std::ios::binary);
    for(uint32_t value:{uint32_t(f.metadata.present),uint32_t(f.blocked),uint32_t(f.metadata.flags),uint32_t(f.metadata.count),uint32_t(f.metadata.firstVertex),uint32_t(f.metadata.runs.size()),uint32_t(f.points.size()),f.cache.groups,uint32_t(f.cache.storage.size())})supportPut(payload,value);
    supportPut(payload,f.matrix);supportPut(payload,f.direction);for(uint8_t value:f.metadata.runs)supportPut(payload,value);for(const auto&p:f.points)supportPut(payload,p);for(const auto&p:f.cache.storage)supportPut(payload,p);
    const std::string bytes=payload.str();supportPut(out,uint32_t(bytes.size()));out.write(bytes.data(),bytes.size());
}
inline bool preparationReadInput(std::istream&in,PhysicsSupportPreparationInputFixture&f){
    uint32_t size=0;if(!in.read(reinterpret_cast<char*>(&size),4)){if(in.eof()&&!in.gcount())return false;throw std::runtime_error("truncated support preparation length");}if(size<96||size>4*1024*1024)throw std::runtime_error("support preparation extent");std::string bytes(size,'\0');if(!in.read(bytes.data(),size))throw std::runtime_error("truncated support preparation input");std::istringstream p(bytes,std::ios::binary);std::array<uint32_t,9>header;supportGet(p,header);
    if(header[0]>1||header[1]>1||header[2]>255||header[3]>32768||header[4]>32767||header[5]>14||header[6]>32768||header[8]>32768||header[8]%4||header[7]>header[8]/4)throw std::runtime_error("invalid support preparation header");f={};f.metadata.present=header[0]!=0;f.blocked=header[1]!=0;f.metadata.flags=uint8_t(header[2]);f.metadata.count=uint16_t(header[3]);f.metadata.firstVertex=uint16_t(header[4]);f.metadata.runs.resize(header[5]);f.points.resize(header[6]);f.cache.groups=header[7];f.cache.storage.resize(header[8]);supportGet(p,f.matrix);supportGet(p,f.direction);for(auto&v:f.metadata.runs)supportGet(p,v);for(auto&v:f.points)supportGet(p,v);for(auto&v:f.cache.storage)supportGet(p,v);if(p.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing support preparation data");return true;
}
inline void preparationWriteOutput(std::ostream&out,const PhysicsSupportPreparationOutputFixture&f){
    supportPut(out,uint32_t(24+f.cache.storage.size()*12));supportPut(out,uint32_t(f.built));supportPut(out,f.cache.groups);for(const auto&p:f.cache.storage)supportPut(out,p);supportPut(out,f.selected.feature);supportPut(out,f.selected.point);
}
inline PhysicsSupportPreparationOutputFixture preparationInvokeOwned(const PhysicsSupportPreparationInputFixture&f){
    PhysicsSupportPreparationOutputFixture result;result.cache=f.cache;result.selected.feature=UINT32_MAX;result.selected.point={123456,123456,123456};result.built=sourcePhysicsPreparePackedSupportCandidate(f.metadata,f.blocked,f.points,f.matrix,result.cache);
    if(result.built){SourcePhysicsSupportContextCandidate c;c.mode=2;c.points.assign(result.cache.storage.begin(),result.cache.storage.begin()+result.cache.groups*4);SourcePhysicsSupportCacheCandidate unused;result.selected=sourcePhysicsSupportCandidate(c,f.direction,unused);}return result;
}
