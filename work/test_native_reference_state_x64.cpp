#include "reference_registration_state.h"
#include <fstream>
#include <array>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;for(unsigned context=0;context<256;++context)for(unsigned byte=0;byte<256;++byte){uint16_t flags=uint16_t((context*4099u+byte*521u)^0xa55au);RecoveredReferenceRegistrationStateCandidate s{flags,uint8_t(byte)};std::array<uint16_t,4>actual{},expected{};actual[0]=s.eligible(context);actual[1]=s.suppressed();s.setEligible(true,context);actual[2]=s.contexts;s.contexts=flags;s.setEligible(false,context);actual[3]=s.contexts;if(s.state!=byte)throw std::runtime_error("state clobbered");f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native reference state mismatch");++cases;}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_reference_state_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
