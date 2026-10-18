#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "native_reset_presentation_lifecycle_candidate.h"
#include "native_manager_callback_registry_candidate.h"
#include "native_manager_reset_hooks_candidate.h"
static void require(bool value){if(!value)throw std::runtime_error("reset/presentation composition mismatch");}
static bool resourceAlive=true;static unsigned callbackCalls=0;static unsigned* managerCount=nullptr;static D3DPRESENT_PARAMETERS* callbackParameters=nullptr;
static void releaseResource(){require(resourceAlive&&callbackParameters->BackBufferWidth==777);resourceAlive=false;++callbackCalls;++*managerCount;}
static void otherResource(){require(!resourceAlive);++callbackCalls;}
struct Device{
 D3DPRESENT_PARAMETERS& parameters;HWND& window;StudioPresentationDescriptorCandidate& descriptor;HRESULT result;unsigned calls=0;
 HRESULT Reset(D3DPRESENT_PARAMETERS* value){require(!resourceAlive&&callbackCalls==9);require(value==&parameters&&value->hDeviceWindow==window&&value->BackBufferWidth==static_cast<unsigned>(descriptor.width)&&value->BackBufferHeight==static_cast<unsigned>(descriptor.height)&&value->Windowed==1&&value->BackBufferFormat==D3DFMT_X8R8G8B8);++calls;return result;}
};
struct Presentation{
 NativeDeviceResetOwnerCandidate owner;unsigned displays=0,dimensionsCalls=0,reads=0,conditions=0,formats=0,samples=0;
 void displayMode(std::array<uint32_t,6>& value,unsigned adapter){require(adapter==owner.adapter&&value[0]==1);++displays;value={1,1280,720,0,0,0};}
 void dimensions(std::array<uint32_t,6>&,StudioPresentationDescriptorCandidate&,int32_t& width,int32_t& height){++dimensionsCalls;width=1280;height=720;}
 float aspectThreshold(){return 1.5f;}Presentation& material(){++reads;return *this;}
 void setCondition(const char* name,int value){const char* names[]={"WIN32WIDE","WIN32HIDEF","WIN32LODEF"};unsigned index=conditions%3;require(!std::strcmp(name,names[index])&&value==(index==2?0:1));++conditions;}
 unsigned colorChoice(unsigned adapter,unsigned type,unsigned prior,unsigned requested,unsigned windowed){require(adapter==owner.adapter&&type==1&&prior==22&&requested==22&&windowed==1);++formats;return 22;}
 unsigned depthFormat(unsigned adapter,unsigned prior,unsigned color,unsigned preferred){require(adapter==owner.adapter&&prior==22&&color==22&&preferred==77);++formats;return 75;}
 unsigned translateFormat(unsigned color){require(color==22);++formats;return 22;}
 bool forceNoVsync(){++samples;return false;}bool tripleBuffered(){++samples;return false;}
 unsigned sampleType(int32_t){++samples;return 0;}unsigned adapterVendor(unsigned){++samples;return 0;}
 HRESULT sampleSupport(unsigned,unsigned,D3DFORMAT,BOOL,D3DMULTISAMPLE_TYPE,unsigned&){++samples;return E_FAIL;}
};
struct Hooks{
 NativeDeviceResetOwnerCandidate owner;StudioPresentationDescriptorCandidate& descriptor;Presentation& presentation;Device*& current;Device& replacement;bool mutate,replace;unsigned managers=0,hooks=0,flushes=0,logs=0,posts=0;
 void managerCleanup(){require(presentation.displays==1&&owner.parameters.BackBufferWidth==777);++managers;}
 void ownerHook(bool enabled){require(enabled&&managers==1&&presentation.displays==1&&owner.parameters.BackBufferWidth==777);++hooks;if(mutate){owner.window=reinterpret_cast<HWND>(uintptr_t{0x100005678ull});owner.adapter=9;descriptor.width=1920;descriptor.height=1080;}}
 Device*& commandState(){require(presentation.displays==2&&hooks==1);return current;}
 void flush(Device*& state){require(presentation.formats==3);++flushes;if(replace)state=&replacement;}
 Device* device(Device*& state){require(flushes==1);return state;}
 void resetFailure(HRESULT result){require(result<0);++logs;}
 void postReset(bool a,bool b){require(a&&b);++posts;}
};
int main(){try{unsigned cases=0;for(bool mutate:{false,true})for(bool replace:{false,true})for(unsigned bits:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
 unsigned char flags=0xff;HWND window=reinterpret_cast<HWND>(uintptr_t{0x100001234ull});UINT adapter=3;D3DPRESENT_PARAMETERS parameters{};parameters.BackBufferWidth=777;NativeDeviceResetOwnerCandidate owner{flags,window,adapter,parameters};StudioPresentationDescriptorCandidate descriptor{800,600,22,0,1,0,0,0};unsigned prior=22,depthBits=0;
 Device first{parameters,window,descriptor,static_cast<HRESULT>(bits)},second{parameters,window,descriptor,static_cast<HRESULT>(bits)};Device* current=&first;Presentation presentation{owner};Hooks hooks{owner,descriptor,presentation,current,second,mutate,replace};NativeManagerCallbackAllocatorCandidate allocator;NativeManagerCallbackRegistryCandidateT<> registry{allocator};registry.append(releaseResource);for(unsigned i=0;i<8;++i)registry.append(otherResource);resourceAlive=true;callbackCalls=0;managerCount=&hooks.managers;callbackParameters=&parameters;NativeManagerResetHooksCandidateT<Hooks,decltype(registry)> managerHooks{hooks,registry};NativeResetPresentationLifecycleCandidateT<decltype(managerHooks),Presentation> lifecycle{managerHooks,owner,prior,depthBits,presentation};NativeDeviceResetServicesCandidateT<decltype(lifecycle)> services{lifecycle};
 const bool expected=static_cast<HRESULT>(bits)>=0;require(studioDeviceResetCandidate(owner,descriptor,services)==expected);require(presentation.displays==2&&presentation.dimensionsCalls==2&&presentation.reads==6&&presentation.conditions==6&&presentation.formats==3&&presentation.samples==0&&depthBits==8);require(first.calls==(replace?0u:1u)&&second.calls==(replace?1u:0u)&&hooks.logs==(expected?0u:1u)&&hooks.posts==(expected?1u:0u));++cases;
 require(registry.remove(releaseResource));unsigned removed=0;while(registry.remove(otherResource))++removed;require(removed==8&&registry.size()==0);resourceAlive=true;const auto beforeCallbacks=callbackCalls;descriptor.flags=3;flags=0xff;auto before=parameters;const unsigned previous=presentation.displays;require(!studioDeviceResetCandidate(owner,descriptor,services)&&flags==0xf7&&presentation.displays==previous&&!std::memcmp(&before,&parameters,sizeof(parameters))&&resourceAlive&&callbackCalls==beforeCallbacks);++cases;
 }std::printf("x64_reset_presentation_composition_cases=%u gpu_executed=0\n",cases);}catch(const std::exception& error){std::puts(error.what());return 1;}}

