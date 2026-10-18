#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "studio_interface_connector_candidate.h"
#if !defined(_WIN64)
#include "interface_disconnect_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
int main(){unsigned cases=0;
#if !defined(_WIN64)
 unsigned tracked=0,generation=0;std::array<std::array<unsigned,2>,64> records{};std::array<uintptr_t,4> cells{};
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(disconnectOffset);dll.read(reinterpret_cast<char*>(code),sizeof(disconnectBytes));check(bool(dll)&&!std::memcmp(code,disconnectBytes,sizeof(disconnectBytes)));
 for(auto fix:disconnectFixups){uintptr_t address=0;switch(fix[1]){case 0x101909c4:address=reinterpret_cast<uintptr_t>(&generation);break;case 0x101909c8:address=reinterpret_cast<uintptr_t>(&tracked);break;case 0x10190718:address=reinterpret_cast<uintptr_t>(records.data());break;case 0x1019071c:address=reinterpret_cast<uintptr_t>(&records[0][1]);break;default:std::abort();}unsigned value=static_cast<unsigned>(address);std::memcpy(code+fix[0],&value,4);}DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));auto disconnect=reinterpret_cast<void(__cdecl*)()>(code);std::ofstream output("outputs/native-interface-disconnect.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream input("outputs/native-interface-disconnect.bin",std::ios::binary);std::array<uintptr_t,4> cells{};check(reinterpret_cast<uintptr_t>(cells.data())>UINT32_MAX);
#endif
 for(unsigned initialGeneration:{0u,1u,2u,0x80000000u,0x80000001u,0xffffffffu})for(unsigned initialCount:{0u,1u,3u,64u,0xffffffffu}){
 for(unsigned i=0;i<4;++i)cells[i]=100+i;auto expected=cells;unsigned after=initialGeneration-1;
 if(static_cast<int>(after)>=0&&static_cast<int>(initialCount)>0)for(unsigned i=0;i<initialCount;++i)if(i%3==after)expected[i%4]=0;
#if !defined(_WIN64)
 tracked=initialCount;generation=initialGeneration;for(unsigned i=0;i<64;++i)records[i]={static_cast<unsigned>(reinterpret_cast<uintptr_t>(&cells[i%4])),i%3};auto before=records;disconnect();check(records==before&&tracked==initialCount&&generation==after);
#else
 StudioInterfaceConnectorCandidate state;state.tracked=initialCount;state.connection=initialGeneration;for(unsigned i=0;i<64;++i)state.records[i]={&cells[i%4],i%3};studioDisconnectInterfacesCandidate(state);check(state.tracked==initialCount&&state.connection==after);for(unsigned i=0;i<64;++i)check(state.records[i].output==&cells[i%4]&&state.records[i].connection==i%3);
#endif
 check(cells==expected);std::array<unsigned,6> wire{};for(unsigned i=0;i<4;++i)wire[i]=static_cast<unsigned>(cells[i]);wire[4]=initialCount;wire[5]=after;
#if !defined(_WIN64)
 output.write(reinterpret_cast<char*>(wire.data()),sizeof(wire));check(bool(output));
#else
 std::array<unsigned,6> original{};input.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(bool(input)&&wire==original);
#endif
 ++cases;
 }
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif
 std::printf("interface_disconnect_%zu_bit_cases=%u passed\n",sizeof(void*)*8,cases);
}
