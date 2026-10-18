#pragma once
#include "studio_default_render_state_candidate.h"
#include "studio_render_state_restore_submit_candidate.h"
template<class Services>struct StudioDefaultRenderStateOwnerServicesCandidate{
 StudioRenderStateOwnerCandidate& owner;Services& services;
 auto capability(){return services.capability();}
 template<class Capability>unsigned flags(Capability* value){return services.flags(value);}
 template<class Capability>unsigned vendor(Capability* value){return services.vendor(value);}
 void submit(uint32_t key,uint32_t value){studioRenderStateRestoreSubmitCandidate(owner,key,value,services);}
};
template<class Services>void studioDefaultRenderStateOwnerCandidate(StudioRenderStateOwnerCandidate& owner,Services& services){
 StudioDefaultRenderStateOwnerServicesCandidate<Services> adapter{owner,services};studioDefaultRenderStateCandidate(adapter);
}
