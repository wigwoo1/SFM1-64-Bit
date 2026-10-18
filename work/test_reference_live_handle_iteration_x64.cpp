#include "reference_live_handle_iteration.h"
#include "reference_handle_resolution.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream file(argv[1],std::ios::binary);for(unsigned seed=0;seed<4096;++seed){std::vector<RecoveredReferenceHandleSlotCandidate>slots(8);uint32_t r=seed+1;for(auto&s:slots){r=r*1664525u+1013904223u;s.state=((r>>8)%513)|(r&0x80000000u);s.element=r&4?0x1234:0;}std::array<uint32_t,64>actual{},expected{};auto resolve=[&](uint32_t h){actual[16+actual[6]++]=h;return resolveRecoveredReferenceHandleCandidate(h,slots);};actual[0]=firstRecoveredReferenceLiveHandleCandidate(slots,resolve);std::array<uint32_t,5>current{0,3,7,0xffffffffu,0x80000000u};for(unsigned i=0;i<5;++i)actual[1+i]=nextRecoveredReferenceLiveHandleCandidate(slots,current[i],resolve);if(!file.read(reinterpret_cast<char*>(expected.data()),sizeof(expected))||actual!=expected)throw std::runtime_error("oracle mismatch");}char extra;if(file.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_live_handle_iteration_cases=4096"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
