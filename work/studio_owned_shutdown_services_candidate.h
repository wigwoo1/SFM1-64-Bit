#pragma once
#include "studio_composed_base_shutdown_candidate.h"
#include "studio_owned_keyvalues_cleanup_candidate.h"
// Node and payload allocators remain separate injected ownership services.
template<class ServiceOps,class AllocationOps>struct StudioOwnedShutdownServicesCandidate {
 ServiceOps& service;AllocationOps& allocation;
 void resourceSlot9(std::uintptr_t p,std::uintptr_t token){service.resourceSlot9(p,token);}
 void cvarSlot32(std::uintptr_t p,std::uint32_t handle){service.cvarSlot32(p,handle);}
 void cvarSlot11(std::uintptr_t p,std::uint32_t handle){service.cvarSlot11(p,handle);}
 void deleteOwnedTree(std::uintptr_t tree){studioDeleteOwnedKeyValuesCandidate(reinterpret_cast<StudioOwnedKeyValuesNodeCandidate*>(tree),allocation);}
};
