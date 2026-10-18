#include <windows.h>
#include <immintrin.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "gamma_lookup_oracle_bytes.h"
#include "native_gamma_index_candidate.h"
int main(){
 unsigned short saved; __asm fnstcw saved
 unsigned mxSaved=_mm_getcsr();
 void* allocation=VirtualAlloc(nullptr,sizeof(originalGammaCode),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!allocation)return 2;
 int result=0;
 try{
  float threshold=.95f,multiplier=255.f;std::array<float,256> table;
  for(unsigned i=0;i<256;++i)table[i]=float(i);
  unsigned pointers[]={reinterpret_cast<unsigned>(&threshold),reinterpret_cast<unsigned>(&multiplier),reinterpret_cast<unsigned>(table.data())};
  std::memcpy(allocation,originalGammaCode,sizeof(originalGammaCode));
  for(unsigned i=0;i<3;++i)std::memcpy(static_cast<unsigned char*>(allocation)+gammaFixups[i],pointers+i,4);
  DWORD previous;
  if(!VirtualProtect(allocation,sizeof(originalGammaCode),PAGE_EXECUTE_READ,&previous)||!FlushInstructionCache(GetCurrentProcess(),allocation,sizeof(originalGammaCode)))throw std::runtime_error("executable fixture setup failed");
  auto native=reinterpret_cast<float(__cdecl*)(float)>(allocation);
  unsigned comparisons=0;
  for(unsigned mode=0;mode<4;++mode){
   unsigned short control=static_cast<unsigned short>((saved&~0xc00u)|(mode<<10));
   __asm fldcw control
   for(unsigned sseMode=0;sseMode<4;++sseMode){
   _mm_setcsr((mxSaved&~0x6000u)|(sseMode<<13));
   for(unsigned i=0;i<=4096;++i){
    float input=float(i)/4096.f;
    float expected=input>=.95f?1.f:table[static_cast<unsigned>(nativeGammaIndexCandidate(input,static_cast<NativeGammaIndexRounding>(mode)))];
    if(native(input)!=expected)throw std::runtime_error("matched-mode lookup mismatch");
    ++comparisons;
   }
   if(native(-.5f)!=0||native(2.f)!=1)throw std::runtime_error("native clamp mismatch");
   }
  }
  unsigned short nearest=static_cast<unsigned short>(saved&~0xc00u);
  __asm fldcw nearest
  _mm_setcsr((mxSaved&~0x6000u)|0x2000u);
  float original=native(.5f);int sse=_mm_cvtss_si32(_mm_set_ss(.5f*255.f));
  if(original!=128.f||sse!=127)throw std::runtime_error("rounding difference not demonstrated");
  std::cout<<"PASS original x86 code: "<<comparisons<<" explicit-policy comparisons, sixteen x87/SSE mode combinations; mismatched midpoint native=128 SSE=127. Synthetic table only.\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';result=1;}
 __asm fldcw saved
 _mm_setcsr(mxSaved);VirtualFree(allocation,0,MEM_RELEASE);return result;
}
