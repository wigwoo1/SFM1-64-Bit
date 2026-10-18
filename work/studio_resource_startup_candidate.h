#pragma once
#include <array>
#include "studio_shutdown_services_candidate.h"
struct StudioResourceRegistrationNodeCandidate {StudioResourceRegistrationNodeCandidate* next=nullptr;};
struct StudioResourceStartupCandidate {
 std::uintptr_t precache=0;bool precacheInitialized=false;
 std::array<std::uintptr_t,5> groups{};
 StudioResourceRegistrationNodeCandidate* head=nullptr;
};
template<class Services,class State,class Ops>void studioConnectResourceServicesCandidate(Services& services,State& state,Ops& ops){
 if(state.precache&&!state.precacheInitialized){for(unsigned i=0;i<5;++i)ops.precacheSlot10(state.precache,state.groups[i],i);state.precacheInitialized=true;}
 if(!services.resource||services.resourceRegistered)return;
 ops.resourceSlot8(services.resource,services.resourceToken);services.resourceRegistered=true;
 for(auto* node=state.head;node;node=node->next)ops.resourceSlot12(services.resource,node);
}

