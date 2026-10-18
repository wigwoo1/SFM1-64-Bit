#include <cstdio>
#include <stdexcept>
#include <vector>
#include "native_d3d9_vertex_texture_candidate.h"
#include "native_manager_callback_registry_candidate.h"
struct Texture{};
struct Event{unsigned stage,type;uintptr_t value;bool queued;};
struct Lifecycle{
 std::vector<Event> events;uint32_t mode=0;bool switchMode=false;
 void queueTexture(uint32_t stage,Texture* p){events.push_back({stage,0,reinterpret_cast<uintptr_t>(p),true});}
 void queueSampler(uint32_t stage,D3DSAMPLERSTATETYPE type,uint32_t value){events.push_back({stage,unsigned(type),value,true});}
};
struct Device{
 Lifecycle& lifecycle;
 HRESULT SetTexture(UINT stage,Texture* p){lifecycle.events.push_back({stage,0,reinterpret_cast<uintptr_t>(p),false});if(lifecycle.switchMode)lifecycle.mode=1;return E_FAIL;}
 HRESULT SetSamplerState(UINT stage,D3DSAMPLERSTATETYPE type,DWORD value){lifecycle.events.push_back({stage,unsigned(type),value,false});return E_FAIL;}
};
using Services=NativeVertexTextureSubmissionServicesCandidateT<Device,Texture,Lifecycle>;
struct Command{Services& services;unsigned countReads=0;int32_t count(){++countReads;return 3;}};
struct Current{StudioVertexTextureRecordCandidate* records[3];};
struct Previous{StudioVertexTextureCacheCandidate caches[3];};
static void callback(Command* command,Current* current,Previous* previous,bool force){const auto count=command->count();studioVertexTextureSubmitCandidate(current->records,previous->caches,count,force,command->services);}
static void require(bool value){if(!value)throw std::runtime_error("restore vertex composition mismatch");}
// Compile-only SDK instantiation checks actual COM method signatures.
struct SdkLifecycle{void queueTexture(uint32_t,IDirect3DBaseTexture9*){}void queueSampler(uint32_t,D3DSAMPLERSTATETYPE,uint32_t){}};
void compileSdk(IDirect3DDevice9*& device,uint32_t& mode,SdkLifecycle& lifecycle){NativeVertexTextureSubmissionServicesCandidateT<IDirect3DDevice9,IDirect3DBaseTexture9,SdkLifecycle> s{device,mode,lifecycle};s.directTexture(257,0);s.directSampler(257,1,0);s.queueTexture(257,0);s.queueSampler(257,1,0);}
int main(){try{unsigned cases=0;
 for(bool queued:{false,true})for(bool switchMode:{false,true}){
  Lifecycle lifecycle; lifecycle.mode=queued?1:0;lifecycle.switchMode=switchMode;Device object{lifecycle};Device* device=&object;Services services{device,lifecycle.mode,lifecycle};Command command{services};
  uintptr_t resources[]{0x100001234ull,0x200005678ull};StudioVertexTextureRecordCandidate record{{1,2,3,4,5,6},2,1,reinterpret_cast<uintptr_t>(resources)};Current current{{&record,nullptr,&record}};Previous previous{};
  previous.caches[1].record=&record;
  NativeManagerCallbackAllocatorCandidate allocator;using Callback=void(*)(Command*,Current*,Previous*,bool);NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback> registry{allocator};registry.append(callback);
  registry.dispatchWith(&command,&current,&previous,false);require(command.countReads==1&&lifecycle.events.size()==13);
  constexpr unsigned types[]{0,1,2,6,5,7};for(unsigned i=0;i<6;++i){require(lifecycle.events[i].stage==257&&lifecycle.events[i].type==types[i]);require(lifecycle.events[i].queued==(queued||(switchMode&&i>0)));}
  require(lifecycle.events[0].value==resources[1]&&lifecycle.events[6].stage==258&&lifecycle.events[6].value==0&&lifecycle.events[7].stage==259&&lifecycle.events[7].value==resources[1]);
  for(unsigned i=0;i<3;++i)require(previous.caches[i].record==current.records[i]);
  registry.dispatchWith(&command,&current,&previous,false);require(command.countReads==2&&lifecycle.events.size()==13);
  registry.dispatchWith(&command,&current,&previous,true);require(command.countReads==3&&lifecycle.events.size()==26);++cases;
 }
 std::printf("restore_vertex_texture_typed_composition_cases=%u passed\n",cases);
 }catch(const std::exception& e){std::puts(e.what());return 1;}}
