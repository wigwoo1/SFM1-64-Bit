#pragma once
#include "mdl_flex_vertex_candidate.h"
#include "mdl_flex_metadata.h"
struct MdlMeshFlexCandidate {uint32_t body,model,mesh,firstVertex,descriptor,pair;std::array<float,4>targets;std::vector<MdlFlexVertexCandidate>vertices;};
inline std::vector<MdlMeshFlexCandidate>readMdl49HalfMeshFlexesCandidate(const std::vector<uint8_t>&bytes,size_t vvdVertices){
 readMdl49FlexMetadata(bytes);
 auto span=[&](uint64_t p,uint64_t n){if(p>bytes.size()||n>bytes.size()-p)throw std::runtime_error("mesh flex span");};
 auto word=[&](size_t p){span(p,4);uint32_t v;std::memcpy(&v,bytes.data()+p,4);return v;};
 auto positive=[&](size_t p){auto v=word(p);if(v>INT32_MAX)throw std::runtime_error("negative mesh flex field");return v;};
 auto relative=[&](size_t base,size_t field){auto raw=word(base+field);int32_t delta;std::memcpy(&delta,&raw,4);int64_t p=int64_t(base)+delta;if(p<0)throw std::runtime_error("mesh flex relative offset");return size_t(p);};
 auto table=[&](size_t base,size_t countField,size_t offsetField,size_t stride){auto count=positive(base+countField);auto first=relative(base,offsetField);span(first,uint64_t(count)*stride);return std::make_pair(first,count);};
 if(word(152)&0x4000)throw std::runtime_error("converted flex encoding unsupported");auto descriptors=positive(260);std::vector<MdlMeshFlexCandidate>result;auto bodies=table(0,232,236,16);
 for(uint32_t bi=0;bi<bodies.second;++bi){size_t body=bodies.first+bi*16;auto models=table(body,4,12,148);for(uint32_t mi=0;mi<models.second;++mi){size_t model=models.first+size_t(mi)*148;auto vertexCount=positive(model+80),vertexBytes=positive(model+84);if(vertexBytes%48)throw std::runtime_error("flex model vertex alignment");auto modelFirst=vertexBytes/48;if(modelFirst>vvdVertices||vertexCount>vvdVertices-modelFirst)throw std::runtime_error("flex model VVD span");auto meshes=table(model,72,76,116);
  for(uint32_t mei=0;mei<meshes.second;++mei){size_t mesh=meshes.first+size_t(mei)*116;auto count=positive(mesh+8),first=positive(mesh+12);if(first>vertexCount||count>vertexCount-first||relative(mesh,4)!=model)throw std::runtime_error("flex mesh model span");auto flexes=table(mesh,16,20,60);
   for(uint32_t fi=0;fi<flexes.second;++fi){size_t flex=flexes.first+size_t(fi)*60;auto descriptor=positive(flex),pair=positive(flex+28);if(descriptor>=descriptors||pair>=descriptors)throw std::runtime_error("mesh flex descriptor");if(bytes.at(flex+32)!=0)throw std::runtime_error("wrinkle flex encoding unsupported");MdlMeshFlexCandidate value{bi,mi,mei,modelFirst+first,descriptor,pair,{},{}};std::memcpy(value.targets.data(),bytes.data()+flex+4,16);for(float target:value.targets)if(!std::isfinite(target))throw std::runtime_error("nonfinite flex target");auto vertices=table(flex,20,24,16);value.vertices.reserve(vertices.second);for(uint32_t vi=0;vi<vertices.second;++vi)value.vertices.push_back(readMdlFlexHalfVertexCandidate(bytes,vertices.first+size_t(vi)*16,count));result.push_back(std::move(value));}
  }
 }}return result;
}
