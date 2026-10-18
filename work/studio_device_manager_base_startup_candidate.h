#pragma once
#include <cstdint>
struct StudioDeviceManagerBaseStartupCandidate {
 std::uintptr_t incomingFactory=0,cvar=0,fileSystem=0,shaderUtil=0,manager=0;
 std::uint32_t accessInitializationFlags=0;
};
// Operations bind the verified connector/fallback plus registration/timing services.
template<class Ops>bool studioDeviceManagerBaseConnectCandidate(StudioDeviceManagerBaseStartupCandidate& state,std::uintptr_t self,std::uintptr_t factory,Ops& ops){
 state.incomingFactory=factory;ops.connectInterfaces();
 if(!(state.accessInitializationFlags&1)){state.accessInitializationFlags|=1;ops.initializeCvarAccess();}
 if(state.cvar)ops.registerCvars(0x800000);
 ops.connectResources();
 auto utility=ops.externalFactory(state.incomingFactory,"VShaderUtil001");
 if(!utility)utility=ops.localFactory("VShaderUtil001");
 state.shaderUtil=utility;state.manager=self;state.incomingFactory=0;
 if(!utility||!state.fileSystem||!self){ops.warning("ShaderAPIDx10 was unable to access the required interfaces!\n");return false;}
 ops.initializeMathLibrary(2.2f,2.2f,0.0f,2,1,1,1,1);return true;
}

