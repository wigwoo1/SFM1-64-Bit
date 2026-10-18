#include "reference_registration_update.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);for(unsigned initial=0;initial<256;++initial)for(unsigned enabled=0;enabled<4;++enabled){uint8_t state=uint8_t(initial);uint32_t flags=0x87654321u;std::array<uint32_t,24>actual{},expected{};auto event=[&](uint32_t c){actual[2+actual[1]++]=c;};updateRecoveredReferenceRegistrationCandidate(state,flags,enabled,[&](){event(1);},[&](){event(2);},[&](uint32_t m){if(m!=3)throw std::runtime_error("mode");event(3);},[&](uint32_t offset){event(offset);});actual[0]=0xa500beefu|(uint32_t(state)<<16);actual[23]=flags;if(!f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_registration_update_cases=1024"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
