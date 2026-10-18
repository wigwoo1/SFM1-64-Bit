#include "reverse_reference_list.h"
#include <array>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;
for(unsigned seq=0;seq<512;++seq){RecoveredReverseReferenceListCandidate list;uint32_t seed=seq+1;for(unsigned step=0;step<16;++step){seed=seed*1664525u+1013904223u;uint32_t value=1+((seed>>16)%5);if(seed&0x80000000u)list.add(value);else list.remove(value);std::array<uint32_t,18>actual{},expected{};for(auto*node=list.first();node;node=nextRecoveredReverseReferenceCandidate(node)){if(actual[0]>=16)throw std::runtime_error("list overflow");actual[1+actual[0]++]=node->attribute;}f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native mutation mismatch");++cases;}}
char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_reference_mutation_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
