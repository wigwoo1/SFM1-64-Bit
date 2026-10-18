#include "studio_color_lookup_candidate.h"
#include "color_lookup_native_bytes.h"
#include <Windows.h>
#include <fstream>
#include <cstdio>
int main(){
 static_assert(sizeof(void*)==4,"x86 oracle");
 unsigned char raw[sizeof(colorLookupBytes)]{};
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 dll.seekg(colorLookupFileOffset);dll.read(reinterpret_cast<char*>(raw),sizeof(raw));
 if(!dll||std::memcmp(raw,colorLookupBytes,sizeof(raw)))return 1;
 std::array<float,256> table;for(unsigned i=0;i<256;++i)table[i]=float(i)+10.f;
 float threshold=0.95f,scale=255.f;
 auto patch=[&](unsigned offset,const void* ptr){uint32_t address=reinterpret_cast<uintptr_t>(ptr);std::memcpy(raw+offset,&address,4);};
 patch(0x1a,&threshold);patch(0x2a,&scale);patch(0x3f,table.data());
 void* memory=VirtualAlloc(nullptr,sizeof(raw),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!memory)return 2;
 std::memcpy(memory,raw,sizeof(raw));DWORD previous=0;
 if(!VirtualProtect(memory,sizeof(raw),PAGE_EXECUTE_READ,&previous))return 3;
 FlushInstructionCache(GetCurrentProcess(),memory,sizeof(raw));
 auto native=reinterpret_cast<float(__cdecl*)(float)>(memory);
 unsigned short original=0,control=0;__asm fnstcw original
 unsigned cases=0;uint64_t digest=1469598103934665603ull;
 const StudioColorLookupRoundingCandidate modes[]{StudioColorLookupRoundingCandidate::NearestEven,StudioColorLookupRoundingCandidate::Down,StudioColorLookupRoundingCandidate::Up,StudioColorLookupRoundingCandidate::TowardZero};
 for(unsigned m=0;m<4;++m)for(int i=-512;i<4608;++i){
  const float input=float(i)/4096.f;
  control=static_cast<unsigned short>((original&~0xc00u)|(m<<10));
  __asm fldcw control
  const float expected=native(input);
  __asm fldcw original
  const float got=studioColorLookupCandidate(input,table,modes[m]);
  if(got!=expected){std::printf("mismatch mode=%u input=%g got=%g native=%g\n",m,input,got,expected);return 4;}
  uint32_t bits=0;std::memcpy(&bits,&expected,4);digest=(digest^bits)*1099511628211ull;
  ++cases;
 }
 std::ifstream generated("outputs/native-color-table-candidate.bin",std::ios::binary);generated.read(reinterpret_cast<char*>(table.data()),1024);if(!generated||generated.peek()!=EOF)return 5;
 uint64_t actualDigest=1469598103934665603ull;unsigned actualCases=0;
 for(unsigned m=0;m<4;++m)for(int i=-512;i<4608;++i){
  const float input=float(i)/4096.f;control=static_cast<unsigned short>((original&~0xc00u)|(m<<10));
  __asm fldcw control
  const float expected=native(input);
  __asm fldcw original
  const float got=studioColorLookupCandidate(input,table,modes[m]);if(got!=expected)return 6;
  uint32_t bits;std::memcpy(&bits,&expected,4);actualDigest=(actualDigest^bits)*1099511628211ull;++actualCases;
 }
 VirtualFree(memory,0,MEM_RELEASE);std::printf("native_color_lookup_cases=%u digest=%llu generated_table_cases=%u generated_table_digest=%llu\n",cases,static_cast<unsigned long long>(digest),actualCases,static_cast<unsigned long long>(actualDigest));return 0;
}
