#include "reverse_reference_iterator.h"
#include <fstream>
#include <iostream>
#include <array>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);for(unsigned i=0;i<1024;++i){RecoveredReverseReferenceNodeCandidate head{},node{};head.attribute=(i&2)?0xffffffffu:i;node.next=(i&4)?&head:nullptr;auto*p=firstRecoveredReverseReferenceCandidate((i&1)?&head:nullptr);auto*q=nextRecoveredReverseReferenceCandidate((i&8)?nullptr:&node);std::array<uint32_t,2>actual{p?1u:0u,q?1u:0u},expected{};f.read(reinterpret_cast<char*>(expected.data()),8);if(!f||actual!=expected)throw std::runtime_error("iterator mismatch");}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_reference_iterator_matches=1024"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
