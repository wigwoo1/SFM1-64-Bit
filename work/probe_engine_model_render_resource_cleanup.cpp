#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include "model_render_record_resource_cleanup_candidate.h"
#ifndef _WIN64
#include "engine_model_render_resource_cleanup_native_bytes.h"
static uint32_t *observation,*owner;static unsigned char* replacementBase;static bool replaceStorage;
static void __fastcall release(void*,void*,uint32_t resource,uint32_t a,uint32_t b,uint32_t c){++observation[0];observation[1]=resource;observation[2]=a;observation[3]=b;observation[4]=c;if(replaceStorage)owner[3]=uint32_t(reinterpret_cast<uintptr_t>(replacementBase));}
#endif
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("oracle path");unsigned count=0;
#ifdef _WIN64
 std::ifstream f(argv[1],std::ios::binary);std::vector<ModelRenderRecordResourceCandidate> original(65536),replacement(65536);
#else
 std::ofstream f(argv[1],std::ios::binary);std::vector<std::array<unsigned char,264>> original(65536),replacement(65536);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(modelRenderResourceCleanupNativeBytes),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)throw std::runtime_error("code allocation");std::memcpy(code,modelRenderResourceCleanupNativeBytes,sizeof(modelRenderResourceCleanupNativeBytes));DWORD protection;if(!VirtualProtect(code,sizeof(modelRenderResourceCleanupNativeBytes),PAGE_EXECUTE_READ,&protection))throw std::runtime_error("protect");FlushInstructionCache(GetCurrentProcess(),code,sizeof(modelRenderResourceCleanupNativeBytes));using Cleanup=void(__thiscall*)(void*,uint32_t);auto cleanup=reinterpret_cast<Cleanup>(code);
 uint32_t vtable[9]={};vtable[8]=uint32_t(reinterpret_cast<uintptr_t>(&release));uint32_t service=uint32_t(reinterpret_cast<uintptr_t>(vtable));
#endif
 for(unsigned index=0;index<65536;++index)for(unsigned mode=0;mode<4;++mode){uint32_t state[7]={};uint32_t value=(mode&1)?0x12345678u:0;uint32_t replacementValue=0x87654321u;
#ifdef _WIN64
  original[index].resource=value;replacement[index].resource=replacementValue;auto* live=original.data();
  modelRenderRecordResourceCleanupCandidate(0xabcd0000u|index,[&](uint16_t selected)->ModelRenderRecordResourceCandidate&{return live[selected];},[&](uintptr_t resource,int a,int b,int c){++state[0];state[1]=uint32_t(resource);state[2]=a;state[3]=b;state[4]=c;if(mode&2)live=replacement.data();});state[5]=uint32_t(original[index].resource);state[6]=uint32_t(replacement[index].resource);
  uint32_t expected[7];if(!f.read(reinterpret_cast<char*>(expected),sizeof(expected)))throw std::runtime_error("truncated oracle");if(std::memcmp(state,expected,sizeof(state)))throw std::runtime_error("cleanup mismatch "+std::to_string(count));
#else
  original[index].fill(0xa5);replacement[index].fill(0x5a);std::memcpy(original[index].data()+256,&value,4);std::memcpy(replacement[index].data()+256,&replacementValue,4);
  uint32_t rawOwner[4]={0,0,uint32_t(reinterpret_cast<uintptr_t>(&service)),uint32_t(reinterpret_cast<uintptr_t>(original.data()))};owner=rawOwner;observation=state;replacementBase=reinterpret_cast<unsigned char*>(replacement.data());replaceStorage=(mode&2)!=0;cleanup(rawOwner,0xabcd0000u|index);
  std::memcpy(state+5,original[index].data()+256,4);std::memcpy(state+6,replacement[index].data()+256,4);
  for(unsigned j=0;j<264;++j)if(j<256||j>=260){if(original[index][j]!=0xa5||replacement[index][j]!=0x5a)throw std::runtime_error("native changed unrelated payload");}
  f.write(reinterpret_cast<char*>(state),sizeof(state));
#endif
  ++count;
 }
#ifdef _WIN64
 char extra;if(f.read(&extra,1)||!f.eof())throw std::runtime_error("trailing oracle");
#else
 f.flush();if(!f)throw std::runtime_error("oracle write");VirtualFree(code,0,MEM_RELEASE);
#endif
 if(count!=262144)throw std::runtime_error("count");std::cout<<"cleanup_cases="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
