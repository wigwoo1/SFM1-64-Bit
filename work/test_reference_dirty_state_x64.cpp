#include "reference_dirty_state.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned initial=0;initial<256;++initial)for(unsigned lowflags=0;lowflags<16;++lowflags)for(unsigned request=0;request<4;++request)for(unsigned pattern=0;pattern<4;++pattern){uint8_t state=uint8_t(initial);uint32_t flags=0x87654320u|lowflags;std::array<uint32_t,8>actual{},expected{};auto event=[&](uint32_t c){actual[2+actual[1]++]=c;};event(1);if(dirtyRecoveredReferenceStateCandidate(state,flags,request,pattern&1,pattern&2,[&](){event(2);},[&](){event(3);}))event(4);actual[0]=0xa500beefu|(uint32_t(state)<<16);actual[7]=flags;if(!f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");++cases;}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_dirty_state_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
