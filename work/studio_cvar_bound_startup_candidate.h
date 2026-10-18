#pragma once
#include "studio_cvar_startup_candidate.h"
#include "studio_resource_bound_startup_candidate.h"
struct StudioCvarStartupServiceViewCandidate {
 std::uintptr_t& cvar;bool& cvarRegistered;std::uint32_t& cvarHandle;
};
template<class Host,class CvarOps>struct StudioCvarBoundStartupHostCandidate {
 Host& host;CvarOps& cvarOps;StudioDeviceManagerBaseStartupCandidate& startup;
 StudioShutdownServicesCandidate& bookkeeping;StudioCvarStartupCandidate& cvars;
 std::uintptr_t accessObject;
 void diagnostic(unsigned count){host.diagnostic(count);}
 void initializeCvarAccess(){host.initializeCvarAccess();}
 void registerCvars(unsigned flags){StudioCvarStartupServiceViewCandidate services{startup.cvar,bookkeeping.cvarRegistered,bookkeeping.cvarHandle};studioRegisterCvarServicesCandidate(services,cvars,flags,accessObject,cvarOps);}
 void connectResources(){host.connectResources();}
 void warning(const char* message){host.warning(message);}
 void initializeMathLibrary(float a,float b,float c,int d,int e,int f,int g,int h){host.initializeMathLibrary(a,b,c,d,e,f,g,h);}
};

