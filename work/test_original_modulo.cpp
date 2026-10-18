#include <windows.h>
#include <random>
#include <fstream>
#include <iostream>
#include <cstring>
#include "expression_modulo.h"
#include "original_modulo.inc"
void* moduloBody=nullptr;
__declspec(naked) void __cdecl nativeModulo(float,float,float*){__asm{
 push ebp
 mov ebp,esp
 fld dword ptr [ebp+8]
 fld dword ptr [ebp+0ch]
 call moduloBody
 mov eax,[ebp+10h]
 fstp dword ptr [eax]
 pop ebp
 ret
}}
int main(){moduloBody=VirtualAlloc(nullptr,sizeof originalModulo,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!moduloBody)return 1;
 std::memcpy(moduloBody,originalModulo,sizeof originalModulo);DWORD previous;if(!VirtualProtect(moduloBody,sizeof originalModulo,PAGE_EXECUTE_READ,&previous))return 2;FlushInstructionCache(GetCurrentProcess(),moduloBody,sizeof originalModulo);
 std::ofstream fixtures("work/original_modulo_fixtures.bin",std::ios::binary);if(!fixtures)return 3;std::mt19937 random(924);unsigned count=0;
 for(unsigned index=0;index<100000;++index){uint32_t inputs[2]={random(),random()};float left,right;std::memcpy(&left,inputs,4);std::memcpy(&right,inputs+1,4);if(!std::isfinite(left)||!std::isfinite(right)||right==0)continue;auto actual=expressionModulo(left,right);
  float expected;nativeModulo(left,right,&expected);uint32_t expectedBits,actualBits;std::memcpy(&expectedBits,&expected,4);std::memcpy(&actualBits,&*actual,4);if(expectedBits!=actualBits)return 4;
  fixtures.write(reinterpret_cast<const char*>(inputs),8);fixtures.write(reinterpret_cast<const char*>(&expectedBits),4);++count;
 }
 fixtures.close();VirtualFree(moduloBody,0,MEM_RELEASE);if(!fixtures)return 5;std::cout<<count<<" native FPREM modulo cases passed\n";
}
