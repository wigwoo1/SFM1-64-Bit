#include "reference_registration_observers.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);for(unsigned length=0;length<=16;++length)for(unsigned pattern=0;pattern<8;++pattern){std::vector<unsigned>list;for(unsigned i=0;i<length;++i)list.push_back(pattern&4?i%3:i);std::array<uint32_t,20>actual{},expected{};int element=0;notifyRecoveredReferenceRegistrationObserversCandidate<unsigned>(element,pattern&1,[&](auto visit){for(auto id:list)visit(id);},[&](unsigned id,int&e){if(&e!=&element)throw std::runtime_error("element mismatch");actual[1+actual[0]++]=id;if(pattern&2)list.clear();});actual[19]=!list.empty();if(!f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_registration_observer_cases=136"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
