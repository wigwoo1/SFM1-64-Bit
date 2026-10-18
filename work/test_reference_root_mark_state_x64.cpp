#include "reference_root_mark_state.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle path required");std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;
for(unsigned mask=0;mask<256;++mask)for(unsigned enabled=0;enabled<4;++enabled)for(unsigned context=0;context<64;++context){uint32_t expected=0;if(!f.read(reinterpret_cast<char*>(&expected),4))throw std::runtime_error("short oracle");RecoveredReferenceRegistrationStateCandidate state{uint16_t(mask),8};bool visit=beginRecoveredReferenceRootMarkCandidate(state,enabled,context);if(visit!=(uint8_t((mask&(1u<<(context&31)))!=0)!=uint8_t(enabled)))throw std::runtime_error("visit mismatch");if(expected!=(0xA5080000u|state.contexts)||state.state!=8)throw std::runtime_error("native state mismatch");++cases;}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_root_mark_state_cases="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
