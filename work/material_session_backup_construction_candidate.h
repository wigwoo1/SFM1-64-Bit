#pragma once
#include <cstdint>
#include <optional>
#include <vector>
struct MaterialSessionBackupConstructionServicesCandidate{
 virtual ~MaterialSessionBackupConstructionServicesCandidate()=default;
 virtual uintptr_t resolve(uint32_t)=0;
 virtual bool isElement(uintptr_t)=0;
 // Engaged0 is an actual failed native copy. Nullopt is an unavailable service.
 // Service must implement full native mode2 copy and own temporary lifetimes;
 // this collection builder does not assume a shallow Graph copy is equivalent.
 virtual std::optional<uintptr_t>copyElement(uintptr_t,uint32_t mode)=0;
 virtual bool isMaterial(uintptr_t)=0;
 virtual bool setFileId(uintptr_t,uint32_t file,uint32_t mode,uint8_t sameFileOnly)=0;
 virtual uint32_t handle(uintptr_t)=0;
};
//10240950 preserves source positions, including invalid placeholders and
// duplicates. Attribute lookup/type15 selection belongs to the caller adapter.
// Value output is staged; services own cleanup of created/detached elements.
inline bool constructMaterialSessionBackupsCandidate(const std::vector<uint32_t>*source,std::vector<uint32_t>&output,MaterialSessionBackupConstructionServicesCandidate&service){
 if(source&&source->size()>4096)return false;
 std::vector<uint32_t>plan;
 if(source){const auto count=source->size();plan.reserve(count);for(size_t i=0;i<count;++i){
  uint32_t result=UINT32_MAX;auto element=service.resolve((*source)[i]);
  if(element&&service.isElement(element)){
   // The native repeats resolution/type checking before copying. It assumes
   // stable resolution; refuse a broken provider instead of dereferencing null.
   element=service.resolve((*source)[i]);if(!element||!service.isElement(element))return false;
   auto copy=service.copyElement(element,2);if(!copy)return false;
   if(*copy&&service.isMaterial(*copy)){
    if(!service.setFileId(*copy,UINT32_MAX,2,0))return false;
    result=service.handle(*copy);
   }
  }
  plan.push_back(result);
 }}
 output.swap(plan);return true;
}
