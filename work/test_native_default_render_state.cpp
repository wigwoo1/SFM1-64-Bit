#include <array>
#include "native_default_render_state_candidate.h"
#include <cassert>
#include <vector>
struct Capability{unsigned flags=0x30,vendor=0x1002;};
struct Lifecycle{Capability caps;unsigned suppressed(){return 0;}void flush(){assert(false);}void publish(uint32_t*){assert(false);}uint32_t* acquire(unsigned){return nullptr;}Capability* capability(){return &caps;}unsigned flags(Capability* p){return p->flags;}unsigned vendor(Capability* p){return p->vendor;}};
struct Device{std::vector<uint32_t> trace;HRESULT SetRenderState(D3DRENDERSTATETYPE key,DWORD value){trace.push_back(static_cast<uint32_t>(key));trace.push_back(value);return E_FAIL;}};
void compileSdk(StudioRenderStateOwnerCandidate& owner,IDirect3DDevice9*& device,uint32_t& queued,Lifecycle& lifecycle){NativeDefaultRenderStateServicesCandidateT<IDirect3DDevice9,Lifecycle> services{device,queued,lifecycle};studioDefaultRenderStateOwnerCandidate(owner,services);}
int main(){std::array<uint32_t,256> cache{};std::array<uint32_t,512> commands{};StudioDeviceQueueStateCandidate queue{commands.data(),commands.data(),512};StudioRenderStateOwnerCandidate owner{cache.data(),&queue};Device device;Device* current=&device;uint32_t queued=0;Lifecycle lifecycle;NativeDefaultRenderStateServicesCandidateT<Device,Lifecycle> services{current,queued,lifecycle};studioDefaultRenderStateOwnerCandidate(owner,services);assert(device.trace.size()==182);for(unsigned i=0;i<91;++i){auto entry=studioDefaultRenderStateEntriesCandidate[i];assert(device.trace[2*i]==entry.state&&device.trace[2*i+1]==entry.value);}queued=1;studioDefaultRenderStateOwnerCandidate(owner,services);assert(device.trace.size()==182&&queue.cursor==commands.data()+273&&queue.remaining==239);for(unsigned i=0;i<91;++i){auto entry=studioDefaultRenderStateEntriesCandidate[i];assert(commands[3*i]==1&&commands[3*i+1]==entry.state&&commands[3*i+2]==entry.value);} }

