#pragma once
#include <cstdint>
template<class Adapter>auto lookupControllerRegistrationCandidate(std::uint32_t code,Adapter& a){
 if(!a.registrationService()){
  a.guardService(9,1,215);
  if(!a.registrationService())a.installDefaultRegistrationService();
  a.guardService(10,1,218);
 }
 const std::uint32_t key=code&0xfffff000u;
 auto* record=a.lookupServiceSlot2(a.registrationService(),&key);
 return record?a.recordPayload(record):a.nullPayload();
}
