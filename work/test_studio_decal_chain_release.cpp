#include <vector>
#include <iostream>
#include <stdexcept>
#include "studio_decal_chain_release_candidate.h"
static void require(bool v){if(!v)throw std::runtime_error("chain contract");}
int main(){try{unsigned cases=0;
for(unsigned length=1;length<=128;++length)for(unsigned mask=0;mask<8;++mask)for(unsigned relocate=0;relocate<2;++relocate){
 std::vector<StudioDecalChainNodeCandidate> original(length),replacement;
 for(unsigned i=0;i<length;++i){auto& n=original[i];n.next=i+1<length?uint16_t(i+1):uint16_t(0xffff);n.contribution=uint16_t(i+1);n.array={uintptr_t(0x100000000ULL+i*3+1),7,(mask&1)?-1:0};n.list48={uintptr_t(0x100000000ULL+i*3+2),8,(mask&2)?-1:0};n.list24={uintptr_t(0x100000000ULL+i*3+3),9,(mask&4)?-1:0};n.arrayCount=10;n.arrayCache=123;}
 replacement=original;auto* active=&original;uint16_t freeHead=0xffff;uint32_t counter=3;unsigned unlinks=0,removes=0,frees=0;std::vector<uintptr_t> released;
 studioDecalChainReleaseCandidate(0,freeHead,counter,[&](uint16_t i){return &(*active)[i];},[&](uint16_t i){require(i==unlinks++);if(relocate&&i==0){active=&replacement;original[0].next=0xffff;}},[&](auto& b){require(removes%2==0);++removes;require(b.capacity==9);},[&](auto& b){require(removes%2==1);++removes;require(b.capacity==8);},[&](uintptr_t p){require(p>0xffffffffULL);released.push_back(p);++frees;});
 require(unlinks==length&&removes==2*length&&counter==uint32_t(3-length*(length+1)/2));unsigned per=3-unsigned(bool(mask&1))-unsigned(bool(mask&2))-unsigned(bool(mask&4));require(frees==per*length);require(freeHead==length-1);
 for(unsigned j=0;j<length;++j){auto& n=(*active)[j];require(n.next==(j?uint16_t(j-1):uint16_t(0xffff)));require(n.arrayCount==0&&n.arrayCache==n.array.address);require(bool(n.array.address)==bool(mask&1)&&bool(n.list48.address)==bool(mask&2)&&bool(n.list24.address)==bool(mask&4));require(n.array.capacity==((mask&1)?7:0)&&n.list48.capacity==((mask&2)?8:0)&&n.list24.capacity==((mask&4)?9:0));}
 unsigned cursor=0;for(unsigned i=0;i<length;++i){if(!(mask&4))require(released[cursor++]==uintptr_t(0x100000000ULL+i*3+3));if(!(mask&2))require(released[cursor++]==uintptr_t(0x100000000ULL+i*3+2));if(!(mask&1))require(released[cursor++]==uintptr_t(0x100000000ULL+i*3+1));}++cases;
}
std::cout<<"chain_cases="<<cases<<"\n";return 0;}catch(const std::exception&e){std::cerr<<e.what();return 1;}}
