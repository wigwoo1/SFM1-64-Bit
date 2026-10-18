#pragma once
#include <cstdint>
// Recovered post-reset policy; service/resource creation bindings remain external.
template<class Owner,class Services>
void studioOwnerPostResetCandidate(Owner& owner,Services& services,bool restore,bool force,const char* reason){
 if(!services.inOwningThread(owner)||!services.inMainThread()){
  if(restore)services.deferPostReset(7u);
  services.deferPostReset(2u);return;
 }
 if(force){
  const uint32_t depth=owner.cleanupDepth;
  if(depth>1u&&depth<0x80000000u)services.forcedDepthWarning(depth,reason);
  owner.cleanupDepth=0;
 }else{
  owner.cleanupDepth-=uint32_t{1};
  if(owner.cleanupDepth!=0){
   services.unbalancedWarning(owner.cleanupDepth);services.unbalancedDiagnostic();
   if(owner.cleanupDepth&0x80000000u)owner.cleanupDepth=0;
   return;
  }
 }
 if(restore)services.restoreOwner(owner,true);
 if(services.notificationAvailable())services.beginPostNotification(uint32_t{0xfff5940f});
 services.prepareRecreation();
 services.recreateFirst(owner);services.recreateSecond(owner);
 services.resumeCommandState();services.publishInterfaceFactory();
 if(services.notificationAvailable())services.endNotification();
}
