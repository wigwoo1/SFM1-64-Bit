#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "constant_restore_defaults_native_init.h"
#include "studio_constant_restore_defaults_candidate.h"
#include "studio_vertex_constant_submit_candidate.h"
static std::array<uint32_t,4> trace;static unsigned calls;
struct Device{void** table;};static void* table[95]{};static Device device{table};
static HRESULT __stdcall direct(Device* self,uint32_t index,const uint32_t*,uint32_t count){if(self!=&device||count!=1||calls>=4)std::abort();trace[calls++]=index;return E_FAIL;}
#ifdef _M_IX86
static void __fastcall refill(void*,void*){std::abort();}
static void __fastcall cookie(uint32_t value,void*){if(value!=0x12345678)std::abort();}
#endif
struct Backend{
 uint32_t* first;uint32_t* second;bool queued;StudioDeviceQueueStateCandidate& state;unsigned unexpectedRefills=0;
 uint32_t* primaryCache(){return first;}uint32_t* secondaryCache(){return second;}
 bool queueEnabled(){return queued;}auto& queueState(){return state;}
 void publish(uint32_t*){++unexpectedRefills;}uint32_t* acquire(unsigned){++unexpectedRefills;return state.block;}
 void directVertex(uint32_t index,uint32_t count,const uint32_t* words){::direct(&device,index,words,count);}
 void submitVertex(unsigned index,const std::array<uint32_t,4>& words){studioVertexConstantSubmitCandidate(index,words,*this);}
};
int main(){
 const unsigned previousMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(restoreDefaultsBytes)> bytes{};dll.seekg(restoreDefaultsFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),restoreDefaultsBytes,bytes.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());static uint32_t cookieValue=0x12345678;
 for(auto& fixup:restoreDefaultsFixups){const void* target=nullptr;if(fixup[1]==0x1017c370)target=&cookieValue;else for(auto& constant:restoreDefaultsConstants)if(constant[0]==fixup[1])target=&constant[1];if(!target)return 3;const uint32_t address=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target));std::memcpy(code+fixup[0],&address,4);}
 for(auto& call:restoreDefaultsCalls){const uintptr_t target=call[1]==0x10002da0?reinterpret_cast<uintptr_t>(&refill):reinterpret_cast<uintptr_t>(&cookie);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,float)>(code);std::ofstream oracle("outputs/native-constant-restore-defaults.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-constant-restore-defaults.bin",std::ios::binary);
#endif
 table[94]=reinterpret_cast<void*>(&direct);unsigned cases=0;
 for(bool queued:{false,true})for(unsigned mask=0;mask<16;++mask)for(unsigned inputCase=0;inputCase<4;++inputCase){
  const uint32_t inputs[]={0x40000000,0xc0000000,0,0x7f800000},reciprocals[]={0x3f000000,0xbf000000,0x7f800000,0};const unsigned indices[]={0,1,3,225};
  const std::array<std::array<uint32_t,4>,4> values={{{0,0x3f800000,0x40000000,0x3f000000},{0x3ee8ba2e,inputs[inputCase],0x3eaaaaab,reciprocals[inputCase]},{0x41d80000,0x42000000,0x443f40a4,0x42680000},{0,0,0,0}}};std::array<uint32_t,904> first,second;first.fill(0x11111111);second.fill(0x22222222);for(unsigned phase=0;phase<4;++phase)if(mask&(1u<<phase))std::memcpy(first.data()+indices[phase]*4,values[phase].data(),16);std::array<uint32_t,64> block{};trace.fill(0);calls=0;uint32_t remaining=63,cursor=1;
#ifdef _M_IX86
  std::array<uint32_t,2500> owner{};owner[0x1710/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(first.data()));owner[0x2588/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(second.data()));owner[0x378/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&device));owner[0x388/4]=queued?1:0;owner[0x390/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data()+1));owner[0x394/4]=63;float input;std::memcpy(&input,&inputs[inputCase],4);native(owner.data(),input);remaining=owner[0x394/4];cursor=(owner[0x390/4]-static_cast<uint32_t>(reinterpret_cast<uintptr_t>(block.data())))/4;
#else
  StudioDeviceQueueStateCandidate state{block.data(),block.data()+1,63};Backend backend{first.data(),second.data(),queued,state};studioConstantRestoreDefaultsCandidate(inputs[inputCase],backend);if(backend.unexpectedRefills)return 8;remaining=state.remaining;cursor=static_cast<uint32_t>(state.cursor-block.data());
#endif
  std::array<uint32_t,1879> record{};record[0]=calls;record[1]=remaining;record[2]=cursor;std::memcpy(record.data()+3,trace.data(),16);std::memcpy(record.data()+7,first.data(),3616);std::memcpy(record.data()+911,second.data(),3616);std::memcpy(record.data()+1815,block.data(),256);
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));
#else
  std::array<uint32_t,1879> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(record!=expected){std::printf("mismatch queued=%u mask=%u input=%u\n",queued,mask,inputCase);return 5;}
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 _mm_setcsr(previousMxcsr);std::printf("constant_restore_defaults_native_cases=%u passed\n",cases);
}
