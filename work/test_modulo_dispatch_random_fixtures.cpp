#include "expression_modulo.h"
#include <fstream>
#include <iostream>
#include <cstring>
int main(){static_assert(sizeof(void*)==8,"x64 replay required");std::ifstream fixtures("work/original_modulo_dispatch_fixtures.bin",std::ios::binary);if(!fixtures)return 1;
 uint32_t inputs[2],expectedBits;unsigned count=0;
 while(fixtures.read(reinterpret_cast<char*>(inputs),8)){if(!fixtures.read(reinterpret_cast<char*>(&expectedBits),4))return 2;float left,right;std::memcpy(&left,inputs,4);std::memcpy(&right,inputs+1,4);auto actual=expressionModulo(left,right);if(!actual)return 3;uint32_t bits;std::memcpy(&bits,&*actual,4);if(bits!=expectedBits)return 4;++count;}
 if(!fixtures.eof()||fixtures.gcount()!=0||count!=100000)return 5;std::cout<<count<<" native modulo dispatch fixtures passed in x64 executable\n";
}
