#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "restore_publish_init.h"
static void check(bool b){if(!b)throw std::runtime_error("publish mismatch");}
static unsigned* activeRing;static unsigned waits;
static void __stdcall waitCallback(unsigned value){check(value==0);++waits;check(waits==1);activeRing[0x9d8/4]=activeRing[0x8d0/4]-499u;}
int main(){try{
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(installed));unsigned char raw[105]{};installed.seekg(publishOffset);installed.read(reinterpret_cast<char*>(raw),105);check(installed.gcount()==105&&std::memcmp(raw,publishBytes,105)==0);
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(memory!=nullptr);std::memcpy(memory,raw,105);auto waitFunction=&waitCallback;auto cell=reinterpret_cast<unsigned>(&waitFunction);std::memcpy(memory+30,&cell,4);DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,105)!=0);auto publish=reinterpret_cast<void(__thiscall*)(void*,unsigned)>(memory);unsigned cases=0;
 std::ofstream records("outputs/native-restore-publication.bin",std::ios::binary);check(bool(records));
 for(unsigned producer:{0u,499u,500u,0xffffffffu})for(unsigned index:{0u,1u,499u})for(unsigned full=0;full<2;++full){
  std::array<unsigned,640> ring{};ring.fill(0xabababab);ring[0x8d0/4]=producer;ring[0x8d4/4]=index;ring[0x9d8/4]=producer-(full?500u:0u);auto expected=ring;expected[index]=0x12345678;expected[0x8d0/4]=producer+1;expected[0x8d4/4]=(index+1)%500;if(full)expected[0x9d8/4]=producer-499u;activeRing=ring.data();waits=0;publish(ring.data(),0x12345678);check(ring==expected&&waits==full);++cases;
  records.write(reinterpret_cast<const char*>(ring.data()),sizeof(ring));records.write(reinterpret_cast<const char*>(&waits),4);check(bool(records));
 }
 check(VirtualFree(memory,0,MEM_RELEASE)!=0);std::cout<<"original_publish_cases="<<cases<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
