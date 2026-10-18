#pragma once
#include "source_bsp_trace_surface_flags_candidate.h"
#include "source_surface_name_table_candidate.h"
#include "source_displacement_face_input_candidate.h"
#include "source_displacement_load_policy_candidate.h"
#include <optional>
#include <string>
// Effective IMaterial parameter snapshot, NOT raw VMT declarations. The caller
// must resolve shader defaults, patches/includes and mount precedence first.
// nullopt is an absent parameter; an explicit empty name is a real lookup.
struct SourceBspCollisionMaterialParametersCandidate {
 std::string name;bool available=true,error=false;
 std::optional<std::string>primary,secondary;
};
struct SourceBspCollisionSurfaceEntryCandidate {
 std::string name;uint16_t property=0,flags=0;
};
// Owned immutable material-indexed collision namespace. Texdata order IS the
// native surface identity; it is distinct from texinfo order and physics IDs.
// Checked unsupported/malformed input throws before an instance is published.
class SourceBspCollisionSurfaceTableCandidate {
 std::vector<SourceBspCollisionSurfaceEntryCandidate>entries_;
 std::vector<uint32_t>mapping_;
 std::vector<std::array<uint16_t,2>>pairs_;
public:
 SourceBspCollisionSurfaceTableCandidate(const std::vector<uint8_t>&b,
  const std::vector<SourceBspCollisionMaterialParametersCandidate>&materials,
  const SourceSurfaceNameTableCandidate&properties,int32_t hdrType=0){
  if(b.size()<1036||std::memcmp(b.data(),"VBSP",4))throw std::runtime_error("surface table BSP header");
  auto u32=[&](size_t p){uint32_t n;std::memcpy(&n,b.data()+p,4);return n;};auto i32=[&](size_t p){int32_t n;std::memcpy(&n,b.data()+p,4);return n;};
  if(u32(4)!=20)throw std::runtime_error("surface table BSP version");
  auto lump=[&](unsigned id,size_t stride,uint32_t maxVersion=0){size_t h=8+16*id;uint32_t o=u32(h),n=u32(h+4);if(o>b.size()||n>b.size()-o||(n&&o<1036)||n%stride||n>INT32_MAX||u32(h+8)>maxVersion||u32(h+12))throw std::runtime_error("surface table lump span/version");return std::array<size_t,2>{o,n/stride};};
  const auto td=lump(2,32),ti=lump(6,72),strings=lump(43,1),table=lump(44,4),di=lump(26,176);
  if(td[1]<1||td[1]>2048||ti[1]>65535||strings[1]>16u*1024u*1024u||di[1]>65535||materials.size()!=td[1])throw std::runtime_error("surface table count/binding");
  std::vector<std::array<uint16_t,2>>materialPairs;materialPairs.reserve(td[1]);
  for(size_t i=0;i<td[1];++i){const auto id=i32(td[0]+32*i+12);if(id<0||size_t(id)>=table[1])throw std::runtime_error("surface table string ID");const auto relative=i32(table[0]+4*size_t(id));if(relative<0||size_t(relative)>=strings[1])throw std::runtime_error("surface table name offset");size_t p=strings[0]+size_t(relative),end=p;while(end<strings[0]+strings[1]&&b[end]){if(b[end]<32||b[end]>126)throw std::runtime_error("surface table name encoding");++end;}if(end==p||end==strings[0]+strings[1])throw std::runtime_error("surface table name extent");std::string name(reinterpret_cast<const char*>(b.data()+p),end-p);const auto&m=materials[i];if(m.name!=name)throw std::runtime_error("surface table material identity mismatch");
   uint16_t first=0,second=0;if(m.available&&!m.error){if(m.primary)first=uint16_t(properties.lookup(*m.primary));second=first;if(m.secondary)second=uint16_t(properties.lookup(*m.secondary));}entries_.push_back({std::move(name),first,0});materialPairs.push_back({first,second});
  }
  for(size_t i=0;i<ti[1];++i){int32_t id=i32(ti[0]+72*i+68);if(id<0||size_t(id)>=entries_.size())throw std::runtime_error("surface table texdata reference");mapping_.push_back(uint32_t(id));entries_[size_t(id)].flags|=uint16_t(u32(ti[0]+72*i+64));}
  const auto flags=SourceBspTraceSurfaceFlagsCandidate(b,mapping_);for(size_t i=0;i<entries_.size();++i){bool used=false;for(auto id:mapping_)if(id==i){used=true;break;}if(used&&flags.flags({SourceCollisionSurfaceKindCandidate::RuntimeSurface,uint32_t(i)})!=entries_[i].flags)throw std::runtime_error("surface flag binding mismatch");}
  const auto fi=lump(sourceDisplacementFaceLumpCandidate(hdrType,i32(8+58*16+4)),56,1);std::vector<SourceDisplacementFaceInputCandidate>faces(fi[1]);if(!faces.empty())std::memcpy(faces.data(),b.data()+fi[0],faces.size()*56);const auto faceMap=sourceDisplacementFaceMapCandidate(faces,uint32_t(di[1]));
  for(auto face:faceMap){if(face==65535||face>=faces.size())throw std::runtime_error("inactive/wrapped surface displacement face unsupported");int16_t texture;std::memcpy(&texture,faces[face].data()+10,2);if(texture<0||size_t(texture)>=mapping_.size())throw std::runtime_error("surface displacement texinfo reference");pairs_.push_back(materialPairs.at(mapping_[size_t(texture)]));}
 }
 const auto&entries()const{return entries_;}
 const auto&textureToSurface()const{return mapping_;}
 const auto&displacementProperties()const{return pairs_;}
};
