#pragma once
#include <cstdint>
// Owner binding supplies live flags/window/adapter/typed presentation fields.
// Descriptor interpretation and lifecycle callbacks remain service-owned.
template<class Owner,class Descriptor,class Services>bool studioDeviceResetCandidate(Owner& owner,Descriptor& descriptor,Services& services){
 owner.flags&=0xf7u;if(services.descriptorFlags(descriptor)&2u)return false;
 services.rebuild(owner.window,owner.adapter,descriptor,true);
 services.managerCleanup();services.ownerHook(true);
 services.rebuild(owner.window,owner.adapter,descriptor,false);
 auto& commandStateRef=services.commandState();services.flush(commandStateRef);
 auto* device=services.device(commandStateRef);const auto result=services.reset(device,owner.parameters);
 if(static_cast<int32_t>(result)<0){services.resetFailure(result);return false;}
 services.postReset(true,true);return true;
}
