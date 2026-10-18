#include "pending_reference_identity_lookup.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;
 for(unsigned mode=0;mode<2;++mode)for(uint32_t hash=0;hash<32;++hash)for(unsigned count=0;count<=6;++count)for(unsigned pattern=0;pattern<64;++pattern){std::array<uint32_t,2>key{hash,77};uint32_t bucket=0xffffffffu,index=0xabcdef01u,hashes=0,equals=0;
 bool hit=recoveredPendingIdentityLookupCandidate(key,mode!=0,3,mode?4:3,bucket,index,[&](const auto&k){++hashes;return k[0];},[&](uint32_t){return count;},[&](uint32_t,uint32_t i){return (pattern&(1u<<i))?77u:88u;},[&](uint32_t e,const auto&k){++equals;return e==k[1];});
 std::array<uint32_t,5>expected{},actual{uint32_t(hit),bucket,index,hashes,equals};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native pending lookup mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle data");std::cout<<"native_pending_identity_lookup_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
