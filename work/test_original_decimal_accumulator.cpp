#include <windows.h>
#include <fstream>
#include <iostream>
#include <random>
#include <cstring>
#include "decimal_accumulator.h"
#include "original_decimal_accumulator.inc"
int main(){
 auto code=VirtualAlloc(nullptr,sizeof originalAccumulator,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!code)return 1;std::memcpy(code,originalAccumulator,sizeof originalAccumulator);
 DWORD oldProtection;if(!VirtualProtect(code,sizeof originalAccumulator,PAGE_EXECUTE_READ,&oldProtection))return 2;
 FlushInstructionCache(GetCurrentProcess(),code,sizeof originalAccumulator);
 using Original=void(__cdecl*)(const unsigned char*,unsigned,uint32_t*);
 auto original=reinterpret_cast<Original>(code);
 std::ofstream fixtures("work/original_accumulator_fixtures.bin",std::ios::binary);if(!fixtures)return 3;
 std::mt19937 random(32108);
 for(unsigned index=0;index<24000;++index){
  unsigned char length=static_cast<unsigned char>(1+index%24);char digits[24]{};unsigned char numeric[24]{};
  for(unsigned i=0;i<length;++i){numeric[i]=static_cast<unsigned char>(random()%10);digits[i]=char('0'+numeric[i]);}
  // Native retained digits omit leading zeros; never invoke its zero loop.
  numeric[0]=static_cast<unsigned char>(1+random()%9);digits[0]=char('0'+numeric[0]);
  uint32_t expected[3]{};original(numeric,length,expected);
  const auto actual=decimalExtendedInteger(std::string_view(digits,length));
  if(std::memcmp(expected,actual.data(),12)){std::cerr<<"Mismatch "<<index<<"\n";return 4;}
  fixtures.write(reinterpret_cast<const char*>(&length),1);fixtures.write(digits,24);fixtures.write(reinterpret_cast<const char*>(expected),12);
 }
 fixtures.close();VirtualFree(code,0,MEM_RELEASE);if(!fixtures)return 5;
 std::cout<<"24000 original accumulator differential cases passed\n";
}
