#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cfenv>
#include "studio_gamma_startup_candidate.h"
#if !defined(_WIN64)
#include "gamma_startup_native_init.h"
#include "actual_power_native_init.h"
#endif
static void check(bool b){if(!b)std::abort();}
static std::vector<unsigned char> trace;
template<class T> static void append(const T& x){const auto* p=reinterpret_cast<const unsigned char*>(&x);trace.insert(trace.end(),p,p+sizeof(x));}
static unsigned floatCalls,doubleCalls;
#if !defined(_WIN64)
static float(__vectorcall* actualFloat)(float,float);
static double(__vectorcall* actualDouble)(double,double);
#endif
static float __vectorcall controlledFloat(float x,float y){++floatCalls;append(x);append(y);
#if !defined(_WIN64)
 return actualFloat(x,y);
#else
 return std::pow(x,y);
#endif
}
static double __vectorcall controlledDouble(double x,double y){++doubleCalls;append(x);append(y);
#if !defined(_WIN64)
 return actualDouble(x,y);
#else
 return std::pow(x,y);
#endif
}
struct Ops {
 float powerFloat(float a,float b){return controlledFloat(a,b);}
 double powerDouble(double a,double b){return controlledDouble(a,b);}
 int truncateFloat(float a){return static_cast<int>(a);}
 int truncateDouble(double a){return static_cast<int>(a);}
 int roundFloat(float a){return static_cast<int>(std::nearbyint(a));}
};
int main(){check(std::fegetround()==FE_TONEAREST);StudioGammaTablesCandidate t;
#if !defined(_WIN64)
 auto power=static_cast<unsigned char*>(VirtualAlloc(nullptr,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(power);
 std::ifstream source(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 const auto load=[&](unsigned offset,const unsigned char* expected,unsigned size,unsigned fileOffset){source.seekg(fileOffset);source.read(reinterpret_cast<char*>(power+offset),size);check(bool(source)&&!std::memcmp(power+offset,expected,size));};
 load(0,powerBytes0,sizeof(powerBytes0),powerFileOffset0);load(4096,powerBytes1,sizeof(powerBytes1),powerFileOffset1);load(16384,powerData,sizeof(powerData),powerDataFileOffset);
 for(const auto& f:powerFixups0){unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(power+16384+f[1]-0x10153a00));std::memcpy(power+f[0],&value,4);}
 for(const auto& f:powerFixups1){unsigned value=static_cast<unsigned>(reinterpret_cast<uintptr_t>(power+16384+f[1]-0x10153a00));std::memcpy(power+4096+f[0],&value,4);}
 DWORD previous;check(VirtualProtect(power,65536,PAGE_EXECUTE_READ,&previous)&&FlushInstructionCache(GetCurrentProcess(),power,65536));
 actualDouble=reinterpret_cast<double(__vectorcall*)(double,double)>(power);actualFloat=reinterpret_cast<float(__vectorcall*)(float,float)>(power+4096);
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code);
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);dll.seekg(gammaOriginalFileOffset);dll.read(reinterpret_cast<char*>(code),sizeof(gammaOriginalBytes));check(bool(dll)&&!std::memcmp(code,gammaOriginalBytes,sizeof(gammaOriginalBytes)));
 for(const auto& f:gammaOriginalFixups){unsigned value=static_cast<unsigned>(gammaAddress(f[1],t));std::memcpy(code+f[0],&value,4);}
 for(const auto& c:gammaOriginalCalls){uintptr_t target=0;if(c[1]==0x100e1154)target=reinterpret_cast<uintptr_t>(&controlledFloat);else if(c[1]==0x100e01aa)target=reinterpret_cast<uintptr_t>(&controlledDouble);else std::abort();unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(code+c[0]+5));std::memcpy(code+c[0]+1,&delta,4);}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)&&FlushInstructionCache(GetCurrentProcess(),code,4096));
 std::ofstream output("outputs/native-actual-power-gamma-startup.bin",std::ios::binary|std::ios::trunc);check(bool(output));
#else
 check(reinterpret_cast<uintptr_t>(&t)>UINT32_MAX);Ops ops;std::ifstream input("outputs/native-actual-power-gamma-startup.bin",std::ios::binary);check(bool(input));
#endif
 unsigned cases=0;
#if defined(_WIN64)
 unsigned mismatchedCases=0;size_t differingBytes=0;
#endif
 for(float gamma:{1.0f,2.2f,4.0f})for(float tex:{1.0f,2.2f,3.0f})for(float brightness:{-0.5f,0.0f,0.5f,1.5f})for(int overbright:{1,2,4}){
 trace.clear();floatCalls=doubleCalls=0;t={};
#if !defined(_WIN64)
 reinterpret_cast<void(__cdecl*)(float,float,float,int)>(code)(gamma,tex,brightness,overbright);
#else
 studioGammaStartupCandidate(t,gamma,tex,brightness,overbright,ops);
#endif
 check(floatCalls==1536&&doubleCalls==5632);
 append(t.textureGamma);append(t.linearToScreenGamma);append(t.textureToLinear);append(t.fixedPower0454);append(t.fixedPower22);append(t.linearToTexture);append(t.linearToVertex);append(t.linearToLightmap);
#if !defined(_WIN64)
 output.write(reinterpret_cast<char*>(trace.data()),trace.size());check(bool(output));
#else
 std::vector<unsigned char> original(trace.size());input.read(reinterpret_cast<char*>(original.data()),original.size());check(bool(input));
 if(trace!=original){++mismatchedCases;for(size_t i=0;i<trace.size();++i)if(trace[i]!=original[i])++differingBytes;}
 // Compare every argument independently before assessing output parity.
 constexpr size_t argumentBytes=1536*8+5632*16;
 check(!std::memcmp(trace.data(),original.data(),argumentBytes));

#endif
 ++cases;
 }
#if !defined(_WIN64)
 check(VirtualFree(code,0,MEM_RELEASE)&&VirtualFree(power,0,MEM_RELEASE));
#else
 check(input.peek()==std::char_traits<char>::eof());
#endif

#if defined(_WIN64)
 std::printf("actual_power_gamma_%zu_bit_cases=%u argument_match=passed mismatched_table_cases=%u differing_bytes=%zu\n",sizeof(void*)*8,cases,mismatchedCases,differingBytes);
#else
 std::printf("actual_power_gamma_%zu_bit_cases=%u original_tables_recorded=passed\n",sizeof(void*)*8,cases);
#endif
}
