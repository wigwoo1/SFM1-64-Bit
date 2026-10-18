#include "reference_handle_allocation.h"
#include <array>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;auto word=[&](){uint32_t v=0;f.read(reinterpret_cast<char*>(&v),4);if(!f)throw std::runtime_error("short oracle");return v;};
 for(unsigned queue=0;queue<65;++queue)for(unsigned gen=0;gen<16;++gen){std::vector<RecoveredReferenceHandleSlotCandidate>slots(8,{gen|0x80000000u,0x1234});std::vector<uint32_t>ring{0,1,2,3,4,5,6,7};int32_t head=queue==64?-1:int32_t(queue/8),tail=queue==64?-1:int32_t(queue%8);uint32_t active=9,result=allocateRecoveredReferenceHandleCandidate(slots,active,ring,head,tail);std::array<uint32_t,5>actual{result&0x7fffffffu,active,uint32_t(slots.size()),uint32_t(head),uint32_t(tail)};for(auto v:actual)if(v!=word())throw std::runtime_error("native allocation state mismatch");for(auto s:slots)if(s.state!=word()||uint32_t(s.element)!=word())throw std::runtime_error("native allocation slot mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_handle_allocation_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
