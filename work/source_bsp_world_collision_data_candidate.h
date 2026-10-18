#pragma once
#include "source_bsp_point_leaf_candidate.h"
#include <set>
// Checked collision records, not rendered triangles and not a trace algorithm.
struct SourceBspCollisionPlaneCandidate {
 std::array<float,3>normal{};float distance=0;uint8_t type=0,signBits=0;std::array<uint8_t,2>reserved{};
 std::array<uint8_t,4>tags()const{return {type,signBits,reserved[0],reserved[1]};}
};
// Original plane import 1014B1F3..1014B40B after a cleared allocation.
// Negative zero does not contribute a sign bit. Reserved bytes remain zero.
inline SourceBspCollisionPlaneCandidate sourceBspCollisionPlaneCandidate(const std::array<float,3>&normal,float distance,uint32_t type){
 if(type>5||!std::isfinite(distance))throw std::runtime_error("invalid collision plane");
 SourceBspCollisionPlaneCandidate result;result.normal=normal;result.distance=distance;result.type=uint8_t(type);
 for(size_t a=0;a<3;++a){if(!std::isfinite(normal[a]))throw std::runtime_error("invalid collision plane normal");if(normal[a]<0)result.signBits|=uint8_t(1u<<a);}return result;
}
struct SourceBspCollisionSideCandidate {uint16_t plane=0;int16_t texture=0,displacement=-1,bevel=0;};
struct SourceBspCollisionBrushCandidate {uint32_t first=0,count=0,contents=0;};
class SourceBspWorldCollisionDataCandidate {
public:
 std::vector<SourceBspCollisionPlaneCandidate>planes;
 std::vector<SourceBspCollisionSideCandidate>sides;
 std::vector<SourceBspCollisionBrushCandidate>brushes;
 // Trace input retains file order and duplicates; worldBrushes is inventory only.
 std::vector<std::vector<uint32_t>>leafBrushes;
 std::vector<uint32_t>leafContents;
 std::vector<uint32_t>worldBrushes;
 size_t displacementRecords=0,worldNodes=0;
 explicit SourceBspWorldCollisionDataCandidate(const std::vector<uint8_t>&b){
  const SourceBspPointLeafCandidate tree(b); // Validates all spatial references/cycles first.
  auto u32=[&](size_t p){uint32_t v;std::memcpy(&v,b.data()+p,4);return v;};
  auto u16=[&](size_t p){uint16_t v;std::memcpy(&v,b.data()+p,2);return v;};
  auto i16=[&](size_t p){int16_t v;std::memcpy(&v,b.data()+p,2);return v;};
  auto i32=[&](size_t p){int32_t v;std::memcpy(&v,b.data()+p,4);return v;};
  auto scalar=[&](size_t p){float v;std::memcpy(&v,b.data()+p,4);return v;};
  auto lump=[&](unsigned id,size_t stride,uint32_t version){const size_t p=8+id*16;const auto offset=u32(p),length=u32(p+4);if(offset>INT32_MAX||length>INT32_MAX||uint64_t(offset)+length>b.size()||(length&&offset<1036)||length%stride||u32(p+8)!=version||u32(p+12))throw std::runtime_error("invalid collision lump");return std::array<size_t,2>{offset,length/stride};};
  const auto plane=lump(1,20,0),node=lump(5,32,0),leaf=lump(10,32,1),refs=lump(17,2,0),brush=lump(18,12,0),side=lump(19,8,0),disp=lump(26,176,0);displacementRecords=disp[1];
  for(size_t i=0;i<plane[1];++i){size_t p=plane[0]+i*20;planes.push_back(sourceBspCollisionPlaneCandidate({scalar(p),scalar(p+4),scalar(p+8)},scalar(p+12),u32(p+16)));}
  for(size_t i=0;i<side[1];++i){size_t p=side[0]+i*8;SourceBspCollisionSideCandidate s{u16(p),i16(p+2),i16(p+4),i16(p+6)};
   // Preserve historical dispinfo raw: this benchmark stores zero in every
   // side, so it must not be treated as a proven displacement-lump reference.
   if(s.plane>=planes.size()||(s.bevel!=0&&s.bevel!=1))throw std::runtime_error("collision side reference");
   if(planes[s.plane].normal==std::array<float,3>{0,0,0})throw std::runtime_error("zero collision plane");sides.push_back(s);
  }
  for(size_t i=0;i<brush[1];++i){size_t p=brush[0]+i*12;const int32_t first=i32(p),count=i32(p+4);if(first<0||count<0||size_t(first)>sides.size()||size_t(count)>sides.size()-size_t(first))throw std::runtime_error("collision brush span");brushes.push_back({uint32_t(first),uint32_t(count),u32(p+8)});}
  for(size_t i=0;i<refs[1];++i)if(u16(refs[0]+i*2)>=brushes.size())throw std::runtime_error("collision leaf brush reference");
  // Validate every leaf span, including unreachable/inline-model leaves.
  for(size_t i=0;i<leaf[1];++i){const size_t p=leaf[0]+i*32;const size_t first=u16(p+24),count=u16(p+26);if(first>refs[1]||count>refs[1]-first)throw std::runtime_error("collision leaf brush span");leafContents.push_back(u32(p));std::vector<uint32_t>ordered;ordered.reserve(count);for(size_t j=0;j<count;++j)ordered.push_back(u16(refs[0]+2*(first+j)));leafBrushes.push_back(std::move(ordered));}
  std::set<uint32_t>selected;std::vector<bool>seen(node[1]);std::vector<int32_t>pending{tree.worldRoot()};
  while(!pending.empty()){const auto cursor=pending.back();pending.pop_back();if(cursor>=0){size_t i=size_t(cursor);if(seen[i])continue;seen[i]=true;++worldNodes;size_t p=node[0]+i*32;pending.push_back(i32(p+4));pending.push_back(i32(p+8));}
   else{const size_t p=leaf[0]+size_t(-1-int64_t(cursor))*32;const size_t first=u16(p+24),count=u16(p+26);for(size_t i=0;i<count;++i)selected.insert(u16(refs[0]+(first+i)*2));}
  }
  worldBrushes.assign(selected.begin(),selected.end());
 }
};
