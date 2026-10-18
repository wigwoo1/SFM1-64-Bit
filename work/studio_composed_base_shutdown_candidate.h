#pragma once
#include "studio_device_manager_base_shutdown_candidate.h"
#include "studio_shutdown_services_candidate.h"
#include "studio_interface_connector_candidate.h"
template<class Services>struct StudioComposedBaseShutdownOpsCandidate {
 StudioShutdownServicesCandidate& serviceState;
 StudioInterfaceConnectorCandidate& interfaces;
 Services& services;
 void resourceCleanup(){studioResourceShutdownCandidate(serviceState,services);}
 void cvarCleanup(){studioCvarShutdownCandidate(serviceState,services);}
 void interfaceCleanup(){studioDisconnectInterfacesCandidate(interfaces);}
 void deleteOwnedTree(std::uintptr_t tree){services.deleteOwnedTree(tree);}
};
template<class Services>void studioComposedBaseShutdownCandidate(StudioDeviceManagerBaseShutdownCandidate& base,StudioShutdownServicesCandidate& serviceState,StudioInterfaceConnectorCandidate& interfaces,Services& services){
 StudioComposedBaseShutdownOpsCandidate<Services> ops{serviceState,interfaces,services};
 studioDeviceManagerBaseDisconnectCandidate(base,ops);
}
