#include "studio_specialized_draw_factor_candidate.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
int main(){
 unsigned cases=0;
#if defined(_M_IX86)
 const unsigned char expected[]={0x8b,0xff,0x55,0x8b,0xec,0x51,0x51,0xdd,0x45,0x08,0xd9,0xfc,0xdd,0x5d,0xf8,0xdd,0x45,0xf8,0xc9,0xc3};
 unsigned char source[sizeof(expected)]={};
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 dll.seekg(0xf5bb0);dll.read(reinterpret_cast<char*>(source),sizeof(source));
 if(!dll||std::memcmp(source,expected,sizeof(source)))return 2;
 void* code=VirtualAlloc(nullptr,sizeof(source),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!code)return 3;
 std::memcpy(code,source,sizeof(source));DWORD prior=0;
 if(!VirtualProtect(code,sizeof(source),PAGE_EXECUTE_READ,&prior))return 4;
 FlushInstructionCache(GetCurrentProcess(),code,sizeof(source));
 using Round=double(__cdecl*)(double);
 const auto native=reinterpret_cast<Round>(code);
 unsigned short original=0,upward=0;
 __asm fnstcw original
 upward=static_cast<unsigned short>((original&~0xc00u)|0x800u);
#endif
 for(int i=-32768;i<32768;++i){
  const float value=static_cast<float>(i)/1024.0f;
  const uint32_t got=studioSpecializedDrawFiniteFactorCandidate(value);
  const int bounded=i<1024?1024:(i>16384?16384:i);
  const uint32_t gridExpected=static_cast<uint32_t>((bounded+1023)/1024);
  if(got!=gridExpected)return 5;
#if defined(_M_IX86)
  const double arg=static_cast<double>(value<1?1:(value>16?16:value));
  __asm fldcw upward
  const double rounded=native(arg);
  __asm fldcw original
  if(rounded!=static_cast<double>(got))return 6;
#endif
  ++cases;
 }
 for(unsigned i=1;i<=16;++i){
  const float value=static_cast<float>(i);
  if(studioSpecializedDrawFiniteFactorCandidate(value)!=i)return 7;
  if(studioSpecializedDrawFiniteFactorCandidate(std::nextafter(value,0.0f))!=i)return 8;
  if(studioSpecializedDrawFiniteFactorCandidate(std::nextafter(value,17.0f))!=(i==16?16:i+1))return 9;
 }
 unsigned rejected=0;
 for(float value:{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  try{studioSpecializedDrawFiniteFactorCandidate(value);}catch(const std::invalid_argument&){++rejected;}
 }
 if(rejected!=3)return 10;
#if defined(_M_IX86)
 VirtualFree(code,0,MEM_RELEASE);
 std::printf("native_frndint_cases=%u ",cases);
#else
 std::printf("x64_grid_cases=%u ",cases);
#endif
 std::printf("integer_boundary_cases=48 nonfinite_policy_rejections=3\n");
 return 0;
}
