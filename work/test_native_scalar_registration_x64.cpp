#include "scalar_reference_registration.h"
#include <fstream>
#include <array>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,4>attrs{0,1,2,3};for(unsigned mask=0;mask<16;++mask)for(unsigned gates=0;gates<16;++gates)for(unsigned removal=0;removal<2;++removal){std::array<uint32_t,8>out{},expected{};registerRecoveredScalarReferencesCandidate(attrs,7,removal!=0,[&](uint32_t a){return (mask&(1u<<a))?1:3;},[](uint32_t){return 1u;},[&](uint32_t){return(gates&1)?1u:0xffffffffu;},[&](uint32_t){return gates&2;},[&](uint32_t){return gates&4;},[&](uint32_t,uint32_t){return gates&8;},[&](uint32_t,uint32_t a,bool remove){out[++out[0]]=1+a+(remove?0x100:0);});f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||out!=expected)throw std::runtime_error("registration mismatch");++cases;}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_scalar_registration_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
