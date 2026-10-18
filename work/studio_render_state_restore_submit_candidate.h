#pragma once
#include "studio_render_state_owner_candidate.h"
// Logical1000e490: force submission even when cached value already matches.
template<class Services>void studioRenderStateRestoreSubmitCandidate(StudioRenderStateOwnerCandidate& owner,uint32_t key,uint32_t value,Services& services){
 StudioRenderStateOwnerAdapterCandidate<Services> adapter{owner,services};
 if(static_cast<uint8_t>(adapter.suppressed()))return;
 if(adapter.queueEnabled())adapter.queueState(key,value);else adapter.directState(key,value);
 owner.cache[key]=value;
}
