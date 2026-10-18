#pragma once
#include <d3d9.h>
#include "studio_device_manager_backend_candidate.h"
// Base startup/shutdown and diagnostics are supplied by the application owner.
template<class Host> struct StudioNativeDeviceManagerBackendCandidate {
 Host& host;
 bool baseConnect(std::uintptr_t factory){return host.baseConnect(factory);}
 std::uintptr_t createDirect3D(unsigned version){return reinterpret_cast<std::uintptr_t>(Direct3DCreate9(version));}
 void warning(const char* text){host.warning(text);}
 void release(std::uintptr_t pointer){reinterpret_cast<IDirect3D9*>(pointer)->Release();}
 void baseDisconnect(){host.baseDisconnect();}
};
