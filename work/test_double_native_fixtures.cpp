#include "extended_double.h"
#include <fstream>
#include <iostream>
#include <cstring>
int main(){
 static_assert(sizeof(void*)==8,"Fixture replay must be a 64-bit executable");
 std::ifstream fixtures("work/original_double_fixtures.bin",std::ios::binary);
 if(!fixtures)return 1;
 unsigned count=0;uint16_t input[6];
 while(fixtures.read(reinterpret_cast<char*>(input),sizeof input)){
  unsigned status;uint64_t bits;
  if(!fixtures.read(reinterpret_cast<char*>(&status),sizeof status)||!fixtures.read(reinterpret_cast<char*>(&bits),sizeof bits))return 2;
  NormalizedExtended value{};std::memcpy(value.significand.data(),input,10);value.exponent=input[5];
  auto actual=packExtendedDouble(value);
  if(actual.bits!=bits||actual.status!=status){std::cerr<<"Mismatch "<<count<<"\n";return 3;}++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=50000)return 4;
 std::cout<<count<<" original-machine-code fixtures passed in x64 executable\n";
}
