#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
struct SfmVtxMeshCandidate {uint32_t body=0,model=0,lod=0,mesh=0;std::vector<std::array<uint32_t,3>>triangles;};
// Explicit SFM extended VTX7 branch: 33-byte groups, 35-byte strips.
// Triangle-list strips only. IDs remain local to their MDL mesh.
inline std::vector<SfmVtxMeshCandidate>readSfmVtxGeometryCandidate(const std::vector<uint8_t>&bytes,uint32_t checksum){
 auto range=[&](uint64_t o,uint64_t n){if(o>bytes.size()||n>bytes.size()-o)throw std::runtime_error("VTX span");};
 auto word=[&](size_t o){range(o,4);uint32_t v;std::memcpy(&v,bytes.data()+o,4);return v;};
 auto nonnegative=[&](size_t o){auto v=word(o);if(v>INT32_MAX)throw std::runtime_error("negative VTX field");return v;};
 auto shortWord=[&](size_t o){range(o,2);uint16_t v;std::memcpy(&v,bytes.data()+o,2);return v;};
 auto table=[&](size_t base,size_t co,size_t oo,size_t stride){auto count=nonnegative(base+co);int32_t relative;auto raw=word(base+oo);std::memcpy(&relative,&raw,4);int64_t start=int64_t(base)+relative;if(start<0)throw std::runtime_error("VTX relative span");range(uint64_t(start),uint64_t(count)*stride);return std::pair<size_t,uint32_t>{size_t(start),count};};
 range(0,36);if(word(0)!=7||word(16)!=checksum)throw std::runtime_error("VTX version/checksum");auto lodCount=nonnegative(20);if(!lodCount||lodCount>8)throw std::runtime_error("VTX LOD count");std::vector<SfmVtxMeshCandidate>result;auto bodies=table(0,28,32,8);
 for(uint32_t bi=0;bi<bodies.second;++bi){size_t body=bodies.first+size_t(bi)*8;auto models=table(body,0,4,8);for(uint32_t mi=0;mi<models.second;++mi){size_t model=models.first+size_t(mi)*8;auto lods=table(model,0,4,12);if(lods.second!=lodCount)throw std::runtime_error("VTX model LOD count");for(uint32_t li=0;li<lods.second;++li){size_t lod=lods.first+size_t(li)*12;auto meshes=table(lod,0,4,9);for(uint32_t mei=0;mei<meshes.second;++mei){size_t mesh=meshes.first+size_t(mei)*9;SfmVtxMeshCandidate output;output.body=bi;output.model=mi;output.lod=li;output.mesh=mei;auto groups=table(mesh,0,4,33);
  for(uint32_t gi=0;gi<groups.second;++gi){size_t group=groups.first+size_t(gi)*33;auto topology=table(group,25,29,2);auto vertices=table(group,0,4,9),indices=table(group,8,12,2),strips=table(group,16,20,35);for(uint32_t n=0;n<indices.second;++n)if(shortWord(indices.first+size_t(n)*2)>=vertices.second)throw std::runtime_error("VTX group index");
   for(uint32_t si=0;si<strips.second;++si){size_t strip=strips.first+size_t(si)*35;auto count=nonnegative(strip),first=nonnegative(strip+4),vertexCount=nonnegative(strip+8),vertexFirst=nonnegative(strip+12);if(first>indices.second||count>indices.second-first||vertexFirst>vertices.second||vertexCount>vertices.second-vertexFirst)throw std::runtime_error("VTX strip range");table(strip,19,23,8);auto topologyCount=nonnegative(strip+27),topologyFirst=nonnegative(strip+31);if(topologyFirst>topology.second||topologyCount>topology.second-topologyFirst)throw std::runtime_error("VTX topology range");if(bytes[strip+18]!=1||count%3)throw std::runtime_error("VTX triangle list required");
    for(uint32_t n=0;n<count;n+=3){std::array<uint32_t,3>triangle{};for(size_t k=0;k<3;++k){auto index=shortWord(indices.first+(size_t(first)+n+k)*2);triangle[k]=shortWord(vertices.first+size_t(index)*9+4);}output.triangles.push_back(triangle);}
   }
  }result.push_back(std::move(output));
 }}}}return result;
}
