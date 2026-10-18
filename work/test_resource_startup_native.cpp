#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "resource_startup_native_init.h"
static void check(bool b){if(!b)std::abort();}
static uintptr_t resource,precache,first,second,head;
static bool initialized,registered,mutate;
static std::array<uintptr_t,4> a,b;
static std::array<uintptr_t,5> groups;
static std::vector<unsigned> events;
static void __fastcall groupCall(void* p,void*,void* group,unsigned i){check(reinterpret_cast<uintptr_t>(p)==((mutate&&i)?second:first)&&reinterpret_cast<uintptr_t>(group)==groups[i]);events.push_back(10+i);if(mutate)precache=second;}
static void __fastcall registerCall(void* p,void*,void* token){check(reinterpret_cast<uintptr_t>(p)==first&&reinterpret_cast<uintptr_t>(token)==second&&!registered);events.push_back(20);if(mutate)resource=second;}
static void __fastcall listCall(void* p,void*,void* node){check(reinterpret_cast<uintptr_t>(p)==(mutate?second:first)&&registered&&(node==a.data()||node==b.data()));events.push_back(node==a.data()?30:31);if(mutate&&node==a.data())a[3]=0;}
int main(){static_assert(sizeof(void*)==4,"originalx86");std::array<uintptr_t,13> table{};table[8]=reinterpret_cast<uintptr_t>(&registerCall);table[12]=reinterpret_cast<uintptr_t>(&listCall);table[10]=reinterpret_cast<uintptr_t>(&groupCall);uintptr_t one=reinterpret_cast<uintptr_t>(table.data()),two=one;first=reinterpret_cast<uintptr_t>(&one);second=reinterpret_cast<uintptr_t>(&two);groups.fill(second);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 for(unsigned i=0;i<3;++i){const auto& body=resourceStartupBodies[i];auto target=code+resourceStartupAddressMap[i][1];dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(target),body.size);check(bool(dll)&&!std::memcmp(target,body.bytes,body.size));for(unsigned n=0;n<body.fixupCount;++n){auto fix=body.fixups[n];uintptr_t p=0;switch(fix[1]){case 0x10190904:p=reinterpret_cast<uintptr_t>(&resource);break;case 0x1019094c:p=reinterpret_cast<uintptr_t>(&precache);break;case 0x101b11fd:p=reinterpret_cast<uintptr_t>(&initialized);break;case 0x101b11fc:p=reinterpret_cast<uintptr_t>(&registered);break;case 0x101b1200:p=second;break;case 0x101b122c:p=reinterpret_cast<uintptr_t>(&head);break;case 0x101b124c:p=reinterpret_cast<uintptr_t>(groups.data());break;default:std::abort();}unsigned value=static_cast<unsigned>(p);std::memcpy(target+fix[0],&value,4);}for(unsigned n=0;n<body.callCount;++n){auto call=body.calls[n];unsigned char* destination=nullptr;for(auto map:resourceStartupAddressMap)if(map[0]==call[1])destination=code+map[1];check(destination);unsigned delta=static_cast<unsigned>(reinterpret_cast<uintptr_t>(destination)-reinterpret_cast<uintptr_t>(target+call[0]+5));std::memcpy(target+call[0]+1,&delta,4);}}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto connect=reinterpret_cast<void(__cdecl*)(void*,int)>(code);unsigned cases=0;std::ofstream output("outputs/native-resource-startup.bin",std::ios::binary|std::ios::trunc);
 for(bool presentPrecache:{false,true})for(bool initial:{false,true})for(bool presentResource:{false,true})for(bool initialRegistered:{false,true})for(bool mutation:{false,true}){precache=presentPrecache?first:0;resource=presentResource?first:0;initialized=initial;registered=initialRegistered;mutate=mutation;a.fill(0xabababab);b.fill(0xabababab);a[3]=reinterpret_cast<uintptr_t>(b.data());b[3]=0;head=reinterpret_cast<uintptr_t>(a.data());auto originalGroups=groups;events.clear();connect(nullptr,1);std::vector<unsigned> expected;if(presentPrecache&&!initial)for(unsigned i=0;i<5;++i)expected.push_back(10+i);if(presentResource&&!initialRegistered){expected.push_back(20);expected.push_back(30);if(!mutation)expected.push_back(31);}check(events==expected&&initialized==(initial||presentPrecache)&&registered==(initialRegistered||presentResource)&&groups==originalGroups&&head==reinterpret_cast<uintptr_t>(a.data()));bool traversed=presentResource&&!initialRegistered;check(a[3]==((traversed&&mutation)?0:reinterpret_cast<uintptr_t>(b.data()))&&b[3]==0);for(unsigned j=0;j<3;++j)check(a[j]==0xabababab&&b[j]==0xabababab);auto tag=[](uintptr_t p){check(p==0||p==first||p==second);return p==first?1u:p==second?2u:0u;};std::array<unsigned,16> wire{};wire[0]=static_cast<unsigned>(events.size());std::copy(events.begin(),events.end(),wire.begin()+1);wire[9]=unsigned(initialized);wire[10]=unsigned(registered);wire[11]=tag(precache);wire[12]=tag(resource);wire[13]=a[3]?1:0;output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));++cases;}
 check(VirtualFree(code,0,MEM_RELEASE));std::printf("original_resource_startup_graph_cases=%u passed\n",cases);
}
