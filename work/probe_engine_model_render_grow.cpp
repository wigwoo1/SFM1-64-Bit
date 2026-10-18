#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "model_render_record_grow_candidate.h"
#ifndef _WIN64
#include "engine_model_render_grow_native_init.h"
static uint32_t *observed,*rawPool,returned;
static void* observe(uint32_t operation,void* old,uint32_t bytes){
 observed[2]=operation;observed[3]=bytes;observed[4]=uint32_t(reinterpret_cast<uintptr_t>(old));observed[5]=rawPool[1];++observed[6];return reinterpret_cast<void*>(returned);
}
static void* __fastcall allocate(void*,void*,uint32_t bytes){return observe(1,nullptr,bytes);}
static void* __fastcall reallocate(void*,void*,void* old,uint32_t bytes){return observe(3,old,bytes);}
#endif
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("oracle path");
 std::vector<int32_t> capacities;for(int n=0;n<=65535;n+=257)capacities.push_back(n);for(int n:{32767,32768,32769,65534})capacities.push_back(n);
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);
#else
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,modelRenderGrowNativeAllocationSize,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("allocate code");
 uint32_t vtable[4]={0,uint32_t(reinterpret_cast<uintptr_t>(&allocate)),0,uint32_t(reinterpret_cast<uintptr_t>(&reallocate))};
 uint32_t allocatorObject=uint32_t(reinterpret_cast<uintptr_t>(vtable)),allocatorVariable=uint32_t(reinterpret_cast<uintptr_t>(&allocatorObject));
 initializeModelRenderGrowNative(code,uint32_t(reinterpret_cast<uintptr_t>(&allocatorVariable)));DWORD oldProtection;
 if(!VirtualProtect(code,modelRenderGrowNativeAllocationSize,PAGE_EXECUTE_READ,&oldProtection))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,modelRenderGrowNativeAllocationSize);
 using Grow=void(__thiscall*)(void*,int32_t);auto grow=reinterpret_cast<Grow>(code);std::ofstream f(argv[1],std::ios::binary);
#endif
 unsigned count=0;
 for(auto capacity:capacities)for(int32_t growth:{-1,0,1,2,3,127,1024,65535})for(int32_t additional:{1,16,65536})for(unsigned mode=0;mode<4;++mode){
  uint32_t state[8]={};uint32_t oldPointer=(mode&1)?0x12345678u:0,newPointer=(mode&2)?0x23456780u:0;
#ifdef _WIN64
  EngineShadowPoolStateCandidate pool;pool.capacity=capacity;pool.links=reinterpret_cast<EngineShadowPoolLinkCandidate*>(uintptr_t(oldPointer));
  modelRenderGrowCandidate(pool,growth,additional,[&](auto* previous,int32_t selected){state[2]=previous?3u:1u;state[3]=uint32_t(selected)*264u;state[4]=uint32_t(reinterpret_cast<uintptr_t>(previous));state[5]=uint32_t(pool.capacity);++state[6];return reinterpret_cast<EngineShadowPoolLinkCandidate*>(uintptr_t(newPointer));});
  state[0]=uint32_t(pool.capacity);state[1]=uint32_t(reinterpret_cast<uintptr_t>(pool.links));state[7]=uint32_t(growth);
  uint32_t expected[8];if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected)))throw std::runtime_error("truncated oracle");if(std::memcmp(state,expected,sizeof(state)))throw std::runtime_error("native growth mismatch case "+std::to_string(count)+" capacity "+std::to_string(capacity)+" growth "+std::to_string(growth)+" additional "+std::to_string(additional));
#else
  uint32_t raw[3]={oldPointer,uint32_t(capacity),uint32_t(growth)};observed=state;rawPool=raw;returned=newPointer;grow(raw,additional);state[0]=raw[1];state[1]=raw[0];state[7]=raw[2];f.write(reinterpret_cast<char*>(state),sizeof(state));
#endif
  ++count;
 }
#ifdef _WIN64
 char extra;if(f.read(&extra,1)||!f.eof())throw std::runtime_error("oracle trailing data");
#else
 f.flush();if(!f)throw std::runtime_error("write oracle");VirtualFree(code,0,MEM_RELEASE);
#endif
 if(count!=24960)throw std::runtime_error("case count");std::cout<<"grow_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
