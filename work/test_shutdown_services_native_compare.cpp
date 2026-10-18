#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "studio_shutdown_services_candidate.h"
#if !defined(_WIN64)
#include "shutdown_services_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
static StudioShutdownServicesCandidate state;
static uintptr_t first,second;static unsigned mode,resourceCalls,cvarTrace;
static void __fastcall resourceCallback(void* receiver,void*,void* token){check(reinterpret_cast<uintptr_t>(receiver)==first&&reinterpret_cast<uintptr_t>(token)==second);++resourceCalls;state.resource=second;state.resourceRegistered=true;}
static void __fastcall cvarFirst(void* receiver,void*,unsigned handle){check(reinterpret_cast<uintptr_t>(receiver)==first&&handle==7);cvarTrace=32;if(mode&1)state.cvar=second;if(mode&2)state.cvarHandle=9;}
static void __fastcall cvarSecond(void* receiver,void*,unsigned handle){check(reinterpret_cast<uintptr_t>(receiver)==((mode&1)?second:first)&&handle==((mode&2)?9u:7u));cvarTrace=3211;state.cvarHandle=99;state.cvarRegistered=true;}
struct Ops {
 void resourceSlot9(uintptr_t p,uintptr_t token){resourceCallback(reinterpret_cast<void*>(p),nullptr,reinterpret_cast<void*>(token));}
 void cvarSlot32(uintptr_t p,unsigned h){cvarFirst(reinterpret_cast<void*>(p),nullptr,h);}
 void cvarSlot11(uintptr_t p,unsigned h){cvarSecond(reinterpret_cast<void*>(p),nullptr,h);}
};
int main(){std::array<uintptr_t,33> table{};table[9]=reinterpret_cast<uintptr_t>(&resourceCallback);table[32]=reinterpret_cast<uintptr_t>(&cvarFirst);table[11]=reinterpret_cast<uintptr_t>(&cvarSecond);uintptr_t a=reinterpret_cast<uintptr_t>(table.data()),b=a;first=reinterpret_cast<uintptr_t>(&a);second=reinterpret_cast<uintptr_t>(&b);unsigned cases=0;
#if !defined(_WIN64)
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 auto load=[&](unsigned position,unsigned offset,const unsigned char* bytes,unsigned count,const unsigned (*fixups)[2],unsigned fixCount){dll.seekg(offset);dll.read(reinterpret_cast<char*>(code+position),count);check(bool(dll)&&!std::memcmp(code+position,bytes,count));for(unsigned i=0;i<fixCount;++i){uintptr_t p=0;switch(fixups[i][1]){case 0x10190904:p=reinterpret_cast<uintptr_t>(&state.resource);break;case 0x101b11fc:p=reinterpret_cast<uintptr_t>(&state.resourceRegistered);break;case 0x101b1200:p=second;break;case 0x101908e4:p=reinterpret_cast<uintptr_t>(&state.cvar);break;case 0x1019fcf4:p=reinterpret_cast<uintptr_t>(&state.cvarRegistered);break;case 0x1017b7f0:p=reinterpret_cast<uintptr_t>(&state.cvarHandle);break;default:std::abort();}unsigned value=static_cast<unsigned>(p);std::memcpy(code+position+fixups[i][0],&value,4);}};
 load(0,resourceOffset,resourceBytes,sizeof(resourceBytes),resourceFixups,sizeof(resourceFixups)/sizeof(resourceFixups[0]));load(128,cvarOffset,cvarBytes,sizeof(cvarBytes),cvarFixups,sizeof(cvarFixups)/sizeof(cvarFixups[0]));DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto resource=reinterpret_cast<void(__cdecl*)()>(code),cvar=reinterpret_cast<void(__cdecl*)()>(code+128);std::ofstream output("outputs/native-shutdown-services.bin",std::ios::binary|std::ios::trunc);
#else
 check(first>UINT32_MAX&&second>UINT32_MAX);std::ifstream input("outputs/native-shutdown-services.bin",std::ios::binary);Ops ops;
#endif
 for(bool present:{false,true})for(bool registered:{false,true})for(unsigned mutation:{0u,1u,2u,3u}){mode=mutation;resourceCalls=cvarTrace=0;state={present?first:0,present?first:0,second,7,registered,registered};
#if !defined(_WIN64)
 resource();cvar();
#else
 studioResourceShutdownCandidate(state,ops);studioCvarShutdownCandidate(state,ops);
#endif
 bool invoked=present&&registered;check(resourceCalls==unsigned(invoked)&&cvarTrace==(invoked?3211u:0u));check(state.resource==(invoked?second:(present?first:0))&&state.cvar==((invoked&&(mode&1))?second:(present?first:0)));check(state.resourceRegistered==(registered&&!present)&&state.cvarRegistered==(registered&&!present)&&state.cvarHandle==(invoked?0xffffffffu:7u)&&state.resourceToken==second);
 auto tag=[](uintptr_t p){check(p==0||p==first||p==second);return p==first?1u:p==second?2u:0u;};std::array<unsigned,7> wire{tag(state.resource),tag(state.cvar),unsigned(state.resourceRegistered),unsigned(state.cvarRegistered),state.cvarHandle,resourceCalls,cvarTrace};
#if !defined(_WIN64)
 output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));
#else
 std::array<unsigned,7> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);
#endif
 ++cases;}
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("shutdown_service_native_comparison_%zu_bit_cases=%u passed\n",sizeof(void*)*8,cases);
}
