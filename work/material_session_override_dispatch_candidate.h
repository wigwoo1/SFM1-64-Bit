#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cstring>
#include <xmmintrin.h>

// Ordered, typed per-draw attributes, NOT KeyValues/InitParams input. Resource
// tokens are caller-owned identities; this layer never fabricates a texture.
struct MaterialSessionOverrideInputCandidate {
 std::string name;bool nameAvailable=true;
 uint32_t typeFlags=0;
 std::array<uint32_t,16>words{};
 std::optional<std::string>textureName=std::string();
};
struct MaterialSessionOverrideLookupCandidate {uintptr_t variable=0;bool found=false;};
struct MaterialSessionOverrideServicesCandidate {
 virtual ~MaterialSessionOverrideServicesCandidate()=default;
 virtual MaterialSessionOverrideLookupCandidate find(std::string_view)=0;
 virtual void integer(uintptr_t,uint32_t)=0;
 virtual void scalar(uintptr_t,uint32_t)=0;
 virtual void vector(uintptr_t,const std::array<uint32_t,4>&,uint32_t)=0;
 virtual void matrix(uintptr_t,const std::array<uint32_t,16>&)=0;
 // Nullopt means unavailable service; engaged zero is the actual null return.
 virtual std::optional<uintptr_t>texture(const std::optional<std::string>&)=0;
 virtual void setTexture(uintptr_t,uintptr_t)=0;
};
// Caller supplies staged setters/commit ownership. This dispatch does not own a
// material, initialize proxies, publish a frame, or restore after a draw.
inline bool dispatchMaterialSessionOverridesCandidate(const std::vector<MaterialSessionOverrideInputCandidate>&rows,MaterialSessionOverrideServicesCandidate&service){
 if(rows.size()>4096)return false;
 for(const auto&r:rows)if(r.name.size()>4095||(r.textureName&&r.textureName->size()>4095))return false;
 auto equal=[](std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i){char c=a[i];if(c>='A'&&c<='Z')c=char(c+32);if(c!=b[i])return false;}return true;};
 for(const auto&r:rows){
  auto name=std::string_view(r.name);name=name.substr(0,name.find('\0'));
  if(!r.nameAvailable||name.empty()||name.front()!='$'||equal(name,"$flags")||equal(name,"$flags2"))continue;
  const auto target=service.find(name);if(!target.variable||!target.found)continue;
  const uint32_t type=r.typeFlags&31u;
  std::array<uint32_t,4>v{};std::copy_n(r.words.begin(),4,v.begin());
  switch(type){
  case 2:service.integer(target.variable,v[0]);break;
  case 3:service.scalar(target.variable,v[0]);break;
  case 4:service.integer(target.variable,(v[0]&255u)!=0);break;
  case 5:{auto text=r.textureName;if(text)text->resize(text->find('\0')==std::string::npos?text->size():text->find('\0'));auto texture=service.texture(text);if(!texture)return false;service.setTexture(target.variable,*texture);break;}
  case 8:{
   // Original color path ignores its alpha byte and uses three MULSS with the
   // installed 1/255 float constant. Integer/float DMX values use other paths.
   const auto packed=v[0];for(unsigned i=0;i<3;++i){auto f=_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),int((packed>>(8*i))&255u)),_mm_set_ss(0x1.010102p-8f));float value=_mm_cvtss_f32(f);std::memcpy(&v[i],&value,4);}v[3]=0;service.vector(target.variable,v,3);break;
  }
  case 9:v[2]=v[3]=0;service.vector(target.variable,v,2);break;
  case 10:v[3]=0;service.vector(target.variable,v,3);break;
  case 11:service.vector(target.variable,v,4);break;
  case 14:service.matrix(target.variable,r.words);break;
  default:break; // Lookup still occurred for ignored types, including arrays.
  }
 }
 return true;
}
