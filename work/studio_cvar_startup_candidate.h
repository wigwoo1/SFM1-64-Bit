#pragma once
#include "studio_shutdown_services_candidate.h"
struct StudioCvarPendingNodeCandidate {StudioCvarPendingNodeCandidate* next=nullptr;};
struct StudioCvarStartupCandidate {
 std::uint32_t flags=0;std::uintptr_t access=0,fallbackAccess=0;
 StudioCvarPendingNodeCandidate* head=nullptr;
};
template<class Services,class Ops>void studioRegisterCvarServicesCandidate(Services& services,StudioCvarStartupCandidate& state,std::uint32_t flags,std::uintptr_t access,Ops& ops){
 if(!services.cvar||services.cvarRegistered)return;
 state.flags=flags;services.cvarRegistered=true;
 services.cvarHandle=ops.cvarSlot8(services.cvar);
 state.access=access?access:state.fallbackAccess;
 for(auto* node=state.head;node;){auto suppliedFlags=state.flags;auto* next=node->next;ops.nodeSlot3(node,suppliedFlags);ops.nodeSlot11(node);node=next;}
 ops.cvarSlot31(services.cvar);ops.cvarSlot40(services.cvar);state.head=nullptr;
}
