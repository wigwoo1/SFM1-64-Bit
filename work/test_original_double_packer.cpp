#include <windows.h>
#include <cstring>
#include <iostream>
#include <random>
#include <fstream>
#include "extended_double.h"
#include "original_double_packer.inc"
int main(){
 auto memory=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(memory!=reinterpret_cast<void*>(0x10000000)){std::cerr<<"Cannot reserve original address range\n";return 1;}
 std::memcpy(memory+0xa31655,originalCode,sizeof originalCode);
 std::memcpy(memory+0xf22858,originalConfig,sizeof originalConfig);
 // The routine's compiler security-cookie check is replaced by RET in this
 // isolated harness. Conversion instructions and constants are unchanged.
 memory[0xa0e353]=0xc3;
 DWORD oldProtection;
 if(!VirtualProtect(memory,0x1000000,PAGE_EXECUTE_READ,&oldProtection))return 2;
 FlushInstructionCache(GetCurrentProcess(),memory,0x1000000);
 using Original=unsigned(__cdecl*)(const uint16_t*,uint64_t*);
 auto original=reinterpret_cast<Original>(memory+0xa31655);
 std::ofstream fixtures("work/original_double_fixtures.bin",std::ios::binary);
 if(!fixtures)return 4;
 std::mt19937 random(73119);unsigned count=0;
 for(unsigned index=0;index<50000;++index){
  NormalizedExtended value{};
  for(auto&word:value.significand)word=uint16_t(random());
  value.significand[4]|=0x8000;
  // Dense subnormal boundaries plus full extended exponent domain.
  unsigned exponent=index<20000?unsigned(0x3fff-1100+random()%100):random()%0x8000;
  value.exponent=uint16_t(exponent|(random()&0x8000));
  if(index%1000==0)value.significand={};
  uint16_t input[6];std::memcpy(input,value.significand.data(),10);input[5]=value.exponent;
  uint64_t expected=0;unsigned status=original(input,&expected);
  auto actual=packExtendedDouble(value);
  fixtures.write(reinterpret_cast<const char*>(input),sizeof input);
  fixtures.write(reinterpret_cast<const char*>(&status),sizeof status);
  fixtures.write(reinterpret_cast<const char*>(&expected),sizeof expected);
  if(actual.bits!=expected||actual.status!=status){
   std::cerr<<"Mismatch "<<index<<" exponent "<<std::hex<<value.exponent<<" expected "<<expected<<" actual "<<actual.bits<<std::dec<<" status "<<status<<"/"<<actual.status<<"\n";return 3;
  }
  ++count;
 }
 VirtualFree(memory,0,MEM_RELEASE);
 fixtures.close();if(!fixtures)return 5;
 std::cout<<count<<" original-machine-code differential cases passed\n";
}
