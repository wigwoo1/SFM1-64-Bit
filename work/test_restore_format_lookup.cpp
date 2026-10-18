#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "restore_format_lookup_init.h"
static void check(bool b){if(!b)throw std::runtime_error("format lookup mismatch");}
int main(){try{
 std::array<std::uint32_t,64> table{};
 for(unsigned i=0;i<table.size();++i)table[i]=0xa5000000u^(i*0x1020304u);
#ifdef _M_IX86
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);
 check(bool(installed));installed.seekg(restoreFormatOffset);
 unsigned char raw[15]{};installed.read(reinterpret_cast<char*>(raw),15);check(installed.gcount()==15);
 const unsigned char expected[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0x8b,0x04,0x85,0xd4,0xe5,0x16,0x10,0x5d,0xc3};
 check(std::memcmp(raw,expected,15)==0);
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(memory!=nullptr);
 std::memcpy(memory,raw,15);auto address=reinterpret_cast<std::uintptr_t>(table.data());std::memcpy(memory+9,&address,4);
 DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,15)!=0);
 auto lookup=reinterpret_cast<std::uint32_t(__cdecl*)(unsigned)>(memory);
 std::ofstream out("outputs/native-restore-format-lookup.bin",std::ios::binary);check(bool(out));
 for(unsigned i=0;i<table.size();++i){auto value=lookup(i);check(value==table[i]);out.write(reinterpret_cast<const char*>(&value),4);}
 installed.seekg(restoreFormatTableOffset);installed.read(reinterpret_cast<char*>(table.data()),sizeof(table));check(installed.gcount()==sizeof(table));check(table==restoreFormatInstalledWindow);
 for(unsigned i=0;i<table.size();++i){auto value=lookup(i);check(value==restoreFormatInstalledWindow[i]);out.write(reinterpret_cast<const char*>(&value),4);}
 check(bool(out));check(VirtualFree(memory,0,MEM_RELEASE)!=0);
 std::cout<<"original_format_lookup_cases=128 synthetic_and_installed_window passed\n";
#else
 std::ifstream in("outputs/native-restore-format-lookup.bin",std::ios::binary);check(bool(in));
 for(unsigned i=0;i<table.size();++i){std::uint32_t value=0;in.read(reinterpret_cast<char*>(&value),4);check(in.gcount()==4);check(value==table.at(i));}
 table=restoreFormatInstalledWindow;
 for(unsigned i=0;i<table.size();++i){std::uint32_t value=0;in.read(reinterpret_cast<char*>(&value),4);check(in.gcount()==4);check(value==table.at(i));}
 check(in.peek()==std::char_traits<char>::eof());std::cout<<"x64_format_lookup_cases=128 matched_original\n";
#endif
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
