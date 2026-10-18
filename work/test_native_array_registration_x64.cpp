#include "array_reference_registration.h"
#include <fstream>
#include <array>
#include <vector>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;
for(unsigned count=0;count<=5;++count){unsigned patterns=1;for(unsigned i=0;i<count;++i)patterns*=3;for(unsigned pattern=0;pattern<patterns;++pattern)for(unsigned gates=0;gates<16;++gates)for(unsigned removal=0;removal<2;++removal){std::vector<uint32_t>targets;unsigned value=pattern;for(unsigned i=0;i<count;++i){targets.push_back(value%3);value/=3;}std::array<uint32_t,8>actual{},expected{};
registerRecoveredArrayReferencesCandidate(1,targets,7,removal!=0,[&](uint32_t h){return h&&(gates&1)?h:0xffffffffu;},[&](uint32_t){return gates&2;},[&](uint32_t){return gates&4;},[&](uint32_t,uint32_t){return gates&8;},[&](uint32_t t,uint32_t,bool remove){actual[++actual[0]]=t|(remove?0x100:0);});f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native array registration mismatch");++cases;}}
char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_array_registration_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
