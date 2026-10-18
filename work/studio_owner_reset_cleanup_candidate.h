#pragma once
#include <cstdint>
// Normalized live owner fields; physical PE32 offsets are not reused.
template<class Owner,class Services>
void studioOwnerResetCleanupCandidate(Owner& owner,Services& services,bool enabled){
 if(!services.inOwningThread(owner)||!services.inMainThread()){
  owner.flags=static_cast<unsigned char>(owner.flags&0xdfu);
  services.deferCleanup(true);return;
 }
 const uint32_t previous=owner.cleanupDepth;
 owner.cleanupDepth=previous+uint32_t{1}; // Defined modulo-2^32 native increment.
 if(previous!=0){services.nestedWarning(owner.cleanupDepth);services.nestedDiagnostic();return;}
 if(services.notificationAvailable()){
  services.beginNotification(uint32_t{0xfff5940f});
  if(services.notificationAvailable())services.endNotification();
 }
 services.queryCleanup(owner);
 services.cleanupFirst(owner);
 services.cleanupSecond(owner);
 services.setCleanupMode(enabled?2u:0u);
 services.finishCommandState();
 services.publishCleanupEnabled(enabled);
 owner.flags=static_cast<unsigned char>(owner.flags|0x20u);
}
