#pragma once
#include <cstdint>
template<class Manager,class Callback,class Services>bool studioManagerRemoveCandidate(Manager& manager,Callback callback,Services& services){
 const int32_t count=manager.count;
 for(int32_t index=0;index<count;++index){
  if(manager.callbacks[index]!=callback)continue;
  const int32_t following=count-index-1;
  if(following>0)services.move(manager.callbacks+index,manager.callbacks+index+1,following);
  --manager.count;return true;
 }
 return false;
}
