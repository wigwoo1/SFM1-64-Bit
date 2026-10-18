#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "default_render_state_native_init.h"
#include "studio_default_render_state_owner_candidate.h"
#include "render_state_restore_native_init.h"
static unsigned liveMutation;static uint32_t* nativeQueueFlag;static bool unexpectedRefill;static unsigned suppression;static bool queuedMode;static HRESULT __stdcall directState(void*,unsigned key,unsigned value);
static std::array<unsigned char,0x600> capabilityA{},capabilityB{};static void* currentCapability;static bool replace;static std::vector<uint32_t> trace;
static void submit(unsigned key,unsigned value){trace.push_back(key);trace.push_back(value);if(trace.size()==86&&replace)currentCapability=capabilityB.data();}
static HRESULT __stdcall directState(void*,unsigned key,unsigned value){submit(key,value);if(trace.size()==86){if(liveMutation==1){queuedMode=true;if(nativeQueueFlag)*nativeQueueFlag=1;}else if(liveMutation==2)suppression=1;}return E_FAIL;}
#ifdef _M_IX86
static unsigned char __fastcall gate(void*,void*){return static_cast<unsigned char>(suppression);}
#endif
struct Services{unsigned suppressed(){return suppression;}bool queueEnabled(){return queuedMode;}void directState(unsigned key,unsigned value){::directState(nullptr,key,value);}void flush(){unexpectedRefill=true;}void publish(uint32_t*){unexpectedRefill=true;}uint32_t* acquire(unsigned){unexpectedRefill=true;return nullptr;}void* capability(){return currentCapability;}unsigned flags(void* value){return static_cast<unsigned char*>(value)[0x5a9];}unsigned vendor(void* value){uint32_t result;std::memcpy(&result,static_cast<unsigned char*>(value)+0x4dc,4);return result;}};
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeDefaultRenderStateBytes)> bytes{};dll.seekg(nativeDefaultRenderStateOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeDefaultRenderStateBytes,bytes.size()))return 1;std::array<unsigned char,sizeof(nativeRenderStateRestoreBytes)> setterBytes{};dll.seekg(nativeRenderStateRestoreOffset);dll.read(reinterpret_cast<char*>(setterBytes.data()),setterBytes.size());if(!dll||std::memcmp(setterBytes.data(),nativeRenderStateRestoreBytes,setterBytes.size()))return 7;auto* setter=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!setter)return 8;std::memcpy(setter,setterBytes.data(),setterBytes.size());setter[54]=0xcc;DWORD setterOld;if(!VirtualProtect(setter,4096,PAGE_EXECUTE_READ,&setterOld))return 9;FlushInstructionCache(GetCurrentProcess(),setter,4096);auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());uint32_t one=0x3f800000,other;// Constants below follow the installed-byte sequence audit.
 other=studioDefaultRenderStateEntriesCandidate[55].value;
 for(auto& fix:nativeDefaultRenderStateFixups){uintptr_t target=fix[1]==0x1016d130?reinterpret_cast<uintptr_t>(&currentCapability):fix[1]==0x100fa5f8?reinterpret_cast<uintptr_t>(&one):reinterpret_cast<uintptr_t>(&other);std::memcpy(code+fix[0],&target,4);}for(auto& call:nativeDefaultRenderStateCalls){uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(setter)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-default-state-composition.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-default-state-composition.bin",std::ios::binary);
#endif
 unsigned cases=0;for(unsigned change:{0u,1u,2u})for(unsigned suppress:{0u,1u,256u})for(bool queued:{false,true})for(unsigned flags:{0u,0x10u,0x20u,0x30u})for(uint32_t vendor:{0x1002u,0x10deu})for(bool mutation:{false,true}){capabilityA.fill(0);capabilityB.fill(0);capabilityA[0x5a9]=static_cast<unsigned char>(flags);capabilityB[0x5a9]=0x20;std::memcpy(capabilityA.data()+0x4dc,&vendor,4);currentCapability=capabilityA.data();replace=mutation;liveMutation=change;nativeQueueFlag=nullptr;unexpectedRefill=false;trace.clear();suppression=suppress;queuedMode=queued;std::array<uint32_t,256> cache{};cache.fill(99);std::array<uint32_t,512> block{};block.fill(99);unsigned cursor=0,remaining=512;
#ifdef _M_IX86
 void* ownerTable[37]{};ownerTable[36]=reinterpret_cast<void*>(&gate);void* deviceTable[58]{};deviceTable[57]=reinterpret_cast<void*>(&directState);void* device=deviceTable;std::array<uint32_t,4096> owner{};owner[0]=reinterpret_cast<uintptr_t>(ownerTable);owner[0x648/4]=reinterpret_cast<uintptr_t>(&device);owner[0x658/4]=queued?1:0;owner[0x660/4]=reinterpret_cast<uintptr_t>(block.data());owner[0x664/4]=512;std::memcpy(owner.data()+0x1c98/4,cache.data(),sizeof(cache));nativeQueueFlag=&owner[0x658/4];native(owner.data());if((owner[0x658/4]!=0)!=queuedMode)return 11;std::memcpy(cache.data(),owner.data()+0x1c98/4,sizeof(cache));cursor=(owner[0x660/4]-reinterpret_cast<uintptr_t>(block.data()))/4;remaining=owner[0x664/4];
#else
 StudioDeviceQueueStateCandidate queue{block.data(),block.data(),512};StudioRenderStateOwnerCandidate owner{cache.data(),&queue};Services services;studioDefaultRenderStateOwnerCandidate(owner,services);cursor=static_cast<unsigned>(queue.cursor-block.data());remaining=queue.remaining;
#endif
 if(unexpectedRefill)return 10;if(change&&!queued&&static_cast<uint8_t>(suppress)==0){if(trace.size()!=86)return 12;if(change==1&&(!queuedMode||cursor==0))return 13;if(change==2&&(suppression!=1||cursor!=0))return 14;}std::array<uint32_t,956> record{static_cast<uint32_t>(trace.size()),currentCapability==capabilityB.data()?1u:0u};for(unsigned i=0;i<trace.size();++i)record[2+i]=trace[i];record[184]=cursor;record[185]=remaining;std::memcpy(record.data()+186,cache.data(),sizeof(cache));std::memcpy(record.data()+442,block.data(),sizeof(block));
#ifdef _M_IX86
 oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
 std::array<uint32_t,956> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected)return 4;
#endif
 if(!oracle)return 5;++cases;}
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);VirtualFree(setter,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("default_state_composition_native_cases=%u passed\n",cases);
}



