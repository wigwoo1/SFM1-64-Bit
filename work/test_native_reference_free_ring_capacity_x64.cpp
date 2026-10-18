#include "reference_free_ring_capacity.h"
#include <array>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;
 for(unsigned old=0;old<=32;++old)for(int grow=-1;grow<=8;++grow)for(unsigned add=1;add<=16;++add)for(unsigned present=0;present<2;++present){uint32_t capacity=recoveredReferenceFreeRingCapacityCandidate(old,grow,add);std::array<uint32_t,4>actual{capacity,uint32_t(grow>=0&&!present),uint32_t(grow>=0&&present),grow>=0?capacity*4:0},expected{};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native capacity mismatch");++cases;}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_free_ring_capacity_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
