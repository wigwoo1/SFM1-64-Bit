#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include "native_device_reset_candidate.h"
static void require(bool value){if(!value)throw std::runtime_error("typed reset mismatch");}
struct Device {
 D3DPRESENT_PARAMETERS* expected;
 HRESULT result;
 unsigned calls=0;
 HRESULT Reset(D3DPRESENT_PARAMETERS* value){require(value==expected);require(value->BackBufferWidth==1920&&reinterpret_cast<uintptr_t>(value->hDeviceWindow)==0x100005678ull);++calls;return result;}
};
struct Lifecycle {
 NativeDeviceResetOwnerCandidate& owner;
 Device*& current;
 Device& replacement;
 bool replace;
 unsigned step=0,logs=0,posts=0;
 unsigned descriptorFlags(unsigned& value){return value;}
 void rebuild(HWND window,UINT adapter,unsigned&,bool initial){require(++step==(initial?1u:4u));require(reinterpret_cast<uintptr_t>(window)==(initial?0x100001234ull:0x100005678ull));require(adapter==(initial?3u:9u));if(!initial){owner.parameters.BackBufferWidth=1920;owner.parameters.hDeviceWindow=window;}}
 void managerCleanup(){require(++step==2);}
 void ownerHook(bool enabled){require(++step==3&&enabled);owner.window=reinterpret_cast<HWND>(uintptr_t{0x100005678ull});owner.adapter=9;}
 Device*& commandState(){require(++step==5);return current;}
 void flush(Device*& value){require(++step==6);if(replace)value=&replacement;}
 Device* device(Device*& value){require(step==6);return value;}
 void resetFailure(HRESULT value){require(value<0);++logs;}
 void postReset(bool a,bool b){require(a&&b);++posts;}
};
int main(){try{
 static_assert(sizeof(HWND)==8,"x64 test required");unsigned cases=0;
 for(bool replace:{false,true})for(uint32_t bits:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
 unsigned char flags=0xff;HWND window=reinterpret_cast<HWND>(uintptr_t{0x100001234ull});UINT adapter=3;D3DPRESENT_PARAMETERS parameters{};
 NativeDeviceResetOwnerCandidate owner{flags,window,adapter,parameters};Device first{&parameters,static_cast<HRESULT>(bits)},second{&parameters,static_cast<HRESULT>(bits)};Device* current=&first;
 Lifecycle lifecycle{owner,current,second,replace};NativeDeviceResetServicesCandidateT<Lifecycle> services{lifecycle};unsigned descriptor=0;
 const bool success=studioDeviceResetCandidate(owner,descriptor,services);const bool expected=static_cast<HRESULT>(bits)>=0;
 require(success==expected&&flags==0xf7&&lifecycle.step==6);require(first.calls==(replace?0u:1u)&&second.calls==(replace?1u:0u));require(lifecycle.logs==(expected?0u:1u)&&lifecycle.posts==(expected?1u:0u));++cases;
 descriptor=2;flags=0xff;lifecycle.step=0;require(!studioDeviceResetCandidate(owner,descriptor,services)&&flags==0xf7&&lifecycle.step==0);++cases;
 }
 std::printf("typed_x64_reset_cases=%u fullwidth_hwnd_and_live_parameters=passed gpu_executed=0\n",cases);
 }catch(const std::exception& error){std::puts(error.what());return 1;}}
