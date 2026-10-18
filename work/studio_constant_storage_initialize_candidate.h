#pragma once
#include <cstdint>
// Initialization sections of 10015bd0, after each six-buffer allocation group.
// Buffer ids are capability count offsets (528..53c), not owner byte offsets.
// Caller owns allocation validity; no invented Source interface ABI here.
inline bool studioConstantCountPositiveCandidate(uint32_t count){return count!=0 && count<0x80000000u;}
template<class Services>
void studioConstantStorageInitializeGroupCandidate(bool vertex,Services& services){
 const unsigned floatId=vertex?3u:0u;
 auto* capability=services.capability();
 uint32_t index=0;
 while(studioConstantCountPositiveCandidate(capability->count(floatId)) && index<capability->count(floatId)){
  auto* destination=services.staging(floatId)+static_cast<uint64_t>(index)*4;
  for(unsigned component=0;component<4;++component)destination[component]=0;
  ++index;
 }
 const uint32_t floatCount=capability->count(floatId);
 auto* captured=services.staging(floatId);
 services.submitFloat(vertex,0,floatCount,captured);
 const uint32_t floatBytes=floatCount<<4;
 services.copy(services.cache(floatId),captured,floatBytes);
 services.copy(services.staging(floatId),captured,floatBytes);
 for(unsigned kind=1;kind<=2;++kind){
  const unsigned id=floatId+kind;
  capability=services.capability();
  const uint32_t capturedCount=capability->count(id);
  if(!vertex && capturedCount==0)continue;
  index=0;
  uint32_t current=capturedCount;
  while(studioConstantCountPositiveCandidate(current) && index<current){
   auto* destination=services.staging(id)+static_cast<uint64_t>(index)*(kind==1?1:4);
   for(unsigned component=0;component<(kind==1?1u:4u);++component)destination[component]=0;
   ++index;
   if(vertex){capability=services.capability();current=capability->count(id);}
  }
  const uint32_t submittedCount=vertex?capability->count(id):capturedCount;
  services.submitSubobject(vertex?(kind==1?41u:42u):(kind==1?43u:44u),0,services.staging(id),submittedCount,1);
 }
}
