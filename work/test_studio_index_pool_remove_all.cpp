#include "studio_index_pool_remove_all_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
void check(bool ok){if(!ok)std::abort();}
int main(){unsigned cases=0;
 for(unsigned length=0;length<=256;++length)for(unsigned prior=0;prior<=8;++prior){std::vector<EngineShadowPoolLinkCandidate> links(length+prior);for(unsigned i=0;i<length;++i)links[i]={i?uint16_t(i-1):uint16_t(0xffff),i+1<length?uint16_t(i+1):uint16_t(0xffff)};for(unsigned i=length;i<length+prior;++i)links[i]={uint16_t(i),i+1<length+prior?uint16_t(i+1):uint16_t(0xffff)};
  EngineShadowPoolStateCandidate pool{links.data(),int32_t(links.size()),length?uint16_t(0):uint16_t(0xffff),length?uint16_t(length-1):uint16_t(0xffff),uint16_t(length),uint16_t(links.empty()?0:links.size()-1)};uint16_t freeHead=prior?uint16_t(length):uint16_t(0xffff);studioIndexPoolRemoveAllCandidate(pool,freeHead);check(pool.head==0xffff&&pool.tail==0xffff&&pool.count==0);uint16_t node=freeHead;for(unsigned i=0;i<links.size();++i){check(node==i&&links[i].previous==i);node=links[i].next;}check(node==0xffff);++cases;
 }
 EngineShadowPoolStateCandidate unused;unused.maximumIndex=0xffff;unused.head=17;unused.tail=19;unused.count=7;uint16_t freeHead=23;studioIndexPoolRemoveAllCandidate(unused,freeHead);check(unused.head==17&&unused.tail==19&&unused.count==7&&freeHead==23);
 std::printf("PASS: %u remove-all states with prior free chains; sentinel-highwater no-op. Native parity pending.\n",cases);
}
