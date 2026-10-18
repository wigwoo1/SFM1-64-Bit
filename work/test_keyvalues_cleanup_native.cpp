#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "keyvalues_cleanup_native_init.h"
static void check(bool b){if(!b)std::abort();}
static std::array<std::array<unsigned,7>,4> nodes;
static std::array<unsigned,8> payloads;
static std::vector<unsigned> events;
static void* nodeService;
static void* __cdecl getNodeService(){return nodeService;}
static void __fastcall freeNode(void*,void*,void* p){for(unsigned i=0;i<4;++i)if(p==nodes[i].data()){check(nodes[i][1]==0&&nodes[i][2]==0);events.push_back(200+i);return;}std::abort();}
static void __fastcall freePayload(void*,void*,void* p){if(!p){events.push_back(0);return;}for(unsigned i=0;i<8;++i)if(p==&payloads[i]){events.push_back(100+i);return;}std::abort();}
int main(){static_assert(sizeof(void*)==4,"originalx86");std::array<uintptr_t,3> nodeTable{0,0,reinterpret_cast<uintptr_t>(&freeNode)};uintptr_t nodeObject=reinterpret_cast<uintptr_t>(nodeTable.data());nodeService=&nodeObject;std::array<uintptr_t,6> payloadTable{};payloadTable[5]=reinterpret_cast<uintptr_t>(&freePayload);uintptr_t payloadObject=reinterpret_cast<uintptr_t>(payloadTable.data());void* allocator=&payloadObject;auto* allocatorVariable=&allocator;auto getter=&getNodeService;
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 for(unsigned i=0;i<3;++i){const auto& body=keyValuesCleanupBodies[i];auto target=code+512*i;dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(target),body.size);check(bool(dll)&&!std::memcmp(target,body.bytes,body.size));for(unsigned n=0;n<body.fixupCount;++n){auto fix=body.fixups[n];check(fix[1]==0x100fa274||fix[1]==0x100fa238);unsigned p=static_cast<unsigned>(fix[1]==0x100fa274?reinterpret_cast<uintptr_t>(&getter):reinterpret_cast<uintptr_t>(&allocatorVariable));std::memcpy(target+fix[0],&p,4);}for(unsigned n=0;n<body.callCount;++n){auto call=body.calls[n];check(call[1]==0x100d8570||call[1]==0x100039b0);auto destination=code+(call[1]==0x100d8570?512:1024);unsigned delta=static_cast<unsigned>(reinterpret_cast<uintptr_t>(destination)-reinterpret_cast<uintptr_t>(target+call[0]+5));std::memcpy(target+call[0]+1,&delta,4);}}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto destroy=reinterpret_cast<void(__thiscall*)(void*)>(code);unsigned cases=0;std::ofstream output("outputs/native-keyvalues-cleanup.bin",std::ios::binary|std::ios::trunc);
 for(unsigned mask:{0u,1u,2u,3u})for(bool self:{false,true}){nodes={};events.clear();for(unsigned i=0;i<4;++i){nodes[i][1]=(mask&1)?static_cast<unsigned>(reinterpret_cast<uintptr_t>(&payloads[2*i])):0;nodes[i][2]=(mask&2)?static_cast<unsigned>(reinterpret_cast<uintptr_t>(&payloads[2*i+1])):0;}auto pointer=[](unsigned i){return static_cast<unsigned>(reinterpret_cast<uintptr_t>(nodes[i].data()));};nodes[0][6]=pointer(1);nodes[1][5]=pointer(2);nodes[0][5]=pointer(self?0:3);auto before=nodes;destroy(nodes[0].data());std::vector<unsigned> expected;auto add=[&](unsigned i){expected.push_back((mask&1)?100+2*i:0);expected.push_back((mask&2)?101+2*i:0);expected.push_back(200+i);};add(1);add(2);if(!self)add(3);add(0);check(events==expected);for(unsigned i=0;i<4;++i){bool visited=i!=3||!self;check(nodes[i][1]==(visited?0:before[i][1])&&nodes[i][2]==(visited?0:before[i][2]));check(nodes[i][0]==before[i][0]&&nodes[i][3]==before[i][3]&&nodes[i][4]==before[i][4]&&nodes[i][6]==before[i][6]);check(nodes[i][5]==(i==1?0:before[i][5]));}std::array<unsigned,16> wire{};wire[0]=static_cast<unsigned>(events.size());std::copy(events.begin(),events.end(),wire.begin()+1);output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));++cases;}
 events.clear();destroy(nullptr);check(events.empty());check(VirtualFree(code,0,MEM_RELEASE));std::printf("original_keyvalues_cleanup_graph_cases=%u passed\n",cases);
}
