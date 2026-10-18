#include <windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "restore_owner_gate_init.h"
#include "studio_restore_owner_gate_candidate.h"
static void check(bool b){if(!b)throw std::runtime_error("owner gate mismatch");}
int main(){try{
#ifdef _M_IX86
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(installed));installed.seekg(ownerGateOffset);unsigned char bytes[36]{};installed.read(reinterpret_cast<char*>(bytes),36);check(installed.gcount()==36&&std::memcmp(bytes,ownerGateBytes,36)==0);
 auto memory=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);check(memory!=nullptr);std::memcpy(memory,bytes,36);DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,36)!=0);auto original=reinterpret_cast<unsigned(__thiscall*)(void*)>(memory);
 std::ofstream records("outputs/native-restore-owner-gate.bin",std::ios::binary);check(bool(records));
#else
 std::ifstream records("outputs/native-restore-owner-gate.bin",std::ios::binary);check(bool(records));
#endif
 unsigned cases=0;
 for(unsigned flags=0;flags<256;++flags)for(unsigned field:{0u,1u,256u,0xffffffffu})for(unsigned depth:{0u,1u,256u,0xffffffffu}){
  auto candidate=studioRestoreOwnerGateCandidate(field,static_cast<std::uint8_t>(flags),depth);
#ifdef _M_IX86
  std::array<unsigned,192> owner{};owner.fill(0xabababab);owner[0xa8/4]=field;reinterpret_cast<unsigned char*>(owner.data())[0xac]=static_cast<unsigned char>(flags);owner[0x2cc/4]=depth;auto before=owner;auto result=original(owner.data());check(result==candidate&&owner==before);records.write(reinterpret_cast<const char*>(&result),4);check(bool(records));
#else
  unsigned result=0;records.read(reinterpret_cast<char*>(&result),4);check(records.gcount()==4&&result==candidate);
#endif
  ++cases;
 }
#ifdef _M_IX86
 check(VirtualFree(memory,0,MEM_RELEASE)!=0);
#else
 check(records.peek()==std::char_traits<char>::eof());
#endif
 std::cout<<"owner_gate_cases="<<cases<<" passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
