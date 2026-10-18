#include "pending_reference_identity.h"
#include "pending_reference_identity_lookup.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");uint32_t seed=0x38459;
 for(unsigned c=0;c<4096;++c){RecoveredPendingIdentityCandidate key{};for(auto&v:key){seed=seed*1664525u+1013904223u;v=seed;}unsigned count=c%7;uint32_t bucket=0xffffffffu,index=0xabcdef01u;
 bool hit=recoveredPendingIdentityLookupCandidate(key,true,15,16,bucket,index,recoveredPendingIdentityHashCandidate,[&](uint32_t){return count;},[&](uint32_t,uint32_t i){auto e=key;if((c>>(i+3))&1)e[1+i%3]^=1u<<(c%32);return e;},recoveredPendingIdentityEqualCandidate);
 std::array<uint32_t,3>expected{},actual{uint32_t(hit),bucket,index};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native real identity lookup mismatch");}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle data");std::cout<<"native_pending_identity_real_callbacks_matches=4096"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
