#include "pack_color_conversion.h"
#include "pack_color_fixture.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream input(argv[1],std::ios::binary);for(uint32_t seed=0;seed<4096;++seed){std::array<float,4>values;for(unsigned i=0;i<4;++i){auto bits=packColorFixtureBits(seed,i);std::memcpy(&values[i],&bits,4);}uint32_t expected;input.read(reinterpret_cast<char*>(&expected),4);if(!input||expected!=recoveredPackColorCandidate(values)){std::cerr<<"pack color mismatch seed="<<seed<<"\n";return 1;}}if(input.peek()!=EOF)return 2;std::cout<<"x64 pack_color_cases=4096 native_bits=matched\n";}
