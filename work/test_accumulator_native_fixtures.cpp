#include "decimal_accumulator.h"
#include <fstream>
#include <iostream>
#include <cstring>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");
 std::ifstream fixtures("work/original_accumulator_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 unsigned char length;unsigned count=0;
 while(fixtures.read(reinterpret_cast<char*>(&length),1)){
  char digits[24];uint32_t expected[3];
  if(length<1||length>24||!fixtures.read(digits,24)||!fixtures.read(reinterpret_cast<char*>(expected),12))return 2;
  const auto actual=decimalExtendedInteger(std::string_view(digits,length));
  if(std::memcmp(expected,actual.data(),12))return 3;++count;
 }
 if(!fixtures.eof()||count!=24000)return 4;
 std::cout<<count<<" original accumulator fixtures passed in x64 executable\n";
}
