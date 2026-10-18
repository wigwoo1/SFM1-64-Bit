#define NOMINMAX
#include <Windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <cstdio>
#include "constant_copy_helper_native_init.h"
int main(){
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 std::array<unsigned char,sizeof(copyHelperBytes)> bytes{};dll.seekg(copyHelperFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),copyHelperBytes,bytes.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());static uint32_t feature=0;
 for(auto& fixup:copyHelperFixups){const uint32_t target=fixup[1]==0x101b610c?static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&feature)):static_cast<uint32_t>(reinterpret_cast<uintptr_t>(code)+fixup[1]-0x100df030);std::memcpy(code+fixup[0],&target,4);}
 // Disabled optimized branch must never leave the isolated helper.
 code[0x3f]=0x0f;code[0x40]=0x0b;code[0x41]=code[0x42]=code[0x43]=0xcc;
 DWORD previous;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&previous))return 3;FlushInstructionCache(GetCurrentProcess(),code,4096);
 auto copy=reinterpret_cast<void*(__cdecl*)(void*,const void*,uint32_t)>(code);
 std::ofstream oracle("outputs/native-constant-copy-helper.bin",std::ios::binary|std::ios::trunc);
#else
 std::ifstream oracle("outputs/native-constant-copy-helper.bin",std::ios::binary);
#endif
 unsigned cases=0;
 for(unsigned count=0;count<=192;++count)for(unsigned alignment=0;alignment<8;++alignment)for(int shift=-8;shift<=9;++shift){
  std::array<unsigned char,512> buffer{};for(unsigned i=0;i<buffer.size();++i)buffer[i]=static_cast<unsigned char>((i*37+count*11)&255);
  auto* source=buffer.data()+64+alignment;auto* destination=shift==9?buffer.data()+288+alignment:source+shift;
#ifdef _M_IX86
  if(copy(destination,source,count)!=destination)return 4;oracle.write(reinterpret_cast<const char*>(buffer.data()),buffer.size());
#else
  if(std::memmove(destination,source,count)!=destination)return 4;std::array<unsigned char,512> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),expected.size());if(buffer!=expected)return 5;
#endif
  if(!oracle)return 6;++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 7;
#endif
 std::printf("constant_copy_helper_cases=%u passed\n",cases);
}
