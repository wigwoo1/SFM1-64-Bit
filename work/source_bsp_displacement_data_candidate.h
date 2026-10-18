#pragma once
#include "source_bsp_world_collision_data_candidate.h"
struct SourceBspDisplacementRecordCandidate {
 std::array<uint8_t,176>raw{};std::array<float,3>start{};
 uint32_t vertexStart=0,triangleStart=0,contents=0,vertexCount=0,triangleCount=0;
 int32_t power=0;uint16_t mapFace=0;
};
// Checked source data only; not a native collision mesh or runtime leaf map.
class SourceBspDisplacementDataCandidate {
public:
 std::vector<SourceBspDisplacementRecordCandidate>records;
 std::vector<std::array<uint8_t,20>>vertices;
 std::vector<uint16_t>triangleTags;
 explicit SourceBspDisplacementDataCandidate(const std::vector<uint8_t>&b){
  const SourceBspWorldCollisionDataCandidate checked(b);
  auto word=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};auto scalar=[&](size_t p){float v;std::memcpy(&v,b.data()+p,4);if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement data");return v;};
  auto lump=[&](unsigned id,size_t stride){size_t p=8+16*id;size_t offset=word(p),length=word(p+4);if(offset>INT32_MAX||length>INT32_MAX||offset>b.size()||length>b.size()-offset||(length&&offset<1036)||length%stride||word(p+8)||word(p+12))throw std::runtime_error("invalid displacement data lump");return std::array<size_t,2>{offset,length/stride};};
  const auto info=lump(26,176),verts=lump(33,20),tris=lump(48,2);
  for(size_t i=0;i<verts[1];++i){size_t p=verts[0]+20*i;for(size_t j=0;j<5;++j)(void)scalar(p+4*j);std::array<uint8_t,20>v;std::memcpy(v.data(),b.data()+p,20);vertices.push_back(v);}
  for(size_t i=0;i<tris[1];++i){uint16_t v;std::memcpy(&v,b.data()+tris[0]+2*i,2);triangleTags.push_back(v);}
  for(size_t i=0;i<info[1];++i){size_t p=info[0]+176*i;SourceBspDisplacementRecordCandidate r;std::memcpy(r.raw.data(),b.data()+p,176);r.start={scalar(p),scalar(p+4),scalar(p+8)};r.vertexStart=word(p+12);r.triangleStart=word(p+16);std::memcpy(&r.power,b.data()+p+20,4);r.contents=word(p+32);std::memcpy(&r.mapFace,b.data()+p+36,2);(void)scalar(p+28);
   // Reader coverage follows the existing geometry reader; native collision
   // power support must be verified separately before building runtime meshes.
   if(r.power<1||r.power>4)throw std::runtime_error("unsupported displacement power");uint32_t width=(1u<<r.power)+1;r.vertexCount=width*width;r.triangleCount=2*(width-1)*(width-1);
   if(r.vertexStart>vertices.size()||r.vertexCount>vertices.size()-r.vertexStart||r.triangleStart>triangleTags.size()||r.triangleCount>triangleTags.size()-r.triangleStart)throw std::runtime_error("displacement span outside data");records.push_back(r);
  }
 }
};
