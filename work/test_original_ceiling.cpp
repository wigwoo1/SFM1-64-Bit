#include <windows.h>
#include <random>
#include <fstream>
#include <iostream>
#include <cmath>
#include "expression_ceiling.h"
#include "original_ceiling.inc"
int main(){
 auto memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(memory!=reinterpret_cast<void*>(0x10000000))return 1;
 std::memcpy(memory+0xa0f010,originalCeiling,sizeof originalCeiling);std::memcpy(memory+0xdfe100,ceilingConstants,sizeof ceilingConstants);
 DWORD previous;if(!VirtualProtect(memory,0x1000000,PAGE_EXECUTE_READ,&previous))return 2;FlushInstructionCache(GetCurrentProcess(),memory,0x1000000);
 using Native=double(__cdecl*)(double);auto native=reinterpret_cast<Native>(memory+0xa0f010);
 std::ofstream fixtures("work/original_ceiling_fixtures.bin",std::ios::binary);if(!fixtures)return 3;
 std::mt19937 random(10010);unsigned count=0;
 const uint32_t focused[]={0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x3f800001,0xbf800001,0x3f7fffff,0xbf7fffff,0x4affffff,0xcaffffff};
 for(unsigned index=0;index<100012;++index){uint32_t bits=index<12?focused[index]:random();float input;std::memcpy(&input,&bits,4);if(std::isnan(input))continue;
  float expected=float(native(double(input)));uint32_t expectedBits,actualBits;std::memcpy(&expectedBits,&expected,4);auto actual=expressionCeiling(input);if(!actual)return 4;std::memcpy(&actualBits,&*actual,4);
  if(expectedBits!=actualBits){std::cerr<<"Mismatch "<<std::hex<<bits<<"\n";return 5;}
  fixtures.write(reinterpret_cast<const char*>(&bits),4);fixtures.write(reinterpret_cast<const char*>(&expectedBits),4);++count;
 }
 fixtures.close();VirtualFree(memory,0,MEM_RELEASE);if(!fixtures)return 6;std::cout<<count<<" native optimized ceiling cases passed\n";
}
