#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <iostream>
#include "studio_material_mesh_draw_candidate.h"
#include "material_draw_invoke_native_bytes.h"
struct Observation{uintptr_t receiver;int32_t first,count;unsigned calls;};
static Observation observed;
static void candidateMethod(uintptr_t receiver,int32_t first,int32_t count){observed={receiver,first,count,observed.calls+1};}
#ifndef _WIN64
static void __fastcall nativeMethod(void* receiver,void*,int32_t first,int32_t count){candidateMethod(reinterpret_cast<uintptr_t>(receiver),first,count);}
#endif
int main(){
#ifndef _WIN64
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 1;std::memcpy(code,drawInvokeBytes,24);DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 2;FlushInstructionCache(GetCurrentProcess(),code,24);
 using Invoke=void(__fastcall*)(void*,void*);auto native=reinterpret_cast<Invoke>(code);
#endif
 uint64_t digest=1469598103934665603ull;auto mix=[&](uint32_t word){digest^=word;digest*=1099511628211ull;};unsigned cases=0;
 for(unsigned i=0;i<65536;++i){
 uintptr_t initial=0x34560000u+i*16u;
#ifdef _WIN64
 initial+=0x123400000000ull;
#endif
 uintptr_t holder=initial;int32_t adjustment=int32_t(i%257)-128;
 int32_t first=int32_t(i*2654435761u),count=int32_t((i+1)*2246822519u);
 StudioMaterialMeshDrawCallCandidate call;call.receiverHolder=&holder;call.adjustment=adjustment;call.first=first;call.count=count;call.method=candidateMethod;
 holder+=0x1000;observed={};call.invoke();auto expected=observed;
 if(expected.receiver!=holder+uintptr_t(adjustment)||expected.first!=first||expected.count!=count||expected.calls!=1)return 3;
#ifndef _WIN64
 uint32_t object[10]={};object[4]=reinterpret_cast<uint32_t>(&nativeMethod);object[5]=uint32_t(adjustment);object[6]=reinterpret_cast<uint32_t>(&holder);object[8]=uint32_t(first);object[9]=uint32_t(count);
 observed={};native(object,nullptr);if(observed.receiver!=expected.receiver||observed.first!=first||observed.count!=count||observed.calls!=1)return 4;
#endif
 bool destroyed=false;call.releaseReference([&](auto* original,unsigned flags){if(original!=&call||flags)throw 1;destroyed=true;});if(!destroyed||call.references)return 5;
 mix(uint32_t(expected.receiver));mix(uint32_t(first));mix(uint32_t(count));++cases;
 }
#ifndef _WIN64
 VirtualFree(code,0,MEM_RELEASE);
#endif
 std::cout<<"draw_invoke_cases="<<cases<<" pointer_bits="<<sizeof(uintptr_t)*8<<" digest="<<digest<<" live_holder=1 signed_adjustment=1 arena_retention=1\n";
}
