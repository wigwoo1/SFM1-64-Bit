#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include "studio_native_device_manager_backend_candidate.h"
static void check(bool v){if(!v)std::abort();}
struct Host {
 StudioDeviceManagerBackendCandidate& state;unsigned connects=0,disconnects=0,warnings=0;
 bool baseConnect(uintptr_t factory){check(factory==0x1234&&state.direct3D==0);++connects;return true;}
 void warning(const char* text){++warnings;std::printf("warning=%s",text);}
 void baseDisconnect(){check(state.direct3D==0);++disconnects;}
};
int main(){static_assert(sizeof(void*)==8,"x64");StudioDeviceManagerBackendCandidate state;Host host{state};StudioNativeDeviceManagerBackendCandidate<Host> backend{host};
 for(unsigned cycle=0;cycle<3;++cycle){bool connected=studioDeviceManagerConnectCandidate(state,0x1234,backend);if(!connected){studioDeviceManagerDisconnectCandidate(state,backend);std::printf("real_d3d9_creation_unavailable cycle=%u\n",cycle);return 2;}
 auto* api=reinterpret_cast<IDirect3D9*>(state.direct3D);check(state.direct3D>UINT32_MAX);UINT count=api->GetAdapterCount();std::printf("cycle=%u pointer_above_4GB=1 adapters=%u\n",cycle,count);
 for(UINT adapter=0;adapter<count;++adapter){D3DADAPTER_IDENTIFIER9 id{};HRESULT identifier=api->GetAdapterIdentifier(adapter,0,&id);D3DCAPS9 caps{};HRESULT capability=api->GetDeviceCaps(adapter,D3DDEVTYPE_HAL,&caps);std::printf("adapter=%u identifier_hresult=%08lx caps_hresult=%08lx",adapter,static_cast<unsigned long>(identifier),static_cast<unsigned long>(capability));if(SUCCEEDED(identifier))std::printf(" description=%s",id.Description);std::printf("\n");}
 studioDeviceManagerDisconnectCandidate(state,backend);check(state.direct3D==0);}
 check(host.connects==3&&host.disconnects==3&&host.warnings==0);studioDeviceManagerDisconnectCandidate(state,backend);check(host.disconnects==4);std::puts("real_x64_d3d9_owner_create_enumerate_release_reconnect=passed");
}
