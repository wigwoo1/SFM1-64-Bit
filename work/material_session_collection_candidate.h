#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// DmeMaterial cached resource identity. Null name means unavailable; engaged
// empty string represents the original invalid-string sentinel's empty name.
struct MaterialSessionCachedResourceCandidate{uintptr_t resource=0;std::optional<std::string>name;};
struct MaterialSessionResourceServicesCandidate{
 virtual ~MaterialSessionResourceServicesCandidate()=default;
 virtual std::optional<uintptr_t>findMaterial(std::string_view)=0;
 virtual void addReference(uintptr_t)=0;
};
inline std::optional<uintptr_t>acquireMaterialSessionResourceCandidate(MaterialSessionCachedResourceCandidate&binding,MaterialSessionResourceServicesCandidate&service){
 if(binding.resource)return binding.resource;
 if(!binding.name)return uintptr_t(0);
 if(binding.name->size()>4095)return {};
 const auto name=std::string_view(*binding.name).substr(0,binding.name->find('\0'));
 auto result=service.findMaterial(name);if(!result)return {};
 // Original105C1220 ->10984A60 stores then AddRef12; no release occurs on
 // this lazy branch because the previous cached identity was null.
 binding.resource=*result;if(*result)service.addReference(*result);return result;
}
struct MaterialSessionCollectionServicesCandidate{
 virtual ~MaterialSessionCollectionServicesCandidate()=default;
 virtual uintptr_t resolve(uint32_t)=0;
 virtual bool isMaterial(uintptr_t)=0;
 virtual std::optional<uintptr_t>acquire(uintptr_t)=0;
 virtual bool capture(uintptr_t backup,uintptr_t material)=0;
 virtual bool apply(uintptr_t source,uintptr_t material)=0;
};
// Original1023E3E0 order, including repeated handles and shared resources.
// An optional backup is selected BY INDEX, never by name/resource identity.
// Services own/stage all mutable values, references and rollback. This loop is
// not a transaction and does not promise original state after applying backups.
inline bool applyMaterialSessionCollectionCandidate(const std::vector<uint32_t>&source,const std::vector<uint32_t>*backup,MaterialSessionCollectionServicesCandidate&service){
 if(source.size()>4096||(backup&&backup->size()>4096))return false;
 const auto count=source.size(); // Original snapshots the source count on entry.
 for(size_t i=0;i<count;++i){
  const auto element=service.resolve(source[i]);if(!element||!service.isMaterial(element))continue;
  const auto material=service.acquire(element);if(!material)return false;if(!*material)continue;
  if(backup&&i<backup->size()){
   auto target=service.resolve((*backup)[i]);if(target&&!service.isMaterial(target))target=0;
   if(!service.capture(target,*material))return false;
  }
  if(!service.apply(element,*material))return false;
 }
 return true;
}
