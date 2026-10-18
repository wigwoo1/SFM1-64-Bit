#include "reference_free_ring.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");auto word=[&](){uint32_t v=0;f.read(reinterpret_cast<char*>(&v),4);if(!f)throw std::runtime_error("short oracle");return v;};unsigned cases=0;
 for(unsigned queue=0;queue<65;++queue)for(unsigned value=0;value<8;++value){std::vector<uint32_t>ring(8);for(unsigned i=0;i<8;++i)ring[i]=100+i;int32_t head=queue==64?-1:int32_t(queue/8),tail=queue==64?-1:int32_t(queue%8);appendRecoveredReferenceFreeRingCandidate(ring,head,tail,value);if(ring.size()!=word()||uint32_t(head)!=word()||uint32_t(tail)!=word()||uint32_t(ring.size()>8)!=word())throw std::runtime_error("native ring cursor mismatch");for(auto v:ring)if(v!=word())throw std::runtime_error("native ring storage mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_free_ring_append_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
