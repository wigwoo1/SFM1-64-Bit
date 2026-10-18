#include <windows.h>
#include <fstream>
#include <iostream>
#include <random>
#include <cstring>
#include "decimal_scaler.h"
#include "original_scaler_kernel.inc"
// Set only the parser locals used by the isolated scaling stage.
// Its original epilogue restores our saved registers and returns to C++.
__declspec(naked) unsigned __cdecl originalScale(uint16_t*,const uint16_t*,int,unsigned){
 __asm {
  push ebp
  mov ebp,esp
  sub esp,7ch
  push esi
  push edi
  push ebx
  mov eax,[ebp+8]
  mov [ebp-78h],eax
  xor eax,eax
  mov [ebp-74h],eax
  mov [ebp-6ch],eax
  mov [ebp-4],eax
  mov esi,[ebp+0ch]
  lea edi,[ebp-3ch]
  movsd
  movsd
  movsd
  mov eax,[ebp+10h]
  xor edx,edx
  mov ecx,10a1e21fh
  jmp ecx
 }
}
int main(){
 auto memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(memory!=reinterpret_cast<void*>(0x10000000))return 1;
 std::memcpy(memory+0xa1e21f,originalScaler,sizeof originalScaler);
 std::memcpy(memory+0xf22800,originalScaleTables,sizeof originalScaleTables);
 memory[0xa0e353]=0xc3; // Isolated compiler cookie-check stub.
 DWORD previous;if(!VirtualProtect(memory,0x1000000,PAGE_EXECUTE_READ,&previous))return 2;
 FlushInstructionCache(GetCurrentProcess(),memory,0x1000000);
 std::ofstream fixtures("work/original_scaler_fixtures.bin",std::ios::binary);if(!fixtures)return 3;
 std::mt19937 random(51288);unsigned count=0;
 for(unsigned index=0;index<30401;++index){
  NormalizedExtended value{{0,0,0,0,0x8000},0x3fff};
  int exponent=int(index)-5200;
  if(index>=10401){for(auto&word:value.significand)word=uint16_t(random());value.significand[4]|=0x8000;value.exponent=uint16_t(16000+random()%1000);exponent=int(random()%10401)-5200;}
  uint16_t input[6];std::memcpy(input,value.significand.data(),10);input[5]=value.exponent;
  uint16_t expected[6]{};originalScale(expected,input,exponent,0);
  auto actual=scaleDecimalExtended(value,exponent);
  if(std::memcmp(expected,actual.significand.data(),10)||expected[5]!=actual.exponent){std::cerr<<"Mismatch "<<index<<" decimal exponent "<<exponent<<"\n";return 4;}
  fixtures.write(reinterpret_cast<const char*>(input),12);fixtures.write(reinterpret_cast<const char*>(&exponent),4);fixtures.write(reinterpret_cast<const char*>(expected),12);++count;
 }
 fixtures.close();VirtualFree(memory,0,MEM_RELEASE);if(!fixtures)return 5;
 std::cout<<count<<" original inline scaler differential cases passed\n";
}
