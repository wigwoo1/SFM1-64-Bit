#include <windows.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>
#include "expression_modulo.h"
#include "original_modulo.inc"
#include "original_modulo_dispatch.inc"
__declspec(naked) void __cdecl dispatch(float,float,float*){__asm{
 push ebp
 mov ebp,esp
 sub esp,2d0h
 push ebx
 fnstcw word ptr [ebp-0a4h]
 fld dword ptr [ebp+8]
 fld dword ptr [ebp+0ch]
 mov edx,10dfe180h
 mov eax,10a1ebb7h
 call eax
 fldcw word ptr [ebp-0a4h]
 mov eax,[ebp+10h]
 fstp dword ptr [eax]
 pop ebx
 leave
 ret
}}
int main(){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);std::cerr<<"Allocate\n";auto memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(memory!=reinterpret_cast<void*>(0x10000000))return 1;
 std::cerr<<"Copy\n";std::memcpy(memory+0xa1ebb7,moduloDispatch,sizeof moduloDispatch);std::memcpy(memory+0xa0f924,originalModulo,sizeof originalModulo);std::memcpy(memory+0xdfe180,moduloDescriptor,sizeof moduloDescriptor);std::memcpy(memory+0xe0b320,moduloClassification,sizeof moduloClassification);
 DWORD previous;if(!VirtualProtect(memory,0x1000000,PAGE_EXECUTE_READ,&previous))return 2;FlushInstructionCache(GetCurrentProcess(),memory,0x1000000);
 const uint32_t inputs[]={0x40e00000,0xc0e00000,0x40400000,0xc0400000,0,0x80000000,0x7f800000,0xff800000,0x7fc00001,0xffc00002,0x7f800003,0xffc00001,0x7fc00002};
 std::ofstream report("outputs/sfm-modulo-dispatch-grid.txt");unsigned count=0;
 for(auto a:inputs)for(auto b:inputs){float left,right,result;std::memcpy(&left,&a,4);std::memcpy(&right,&b,4);dispatch(left,right,&result);uint32_t bits;std::memcpy(&bits,&result,4);report<<std::hex<<a<<" "<<b<<" "<<bits<<"\n";++count;}
 report.close();if(!report)return 3;
 std::ofstream fixtures("work/original_modulo_dispatch_fixtures.bin",std::ios::binary);if(!fixtures)return 4;std::mt19937 random(180924);
 for(unsigned i=0;i<100000;++i){uint32_t pair[2]={random(),random()};
  // Regularly force NaN payload pairs and zero/infinity classifications.
  if(i%4==0){pair[0]=(pair[0]&0x807fffffu)|0x7f800000u;pair[1]=(pair[1]&0x807fffffu)|0x7f800000u;}
  if(i%16==1)pair[1]&=0x80000000u;
  if(i%16==2)pair[1]=(pair[1]&0x80000000u)|0x7f800000u;
  float left,right,result;std::memcpy(&left,pair,4);std::memcpy(&right,pair+1,4);dispatch(left,right,&result);
  uint32_t expected,actual;std::memcpy(&expected,&result,4);auto reconstructed=expressionModulo(left,right);std::memcpy(&actual,&*reconstructed,4);
  if(expected!=actual){std::cerr<<"Mismatch "<<std::hex<<pair[0]<<" "<<pair[1]<<" expected "<<expected<<" actual "<<actual<<"\n";return 5;}
  fixtures.write(reinterpret_cast<const char*>(pair),8);fixtures.write(reinterpret_cast<const char*>(&expected),4);
 }
 fixtures.close();VirtualFree(memory,0,MEM_RELEASE);if(!fixtures)return 6;
 std::cout<<count<<" native modulo classification outputs captured\n100000 randomized native dispatch comparisons passed\n";
}
