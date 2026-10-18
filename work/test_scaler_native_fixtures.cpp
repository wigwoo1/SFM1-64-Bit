#include "decimal_scaler.h"
#include <fstream>
#include <cstring>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");
 std::ifstream fixtures("work/original_scaler_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 uint16_t input[6],expected[6];unsigned count=0;int exponent;
 while(fixtures.read(reinterpret_cast<char*>(input),12)){
  if(!fixtures.read(reinterpret_cast<char*>(&exponent),4)||!fixtures.read(reinterpret_cast<char*>(expected),12))return 2;
  NormalizedExtended value{};std::memcpy(value.significand.data(),input,10);value.exponent=input[5];
  auto actual=scaleDecimalExtended(value,exponent);
  if(std::memcmp(actual.significand.data(),expected,10)||actual.exponent!=expected[5])return 3;++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=30401)return 4;
 std::cout<<count<<" original scaler fixtures passed in x64 executable\n";
}
