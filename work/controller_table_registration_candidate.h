#pragma once
#include <cstdint>
template<class Record,class Adapter>void registerControllerTableCandidate(std::uint32_t mode,Record* records,Adapter& a){
 if(!a.registrationService()){
  a.guardService(9,1,215);
  if(!a.registrationService())a.installDefaultRegistrationService();
  a.guardService(10,1,218);
 }
 for(auto* record=records;record->code;++record){
  if(mode)record->code|=(mode&255u)<<24;
  a.registerServiceSlot3(a.registrationService(),record);
 }
}
