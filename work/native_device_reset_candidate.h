#pragma once
#include <d3d9.h>
#include "studio_device_reset_candidate.h"
// References retain the live owner fields across lifecycle callbacks.
struct NativeDeviceResetOwnerCandidate {
 unsigned char& flags;
 HWND& window;
 UINT& adapter;
 D3DPRESENT_PARAMETERS& parameters;
};
// The lifecycle owns descriptor interpretation, resource release and device lifetime.
template<class Lifecycle>struct NativeDeviceResetServicesCandidateT {
 Lifecycle& lifecycle;
 template<class Descriptor>auto descriptorFlags(Descriptor& value){return lifecycle.descriptorFlags(value);}
 template<class Descriptor>void rebuild(HWND window,UINT adapter,Descriptor& value,bool initial){lifecycle.rebuild(window,adapter,value,initial);}
 void managerCleanup(){lifecycle.managerCleanup();}
 void ownerHook(bool enabled){lifecycle.ownerHook(enabled);}
 auto& commandState(){return lifecycle.commandState();}
 template<class State>void flush(State& value){lifecycle.flush(value);}
 template<class State>auto* device(State& value){return lifecycle.device(value);}
 template<class Device>HRESULT reset(Device* value,D3DPRESENT_PARAMETERS& parameters){return value->Reset(&parameters);}
 void resetFailure(HRESULT value){lifecycle.resetFailure(value);}
 void postReset(bool a,bool b){lifecycle.postReset(a,b);}
};
