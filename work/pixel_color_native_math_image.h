#pragma once
#include "color_math_native_fixups.h"
#include <vector>
#include <iterator>
// Reuse the verified original math image, checking both file-backed chunks
// against the installed PE before fixing references and protecting pages.
inline unsigned char* pixelColorNativeMathImage(){
 std::ifstream input("work/color-math-native-image.bin",std::ios::binary);
 std::vector<unsigned char> image((std::istreambuf_iterator<char>(input)),{});
 if(image.size()!=0xb000)return nullptr;
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 IMAGE_DOS_HEADER dos{};dll.read(reinterpret_cast<char*>(&dos),sizeof(dos));dll.seekg(dos.e_lfanew);
 IMAGE_NT_HEADERS32 nt{};dll.read(reinterpret_cast<char*>(&nt),sizeof(nt));if(!dll||nt.Signature!=IMAGE_NT_SIGNATURE)return nullptr;
 std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);
 dll.read(reinterpret_cast<char*>(sections.data()),sections.size()*sizeof(IMAGE_SECTION_HEADER));
 auto verify=[&](uint32_t rva,unsigned destination,unsigned size){
  for(auto s:sections)if(rva>=s.VirtualAddress&&rva+size<=s.VirtualAddress+s.SizeOfRawData){
   std::vector<char> bytes(size);dll.seekg(s.PointerToRawData+rva-s.VirtualAddress);dll.read(bytes.data(),size);
   return bool(dll)&&!std::memcmp(bytes.data(),image.data()+destination,size);
  }return false;
 };
 if(!verify(0xe0000,0,0x2000)||!verify(0x153000,0x4000,0x7000))return nullptr;
 auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,image.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!memory)return nullptr;
 std::memcpy(memory,image.data(),image.size());
 for(auto& f:colorMathFixups){uint32_t target=reinterpret_cast<uintptr_t>(memory+f[1]);std::memcpy(memory+f[0],&target,4);}
 DWORD old;if(!VirtualProtect(memory,0x2000,PAGE_EXECUTE_READ,&old)||!VirtualProtect(memory+0x2000,0x2000,PAGE_NOACCESS,&old)||!VirtualProtect(memory+0x4000,0x7000,PAGE_READONLY,&old)){VirtualFree(memory,0,MEM_RELEASE);return nullptr;}
 FlushInstructionCache(GetCurrentProcess(),memory,0x2000);return memory;
}
