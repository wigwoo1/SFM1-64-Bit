#pragma once
#include "source_physics_support_metadata_candidate.h"
#include "physics_support_test_fixture.h"
struct PhysicsSupportMetadataInputFixture {uint32_t ledge=0;std::array<uint8_t,20>initial{};SourcePhyConvexGeometryCandidate convex;};
inline void metadataWriteInput(std::ostream&out,const PhysicsSupportMetadataInputFixture&f){
    std::ostringstream payload(std::ios::binary);supportPut(payload,f.ledge);supportPut(payload,f.initial);supportPut(payload,f.convex.rawLedgeHeader);supportPut(payload,uint32_t(f.convex.rawTriangles.size()));supportPut(payload,uint32_t(f.convex.rawPoints.size()));for(const auto&t:f.convex.rawTriangles)supportPut(payload,t);for(const auto&p:f.convex.rawPoints)supportPut(payload,p);const auto b=payload.str();supportPut(out,uint32_t(b.size()));out.write(b.data(),b.size());
}
inline bool metadataReadInput(std::istream&in,PhysicsSupportMetadataInputFixture&f){
    uint32_t size=0;if(!in.read(reinterpret_cast<char*>(&size),4)){if(in.eof()&&!in.gcount())return false;throw std::runtime_error("truncated metadata extent");}if(size<48||size>4*1024*1024)throw std::runtime_error("metadata extent");std::string b(size,'\0');if(!in.read(b.data(),size))throw std::runtime_error("truncated metadata input");std::istringstream payload(b,std::ios::binary);f={};supportGet(payload,f.ledge);supportGet(payload,f.initial);supportGet(payload,f.convex.rawLedgeHeader);uint32_t triangles,points;supportGet(payload,triangles);supportGet(payload,points);if(!triangles||triangles>32767||points>32768||(f.convex.rawLedgeHeader[3]&0xffff)!=triangles)throw std::runtime_error("metadata fixture shape extent");f.convex.rawTriangles.resize(triangles);f.convex.rawPoints.resize(points);for(auto&t:f.convex.rawTriangles)supportGet(payload,t);for(auto&p:f.convex.rawPoints)supportGet(payload,p);if(payload.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing metadata input");return true;
}
