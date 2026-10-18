#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstdio>
#include "studio_cpu_topology_count_candidate.h"
int main(){
 const unsigned char bytes[]{0x8b,0x51,0x08,0x33,0xc0,0x85,0xd2,0x74,0x13,0x8b,0x49,0x04,0x83,0xc1,0x04,0x90,0x83,0x39,0x00,0x75,0x01,0x40,0x83,0xc1,0x18,0x4a,0x75,0xf4,0xc3};
#if defined(_M_IX86)
 std::ifstream dll(R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);
 unsigned char header[4096]{};dll.read(reinterpret_cast<char*>(header),sizeof(header));uint32_t pe;std::memcpy(&pe,header+60,4);uint16_t sections,optSize;std::memcpy(&sections,header+pe+6,2);std::memcpy(&optSize,header+pe+20,2);uint32_t offset=0;
 for(unsigned i=0;i<sections;++i){auto* section=header+pe+24+optSize+40*i;uint32_t rva,size,raw;std::memcpy(&rva,section+12,4);std::memcpy(&size,section+16,4);std::memcpy(&raw,section+20,4);if(0x3010>=rva&&0x3010<rva+size)offset=raw+0x3010-rva;}
 if(!offset)return 1;unsigned char installed[sizeof(bytes)]{};dll.seekg(offset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));if(!dll||std::memcmp(bytes,installed,sizeof(bytes)))return 2;
 auto* code=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 3;std::memcpy(code,bytes,sizeof(bytes));DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 4;FlushInstructionCache(GetCurrentProcess(),code,sizeof(bytes));
 auto native=reinterpret_cast<uint32_t(__thiscall*)(void*)>(code);
 std::ofstream oracle("outputs/native-cpu-topology-count.bin",std::ios::binary|std::ios::trunc);if(!oracle)return 5;
#else
 std::ifstream oracle("outputs/native-cpu-topology-count.bin",std::ios::binary);if(!oracle)return 5;
#endif
 unsigned cases=0;
 for(uint32_t count:{0u,1u,2u,31u,32u,255u,256u,257u,1024u})for(unsigned pattern=0;pattern<5;++pattern){
  std::vector<unsigned char> records(static_cast<size_t>(count)*24,0xa5);
  for(uint32_t i=0;i<count;++i){uint32_t relationship=pattern==0?0:pattern==1?1:pattern==2?(i%2):pattern==3?0xffffffffu:(i%7);std::memcpy(records.data()+static_cast<size_t>(i)*24+4,&relationship,4);}
  uint32_t result;
#if defined(_M_IX86)
  std::array<uint32_t,3> object{0,reinterpret_cast<uint32_t>(records.data()),count};result=native(object.data());oracle.write(reinterpret_cast<const char*>(&result),4);
#else
  oracle.read(reinterpret_cast<char*>(&result),4);if(!oracle||result!=studioCpuTopologyCountCandidate(records.data(),count))return 6;
#endif
  ++cases;
 }
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);if(!oracle)return 7;std::printf("native_cpu_topology_count_cases=%u\n",cases);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;std::printf("x64_native_cpu_topology_count_matches=%u\n",cases);
#endif
}
