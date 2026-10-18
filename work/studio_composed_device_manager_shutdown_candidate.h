#pragma once
#include "studio_device_manager_backend_candidate.h"
#include "studio_owned_shutdown_services_candidate.h"
template<class BackendOps,class ServiceOps,class AllocationOps>struct StudioComposedDeviceManagerShutdownOpsCandidate {
 StudioDeviceManagerBaseShutdownCandidate& base;
 StudioShutdownServicesCandidate& serviceState;
 StudioInterfaceConnectorCandidate& interfaces;
 BackendOps& backend;ServiceOps& services;AllocationOps& allocation;
 void release(std::uintptr_t pointer){backend.release(pointer);}
 void baseDisconnect(){StudioOwnedShutdownServicesCandidate<ServiceOps,AllocationOps> owned{services,allocation};studioComposedBaseShutdownCandidate(base,serviceState,interfaces,owned);}
};
template<class BackendOps,class ServiceOps,class AllocationOps>void studioComposedDeviceManagerShutdownCandidate(StudioDeviceManagerBackendCandidate& device,StudioDeviceManagerBaseShutdownCandidate& base,StudioShutdownServicesCandidate& serviceState,StudioInterfaceConnectorCandidate& interfaces,BackendOps& backend,ServiceOps& services,AllocationOps& allocation){
 StudioComposedDeviceManagerShutdownOpsCandidate<BackendOps,ServiceOps,AllocationOps> ops{base,serviceState,interfaces,backend,services,allocation};
 studioDeviceManagerDisconnectCandidate(device,ops);
}
