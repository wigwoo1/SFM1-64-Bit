#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
struct VvdGeometryCandidate {uint32_t checksum=0,lod=0;std::vector<std::array<uint8_t,48>>vertices;std::vector<std::array<uint8_t,16>>tangents;};
inline VvdGeometryCandidate readVvdGeometryCandidate(const std::vector<uint8_t>&bytes,uint32_t expectedChecksum,uint32_t lod){
 auto range=[&](uint64_t offset,uint64_t size){if(offset>bytes.size()||size>bytes.size()-offset)throw std::runtime_error("VVD byte range");};
 auto word=[&](size_t offset){range(offset,4);uint32_t v;std::memcpy(&v,bytes.data()+offset,4);return v;};
 auto positive=[&](size_t offset){auto v=word(offset);if(v>INT32_MAX)throw std::runtime_error("negative VVD field");return v;};
 range(0,64);if(word(0)!=0x56534449||word(4)!=4)throw std::runtime_error("unsupported VVD encoding");if(word(8)!=expectedChecksum)throw std::runtime_error("VVD checksum mismatch");auto lods=positive(12);if(!lods||lods>8||lod>=lods)throw std::runtime_error("VVD LOD");
 std::array<uint32_t,8>counts{};for(uint32_t i=0;i<lods;++i){counts[i]=positive(16+i*4);if(i&&counts[i]>counts[i-1])throw std::runtime_error("VVD LOD counts");}
 auto fixups=positive(48),fixupStart=positive(52),vertexStart=positive(56),tangentStart=positive(60);if(vertexStart<64||(fixups&&fixupStart<64)||(tangentStart&&tangentStart<64))throw std::runtime_error("VVD data offset");range(vertexStart,uint64_t(counts[0])*48);if(tangentStart)range(tangentStart,uint64_t(counts[0])*16);if(fixups)range(fixupStart,uint64_t(fixups)*12);
 std::vector<uint32_t>indices;indices.reserve(counts[lod]);if(!fixups){for(uint32_t i=0;i<counts[lod];++i)indices.push_back(i);}else for(uint32_t i=0;i<fixups;++i){size_t offset=fixupStart+size_t(i)*12;auto level=positive(offset),source=positive(offset+4),count=positive(offset+8);if(level>=lods||source>counts[0]||count>counts[0]-source)throw std::runtime_error("VVD fixup");if(level<lod)continue;if(indices.size()>counts[lod]||count>counts[lod]-indices.size())throw std::runtime_error("VVD fixup count");for(uint32_t j=0;j<count;++j)indices.push_back(source+j);}
 if(indices.size()!=counts[lod])throw std::runtime_error("VVD incomplete fixups");VvdGeometryCandidate result;result.checksum=expectedChecksum;result.lod=lod;result.vertices.resize(indices.size());if(tangentStart)result.tangents.resize(indices.size());for(size_t i=0;i<indices.size();++i){std::memcpy(result.vertices[i].data(),bytes.data()+vertexStart+size_t(indices[i])*48,48);if(tangentStart)std::memcpy(result.tangents[i].data(),bytes.data()+tangentStart+size_t(indices[i])*16,16);}return result;
}
