#pragma once
#include "material_session_override_dispatch_candidate.h"
#include "attribute_mutation_gate.h"

// Value backup into an EXISTING typed template, not a complete material clone.
// Getters/resource-name interning and mutation phase are explicit host services.
struct MaterialSessionBackupServicesCandidate {
 virtual ~MaterialSessionBackupServicesCandidate()=default;
 virtual MaterialSessionOverrideLookupCandidate find(std::string_view)=0;
 virtual uint32_t integer(uintptr_t)=0;
 virtual uint32_t scalar(uintptr_t)=0;
 virtual std::array<uint32_t,4>vector(uintptr_t,uint32_t)=0;
 virtual std::array<uint32_t,16>matrix(uintptr_t)=0;
 // IMaterialVar slot6 returns string representation, NOT GetTexture(slot15).
 // Nullopt means the string service is unavailable; never invent an empty name.
 virtual std::optional<std::string>stringValue(uintptr_t)=0;
};
inline bool materialSessionBackupEqualFloatCandidate(uint32_t a,uint32_t b){
 if((a&0x7fffffffu)>0x7f800000u||(b&0x7fffffffu)>0x7f800000u)return false;
 if(!(a&0x7fffffffu)&&!(b&0x7fffffffu))return true;return a==b;
}
inline uint32_t materialSessionBackupQuietCandidate(uint32_t word){return (word&0x7fffffffu)>0x7f800000u?word|0x400000u:word;}
// Atomic value-output publication. Caller still owns model/draw lifetime,
// material variable ownership, resource retention, native undo and notifications.
// Resource-name equality assumes explicit canonical interning in the provider;
// broader original symbol-pool identity remains outside this value contract.
inline bool captureMaterialSessionBackupCandidate(std::vector<MaterialSessionOverrideInputCandidate>&rows,MaterialSessionBackupServicesCandidate&service,int phase){
 if(rows.size()>4096)return false;for(const auto&r:rows)if(r.name.size()>4095||(r.textureName&&r.textureName->size()>4095))return false;
 auto plan=rows;
 auto equal=[](std::string_view a,std::string_view b){if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i){char c=a[i];if(c>='A'&&c<='Z')c=char(c+32);if(c!=b[i])return false;}return true;};
 for(auto&r:plan){
  auto name=std::string_view(r.name);name=name.substr(0,name.find('\0'));
  if(!r.nameAvailable||name.empty()||name.front()!='$'||equal(name,"$flags")||equal(name,"$flags2"))continue;
  const auto found=service.find(name);if(!found.variable||!found.found)continue;
  const auto type=r.typeFlags&31u;auto next=r;bool different=false;unsigned count=0;
  switch(type){
  case 2:next.words[0]=service.integer(found.variable);different=next.words[0]!=r.words[0];break;
  case 3:next.words[0]=materialSessionBackupQuietCandidate(service.scalar(found.variable));different=!materialSessionBackupEqualFloatCandidate(next.words[0],r.words[0]);break;
  case 4:next.words[0]=(r.words[0]&0xffffff00u)|(service.integer(found.variable)!=0);different=next.words[0]!=r.words[0];break;
  case 5:{auto text=service.stringValue(found.variable);if(!text)return false;if(text->size()>4095)return false;text->resize(text->find('\0')==std::string::npos?text->size():text->find('\0'));next.textureName=std::move(text);auto before=r.textureName;if(before)before->resize(before->find('\0')==std::string::npos?before->size():before->find('\0'));different=next.textureName!=before;break;}
  case 8:{
   const auto v=service.vector(found.variable,3);uint32_t packed=0;
   for(unsigned i=0;i<3;++i){float value;std::memcpy(&value,&v[i],4);const auto scaled=_mm_mul_ss(_mm_set_ss(value),_mm_set_ss(255.f));packed|=(uint32_t(_mm_cvttss_si32(scaled))&255u)<<(8*i);}next.words[0]=packed;different=packed!=r.words[0];break;
  }
  case 9:case 10:case 11:{count=type==9?2:type==10?3:4;const auto v=service.vector(found.variable,count);for(unsigned i=0;i<count;++i){different|=!materialSessionBackupEqualFloatCandidate(r.words[i],v[i]);next.words[i]=materialSessionBackupQuietCandidate(v[i]);}break;}
  // Matrix equality is bytewise (104968E0), unlike scalar/vector float equality.
  case 14:{const auto v=service.matrix(found.variable);count=16;for(unsigned i=0;i<count;++i){different|=r.words[i]!=v[i];next.words[i]=materialSessionBackupQuietCandidate(v[i]);}break;}
  default:continue;
  }
  if(different&&attributeMutationPermitted(uint16_t(r.typeFlags),phase)){next.typeFlags|=0x4080u;r=std::move(next);}
 }
 rows.swap(plan);return true;
}
