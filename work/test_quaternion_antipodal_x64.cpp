#include <fstream>
#include <iostream>
#include <cstring>
#include "quaternion_interpolation.h"
int main(int argc,char**argv){if(argc!=2)return 1;static_assert(sizeof(void*)==8);std::ifstream f(argv[1],std::ios::binary);if(!f)return 1;for(unsigned i=0;i<1001;++i){float fraction;std::array<float,4>expected;f.read(reinterpret_cast<char*>(&fraction),4);f.read(reinterpret_cast<char*>(expected.data()),16);if(!f)return 2;auto actual=interpolateRecoveredQuaternionUnaligned({.5f,.5f,.5f,.5f},{-.5f,-.5f,-.5f,-.5f},fraction);if(std::memcmp(actual.data(),expected.data(),16))return 3;}char extra;if(f.get(extra))return 4;std::cout<<"quaternion_normalize_x64_checks=1001 mismatches=0\n";}
