#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "restore_blend_native_init.h"
static std::array<unsigned char,512> receiver;
static std::array<uint32_t,256> block,events;
static std::array<uint32_t,8> command;
static bool mutateFixture;
static unsigned used,getters,countReads,specialReads,gates;static int count;static unsigned special,suppression;
static void* capObject;static void* specialObject;static void* gateObject;static void* shaderObject;
static void event(unsigned op,unsigned a,unsigned b,unsigned c=0){if(used+4>events.size())std::abort();events[used++]=op;events[used++]=a;events[used++]=b;events[used++]=c;}
static HRESULT __stdcall render(void*,unsigned key,unsigned value){event(1,key,value);if(mutateFixture&&key==27){const uint32_t v=99;std::memcpy(receiver.data()+0x100,&v,4);}return E_FAIL;}
static HRESULT __stdcall sampler(void*,unsigned stage,unsigned type,unsigned value){event(17,stage,type,value);if(mutateFixture&&stage==0&&type==11){count=0;receiver[0x146]=1;}return E_FAIL;}
static int __fastcall samplerCount(void*,void*){++countReads;return count;}
static unsigned char __fastcall specialEnabled(void*,void*){++specialReads;return static_cast<unsigned char>(special);}
static unsigned char __fastcall suppressed(void*,void*){++gates;if(mutateFixture){const int16_t k=9;const uint32_t d=0x03112233;std::memcpy(receiver.data()+4,&k,2);std::memcpy(receiver.data()+8,&d,4);}return static_cast<unsigned char>(suppression);}
static void __fastcall vertex(void*,void*,int value){if(value!=-1)std::abort();event(200,9,static_cast<unsigned>(value));}
static void __fastcall pixel(void*,void*,int value){if(value!=-1)std::abort();event(200,10,static_cast<unsigned>(value));}
static void __fastcall transition(void* p,void*,int kind,unsigned offset,unsigned n){if(p!=receiver.data())std::abort();event(201,static_cast<unsigned>(kind),offset,n);}
static void __fastcall commit(void* p,void*){if(p!=receiver.data())std::abort();event(202,0,0);}
static void* __cdecl getter(){++getters;return command.data();}
static void __fastcall refill(void*,void*){std::abort();}
int main(){
 std::array<unsigned char,restoreBlendByteCount> saved{},installed{};std::ifstream artifact("outputs/restore-blend-verified-bytes.bin",std::ios::binary);artifact.read(reinterpret_cast<char*>(saved.data()),saved.size());if(!artifact||artifact.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(restoreBlendFileOffset);dll.read(reinterpret_cast<char*>(installed.data()),installed.size());if(!dll||installed!=saved)return 2;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,saved.data(),saved.size());
 for(auto& fix:restoreBlendFixups){void* cell=nullptr;switch(fix[1]){case 0x1016dd08:cell=&capObject;break;case 0x1019092c:cell=&specialObject;break;case 0x1016dd0c:cell=&gateObject;break;case 0x1016e20c:cell=&shaderObject;break;default:return 4;}const auto address=reinterpret_cast<uintptr_t>(cell);std::memcpy(code+fix[0],&address,4);}
 for(auto& call:restoreBlendCalls){void* target=nullptr;switch(call[1]){case 0x1000df50:target=reinterpret_cast<void*>(&getter);break;case 0x10002da0:target=reinterpret_cast<void*>(&refill);break;case 0x1002f2e0:target=reinterpret_cast<void*>(&transition);break;case 0x1002ebb0:target=reinterpret_cast<void*>(&commit);break;default:return 5;}const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0],"\xe8",1);std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 6;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);
 void* capTable[266]{};capTable[265]=reinterpret_cast<void*>(&samplerCount);void* cap=capTable;capObject=&cap;
 void* specialTable[50]{};specialTable[49]=reinterpret_cast<void*>(&specialEnabled);void* spec=specialTable;specialObject=&spec;
 void* gateTable[37]{};gateTable[36]=reinterpret_cast<void*>(&suppressed);void* gate=gateTable;gateObject=&gate;
 void* shaderTable[11]{};shaderTable[9]=reinterpret_cast<void*>(&vertex);shaderTable[10]=reinterpret_cast<void*>(&pixel);void* shaders=shaderTable;shaderObject=&shaders;
 void* deviceTable[70]{};deviceTable[57]=reinterpret_cast<void*>(&render);deviceTable[69]=reinterpret_cast<void*>(&sampler);void* device=deviceTable;
 std::ofstream oracle("outputs/native-restore-blend.bin",std::ios::binary|std::ios::trunc);unsigned cases=0;
 for(bool change:{false,true})for(bool queue:{false,true})for(int n:{-1,0,1,2})for(unsigned specValue:{0u,1u,256u})for(unsigned suppressValue:{0u,1u,256u}){
  mutateFixture=change;receiver.fill(0x55);const int16_t kind=-2;std::memcpy(receiver.data()+4,&kind,2);const uint32_t descriptor=0x02123456;std::memcpy(receiver.data()+8,&descriptor,4);receiver[0x145]=17;receiver[0x146]=0;receiver[0x148]=19;receiver[0x149]=1;block.fill(99);events.fill(0);used=getters=countReads=specialReads=gates=0;count=n;special=specValue;suppression=suppressValue;
  command.fill(0);command[0]=reinterpret_cast<uintptr_t>(&device);command[4]=queue?1:0;command[6]=reinterpret_cast<uintptr_t>(block.data());command[7]=256;native(receiver.data());
  const unsigned stages=n>0?static_cast<unsigned>(n):0u;const unsigned submissions=13+stages*(static_cast<unsigned char>(special)?2u:1u);if(getters!=submissions||countReads!=1||specialReads!=stages||gates!=1)return 7;
  std::array<uint32_t,647> record{};std::memcpy(record.data(),receiver.data(),receiver.size());std::memcpy(record.data()+128,block.data(),sizeof(block));std::memcpy(record.data()+384,events.data(),sizeof(events));record[640]=used;record[641]=getters;record[642]=countReads;record[643]=specialReads;record[644]=gates;record[645]=(command[6]-reinterpret_cast<uintptr_t>(block.data()))/4;record[646]=command[7];oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 8;++cases;
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("restore_blend_original_cases=%u passed\n",cases);return 0;
}

