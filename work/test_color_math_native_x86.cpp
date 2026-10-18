#define NOMINMAX
#include <Windows.h>
#include "color_math_native_fixups.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <cstring>
#include <cmath>
#include <cstdio>
__declspec(noinline) double invokeDouble(void* entry,double a,double b){double result;
 __asm {push ebx
 push esi
 push edi
 movsd xmm0,a
 movsd xmm1,b
 call entry
 movsd result,xmm0
 pop edi
 pop esi
 pop ebx}
 return result;
}
__declspec(noinline) float invokeFloat(void* entry,float a,float b){float result;
 __asm {push ebx
 push esi
 push edi
 movss xmm0,a
 movss xmm1,b
 call entry
 movss result,xmm0
 pop edi
 pop esi
 pop ebx}
 return result;
}
int main(){
 std::ifstream input("work/color-math-native-image.bin",std::ios::binary);
 std::vector<unsigned char> image((std::istreambuf_iterator<char>(input)),{});if(image.size()!=0xb000)return 1;
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 IMAGE_DOS_HEADER dos{};dll.read(reinterpret_cast<char*>(&dos),sizeof(dos));dll.seekg(dos.e_lfanew);
 IMAGE_NT_HEADERS32 nt{};dll.read(reinterpret_cast<char*>(&nt),sizeof(nt));if(nt.Signature!=IMAGE_NT_SIGNATURE)return 2;
 std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);dll.read(reinterpret_cast<char*>(sections.data()),sections.size()*sizeof(IMAGE_SECTION_HEADER));
 auto verify=[&](uint32_t rva,unsigned destination,unsigned size){
  for(auto s:sections)if(rva>=s.VirtualAddress&&rva+size<=s.VirtualAddress+s.SizeOfRawData){
   std::vector<char> bytes(size);dll.seekg(s.PointerToRawData+rva-s.VirtualAddress);dll.read(bytes.data(),size);
   return bool(dll)&&!std::memcmp(bytes.data(),image.data()+destination,size);
  }return false;
 };
 if(!verify(0xe0000,0,0x2000)||!verify(0x153000,0x4000,0x7000))return 3;
 auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,image.size(),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!memory)return 4;
 std::memcpy(memory,image.data(),image.size());
 for(auto& f:colorMathFixups){uint32_t target=reinterpret_cast<uintptr_t>(memory+f[1]);std::memcpy(memory+f[0],&target,4);}
 DWORD old=0;if(!VirtualProtect(memory,0x2000,PAGE_EXECUTE_READ,&old)||!VirtualProtect(memory+0x2000,0x2000,PAGE_NOACCESS,&old)||!VirtualProtect(memory+0x4000,0x7000,PAGE_READONLY,&old))return 5;
 FlushInstructionCache(GetCurrentProcess(),memory,0x2000);
 void* doubleEntry=memory+0x1aa;void* floatEntry=memory+0x1154;
 if(invokeDouble(doubleEntry,0.5,2)!=0.25||invokeDouble(doubleEntry,2,3)!=8||invokeFloat(floatEntry,0.5f,2)!=0.25f)return 6;
 std::vector<float> table(256);float previous=-1;uint64_t digest=1469598103934665603ull;
 for(unsigned i=0;i<256;++i){
  const float normalized=float(i)*(1.f/255.f);
  const double value=invokeDouble(doubleEntry,double(normalized),double(2.2f));
  if(!std::isfinite(value)||std::abs(value-std::pow(double(normalized),double(2.2f)))>1e-12)return 7;
  table[i]=float(value);if(table[i]<previous)return 8;previous=table[i];
  uint32_t bits;std::memcpy(&bits,&table[i],4);digest=(digest^bits)*1099511628211ull;
 }
 std::ofstream output("outputs/native-color-table-candidate.bin",std::ios::binary);output.write(reinterpret_cast<const char*>(table.data()),1024);if(!output)return 9;
 VirtualFree(memory,0,MEM_RELEASE);
 std::printf("native_math_smoke_cases=3 generated_table_entries=256 digest=%llu power_semantics_tolerance=1e-12 full_generator_parity=unverified\n",static_cast<unsigned long long>(digest));return 0;
}
