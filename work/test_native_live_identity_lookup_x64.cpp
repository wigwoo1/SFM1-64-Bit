#include "live_reference_identity_lookup.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;
 for(unsigned mode=0;mode<2;++mode)for(unsigned hash=0;hash<8;++hash)for(unsigned count=0;count<=6;++count)for(unsigned pattern=0;pattern<64;++pattern)for(unsigned missing=0;missing<8;++missing){RecoveredPendingIdentityCandidate key{hash,17,29,41};std::array<RecoveredPendingIdentityCandidate,6>identities{};for(unsigned i=0;i<6;++i){identities[i]=key;if(!(pattern&(1u<<i)))identities[i][3]^=1;}
 uint32_t bucket=0xffffffffu,index=0xabcdef01u,calls=0;bool hit=recoveredLiveIdentityLookupCandidate(key,mode!=0,3,mode?4:3,bucket,index,[&](uint32_t){return count;},[](uint32_t,uint32_t i){return i+1;},[&](uint32_t h)->const RecoveredPendingIdentityCandidate*{++calls;return missing&(1u<<((h-1)%3))?nullptr:&identities[h-1];});std::array<uint32_t,4>actual{uint32_t(hit),bucket,index,calls},expected{};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native live identity mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_live_identity_lookup_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
