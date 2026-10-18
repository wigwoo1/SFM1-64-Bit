#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "scalar_constant_native_init.h"
#include "studio_scalar_constant_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
static unsigned calls;static uint32_t* replacement;static uint32_t** livePrimary;
struct Device{void** table;};static void* table[114]{};static Device device{table};
static HRESULT __stdcall direct(Device* self,uint32_t start,const uint32_t*,uint32_t){if(self!=&device||start!=1)std::abort();++calls;*livePrimary=replacement;return E_FAIL;}
#ifdef _M_IX86
static void* __cdecl copy(void* destination,const void* source,uint32_t bytes){return std::memmove(destination,source,bytes);}
static void __fastcall forbiddenRefill(void*,void*){std::abort();}
#endif
struct Backend{
 uint32_t*& first;uint32_t* second;bool queued;StudioDeviceQueueStateCandidate& queue;
 uint32_t* primary(unsigned){return first;}uint32_t* secondary(unsigned){return second;}
 bool queueEnabled(){return queued;}auto& queueState(){return queue;}
 void refill(StudioDeviceQueueStateCandidate&){std::abort();}
 void direct(unsigned,uint32_t start,const uint32_t* data,uint32_t count){::direct(&device,start,data,count);}
};
int main(){
#ifdef _M_IX86
 std::array<unsigned char*,4> code{};std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 for(unsigned i=0;i<4;++i){const auto& body=scalarBodies[i];std::array<unsigned char,512> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),body.size);if(!dll||std::memcmp(bytes.data(),body.bytes,body.size))return 1;code[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code[i])return 2;std::memcpy(code[i],body.bytes,body.size);
  for(unsigned c=0;c<body.callCount;++c){const auto& call=body.calls[c];const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(&forbiddenRefill);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code[i]+call[0]+5));std::memcpy(code[i]+call[0]+1,&delta,4);}DWORD previous;if(!VirtualProtect(code[i],4096,PAGE_EXECUTE_READ,&previous))return 3;FlushInstructionCache(GetCurrentProcess(),code[i],4096);
 }
 std::ofstream oracle("outputs/native-scalar-constant.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-scalar-constant.bin",std::ios::binary);
#endif
 for(unsigned slot:{98u,96u,113u,111u})table[slot]=reinterpret_cast<void*>(&direct);
 unsigned cases=0;
 for(unsigned k=0;k<4;++k)for(bool queued:{false,true})for(uint32_t force:{0u,1u,256u})for(bool same:{false,true})for(uint32_t count:{0u,1u,2u}){
  const auto kind=studioScalarConstantSubmissionsCandidate[k];std::array<uint32_t,24> first,second,changed;first.fill(0x11111111);second.fill(0x22222222);changed.fill(0x33333333);std::array<uint32_t,8> source;source.fill(0x76543210);std::array<uint32_t,64> block{};const unsigned words=count*kind.wordsPerRegister;if(same)std::memcpy(second.data()+kind.wordsPerRegister,source.data(),words*4);replacement=changed.data();calls=0;uint32_t* current=first.data();uint32_t remaining=63,cursor=1;
#ifdef _M_IX86
  std::array<uint32_t,2500> owner{};const unsigned primaryOffset[]={0x1714,0x1718,0x1720,0x1724};const unsigned secondaryOffset[]={0x258c,0x2590,0x2598,0x259c};livePrimary=reinterpret_cast<uint32_t**>(owner.data()+primaryOffset[k]/4);*livePrimary=current;owner[secondaryOffset[k]/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(second.data()));owner[0x378/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&device));owner[0x388/4]=queued?1:0;owner[0x390/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data()+1));owner[0x394/4]=63;
  auto native=reinterpret_cast<void(__thiscall*)(void*,uint32_t,const uint32_t*,uint32_t,uint32_t)>(code[k]);native(owner.data(),1,source.data(),count,force);current=*livePrimary;remaining=owner[0x394/4];cursor=(owner[0x390/4]-static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data())))/4;
#else
  livePrimary=&current;StudioDeviceQueueStateCandidate queue{block.data(),block.data()+1,63};Backend backend{current,second.data(),queued,queue};studioScalarConstantSubmitCandidate(kind,1,source.data(),count,force,backend);remaining=queue.remaining;cursor=static_cast<uint32_t>(queue.cursor-block.data());
#endif
  std::array<uint32_t,140> record{};record[0]=calls;record[1]=remaining;record[2]=cursor;record[3]=current==changed.data()?1:0;std::memcpy(record.data()+4,first.data(),96);std::memcpy(record.data()+28,changed.data(),96);std::memcpy(record.data()+52,second.data(),96);std::memcpy(record.data()+76,block.data(),256);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,140> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch k=%u queued=%u force=%u same=%u count=%u\n",k,queued,force,same,count);return 4;}
#endif
  if(!oracle)return 5;++cases;
 }
#ifdef _M_IX86
 for(auto* body:code)VirtualFree(body,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 6;
#endif
 std::printf("scalar_constant_native_cases=%u passed\n",cases);
}
