#include "expression_ceiling.h"
#include <fstream>
#include <iostream>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");std::ifstream fixtures("work/original_ceiling_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 uint32_t inputBits,expectedBits;unsigned count=0;
 while(fixtures.read(reinterpret_cast<char*>(&inputBits),4)){
  if(!fixtures.read(reinterpret_cast<char*>(&expectedBits),4))return 2;float input;std::memcpy(&input,&inputBits,4);auto actual=expressionCeiling(input);if(!actual)return 3;uint32_t bits;std::memcpy(&bits,&*actual,4);if(bits!=expectedBits)return 4;++count;
 }
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=99613)return 5;
 std::cout<<count<<" original ceiling fixtures passed in x64 executable\n";
}
