#pragma once
#include "studio_resource_bound_startup_candidate.h"
#include "studio_device_manager_base_shutdown_candidate.h"
#include "studio_owned_keyvalues_cleanup_candidate.h"
struct StudioLiveBaseShutdownViewCandidate {std::uintptr_t& service54;std::uintptr_t& service58;std::uintptr_t& ownedTree;};
struct StudioLiveShutdownServicesViewCandidate {
 std::uintptr_t& resource;std::uintptr_t& cvar;std::uintptr_t& resourceToken;
 std::uint32_t& cvarHandle;bool& resourceRegistered;bool& cvarRegistered;
};
template<class Services,class Allocation>struct StudioLiveBaseShutdownOpsCandidate {
 StudioLiveShutdownServicesViewCandidate& state;StudioInterfaceConnectorCandidate& connector;Services& services;Allocation& allocation;
 void resourceCleanup(){studioResourceShutdownCandidate(state,services);}
 void cvarCleanup(){studioCvarShutdownCandidate(state,services);}
 void interfaceCleanup(){studioDisconnectInterfacesCandidate(connector);}
 void deleteOwnedTree(std::uintptr_t tree){studioDeleteOwnedKeyValuesCandidate(reinterpret_cast<StudioOwnedKeyValuesNodeCandidate*>(tree),allocation);}
};
template<class Services,class Allocation>void studioLiveBaseShutdownCandidate(StudioDeviceManagerBaseStartupCandidate& startup,StudioBaseStartupBindingsCandidate& bindings,StudioShutdownServicesCandidate& bookkeeping,StudioInterfaceConnectorCandidate& connector,std::uintptr_t& ownedTree,Services& services,Allocation& allocation){
 StudioLiveBaseShutdownViewCandidate base{startup.manager,startup.shaderUtil,ownedTree};
 StudioLiveShutdownServicesViewCandidate state{bindings.cells[9],startup.cvar,bookkeeping.resourceToken,bookkeeping.cvarHandle,bookkeeping.resourceRegistered,bookkeeping.cvarRegistered};
 StudioLiveBaseShutdownOpsCandidate<Services,Allocation> ops{state,connector,services,allocation};studioDeviceManagerBaseDisconnectCandidate(base,ops);
}
