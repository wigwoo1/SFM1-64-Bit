#pragma once
#include <cstdint>
template<class Adapter>auto defaultControllerRegistrationLookupCandidate(const std::uint32_t* key,Adapter& a){
 if(!a.registrationService()){
  a.guardService(9,1,215);
  if(!a.registrationService())a.installDefaultRegistrationService();
  a.guardService(10,1,218);
 }
 auto* container=a.getServiceContainer(a.registrationService(),0);
 if(!container)return a.nullRecord();
 a.guardService(5,1,298);
 auto* record=a.lookupContainer(container,key);
 a.guardService(6,1,300);
 return record;
}
