#pragma once
#include "studio_composed_base_startup_candidate.h"
#include "studio_resource_startup_candidate.h"
struct StudioResourceStartupServiceViewCandidate {
 std::uintptr_t& resource;std::uintptr_t& resourceToken;bool& resourceRegistered;
};
struct StudioResourceStartupStateViewCandidate {
 std::uintptr_t& precache;bool& precacheInitialized;
 std::array<std::uintptr_t,5>& groups;StudioResourceRegistrationNodeCandidate*& head;
};
template<class Host,class ResourceOps>struct StudioResourceBoundStartupHostCandidate {
 Host& host;ResourceOps& resourceOps;StudioBaseStartupBindingsCandidate& bindings;
 StudioShutdownServicesCandidate& services;StudioResourceStartupCandidate& resources;
 void diagnostic(unsigned count){host.diagnostic(count);}
 void initializeCvarAccess(){host.initializeCvarAccess();}
 void registerCvars(unsigned flags){host.registerCvars(flags);}
 void connectResources(){
  StudioResourceStartupServiceViewCandidate service{bindings.cells[9],services.resourceToken,services.resourceRegistered};
  StudioResourceStartupStateViewCandidate state{bindings.cells[25],resources.precacheInitialized,resources.groups,resources.head};
  studioConnectResourceServicesCandidate(service,state,resourceOps);
 }
 void warning(const char* message){host.warning(message);}
 void initializeMathLibrary(float a,float b,float c,int d,int e,int f,int g,int h){host.initializeMathLibrary(a,b,c,d,e,f,g,h);}
};

