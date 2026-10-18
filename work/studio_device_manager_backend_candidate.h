#pragma once
#include <cstdint>
struct StudioDeviceManagerBackendCandidate { std::uintptr_t direct3D=0; };
// Logical pointer-wide state; not the original binary object layout.
template<class Ops> bool studioDeviceManagerConnectCandidate(StudioDeviceManagerBackendCandidate& state,std::uintptr_t factory,Ops& ops){
 if(!ops.baseConnect(factory))return false;
 state.direct3D=ops.createDirect3D(32);
 if(state.direct3D)return true;
 ops.warning("Failed to create D3D9!\n");return false;
}
template<class Ops> void studioDeviceManagerDisconnectCandidate(StudioDeviceManagerBackendCandidate& state,Ops& ops){
 if(state.direct3D){ops.release(state.direct3D);state.direct3D=0;}
 ops.baseDisconnect();
}
