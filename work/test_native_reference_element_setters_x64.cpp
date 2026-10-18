#include "reference_runtime_element.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");uint32_t seed=0x41579;
 for(unsigned i=0;i<4096;++i){std::array<uint32_t,16>original{},expected{};RecoveredPendingIdentityCandidate identity{};for(auto&v:original){seed=seed*1664525u+1013904223u;v=seed;}for(auto&v:identity){seed=seed*1664525u+1013904223u;v=seed;}seed=seed*1664525u+1013904223u;RecoveredReferenceRuntimeElementCandidate element{{original[1],original[2],original[3],original[4]},{original[7],original[8],uint8_t(original[9]),original[10],original[11]}};
 setRecoveredReferenceElementIdentityCandidate(element,identity);setRecoveredReferenceElementHandleCandidate(element,seed);auto actual=original;for(unsigned w=0;w<4;++w)actual[1+w]=element.identity[w];actual[7]=element.record.handle;actual[8]=element.record.counts;actual[9]=(actual[9]&0xffffff00u)|element.record.flags;actual[10]=element.record.headAttribute;actual[11]=uint32_t(element.record.next);f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native element setter mismatch");}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_element_setter_matches=4096"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
