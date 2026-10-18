#include "reference_cleanup_dispatch.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;for(unsigned state=0;state<256;++state)for(unsigned added=0;added<256;++added){uint8_t flags=uint8_t(state);uint32_t calls=0,seen=0xffffffffu;dispatchRecoveredReferenceCleanupCandidate(flags,[&](){++calls;seen=flags;flags|=uint8_t(added);});std::array<uint32_t,3>actual{flags,calls,seen},expected{};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native cleanup dispatch mismatch");++cases;}if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle");std::cout<<"native_reference_cleanup_dispatch_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
