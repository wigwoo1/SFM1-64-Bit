#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "vertex_tracker_connector_native_init.h"
static void check(bool b){if(!b)std::abort();}
static unsigned tracked,connection,offers;static std::array<unsigned,2> calls;
static void* __cdecl factoryA(const char* name,int* status){check(status==nullptr);++calls[0];return !std::strcmp(name,"VBAllocTracker001")&&(offers&1u)?reinterpret_cast<void*>(0x1234):nullptr;}
static void* __cdecl factoryB(const char* name,int* status){check(status==nullptr);++calls[1];return !std::strcmp(name,"VBAllocTracker001")&&(offers&2u)?reinterpret_cast<void*>(0x5678):nullptr;}
static void __cdecl forbiddenDiagnostic(const char*,...){std::abort();}
int main(){static_assert(sizeof(void*)==4,"original x86");std::array<unsigned,56> cells{};std::array<std::array<unsigned,2>,56> table{};for(unsigned i=0;i<56;++i)table[i]={static_cast<unsigned>(reinterpret_cast<uintptr_t>(i==19?"VBAllocTracker001":"unavailable")),static_cast<unsigned>(reinterpret_cast<uintptr_t>(&cells[i]))};std::array<std::array<unsigned,2>,64> records;auto diagnostic=&forbiddenDiagnostic;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,329> raw{};dll.seekg(trackerConnectorFileOffset);dll.read(reinterpret_cast<char*>(raw.data()),raw.size());check(bool(dll)&&!std::memcmp(raw.data(),trackerConnectorBytes,raw.size()));auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);std::memcpy(code,raw.data(),raw.size());
 for(auto fix:trackerConnectorFixups){uintptr_t address=0;switch(fix[1]){case 0x101909c8:address=reinterpret_cast<uintptr_t>(&tracked);break;case 0x101909c4:address=reinterpret_cast<uintptr_t>(&connection);break;case 0x1017b158:address=reinterpret_cast<uintptr_t>(table.data());break;case 0x1017b15c:address=reinterpret_cast<uintptr_t>(&table[0][1]);break;case 0x10190718:address=reinterpret_cast<uintptr_t>(records.data());break;case 0x101523c0:address=reinterpret_cast<uintptr_t>("diagnostic trap");break;case 0x100fa23c:address=reinterpret_cast<uintptr_t>(&diagnostic);break;default:std::abort();}unsigned value=static_cast<unsigned>(address);std::memcpy(code+fix[0],&value,4);}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));using Factory=void*(__cdecl*)(const char*,int*);std::array<Factory,2> factories{&factoryA,&factoryB};auto connect=reinterpret_cast<void(__cdecl*)(Factory*,int)>(code);unsigned cases=0;std::ofstream output("outputs/native-tracker-connector.bin",std::ios::binary|std::ios::trunc);
 for(unsigned initial:{0u,1u})for(bool seeded:{false,true})for(unsigned mask=0;mask<4;++mask)for(unsigned count=0;count<3;++count){tracked=initial;connection=9;offers=mask;calls.fill(0);cells.fill(0);cells[19]=seeded?0xabcd:0;records.fill({0xabababab,0xabababab});if(initial)records[0]={static_cast<unsigned>(reinterpret_cast<uintptr_t>(&cells[19])),8};auto expectedCells=cells;auto expectedRecords=records;unsigned expectedTracked=initial;std::array<unsigned,2> expectedCalls{};
 for(unsigned f=0;f<count;++f)for(unsigned i=0;i<56;++i){if(initial==0&&expectedCells[i]!=0)continue;++expectedCalls[f];unsigned value=i==19&&(mask&(1u<<f))?(f==0?0x1234u:0x5678u):0;expectedCells[i]=value;if(initial==0&&value){expectedRecords[expectedTracked++]={static_cast<unsigned>(reinterpret_cast<uintptr_t>(&cells[i])),9};}}
 connect(factories.data(),static_cast<int>(count));check(cells==expectedCells&&records==expectedRecords&&tracked==expectedTracked&&connection==10&&calls==expectedCalls);std::array<unsigned,188> wire{};std::copy(cells.begin(),cells.end(),wire.begin());for(unsigned i=0;i<64;++i){wire[56+2*i]=records[i][0];wire[57+2*i]=records[i][1];if(i<tracked){check(records[i][0]==static_cast<unsigned>(reinterpret_cast<uintptr_t>(&cells[19])));wire[56+2*i]=20;}}wire[184]=tracked;wire[185]=connection;wire[186]=calls[0];wire[187]=calls[1];output.write(reinterpret_cast<const char*>(wire.data()),sizeof(wire));check(bool(output));++cases;}
 check(VirtualFree(code,0,MEM_RELEASE));std::printf("original_tracker_connector_cases=%u passed\n",cases);
}
