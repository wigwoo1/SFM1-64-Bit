#include "reference_removal_retirement_policy.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream f(argv[1],std::ios::binary);for(unsigned mode=0;mode<8;++mode)for(unsigned bits=0;bits<256;++bits){RecoveredTransferReferenceRecordCandidate r{};r.counts=(bits&1?1:0)|(bits&2?0x400:0)|(bits&4?0x100000:0)|(bits&8?0x40000000:0)|(bits&16?0x80000000:0);r.headAttribute=bits&32?1:0xffffffffu;uint32_t expected;if(!f.read(reinterpret_cast<char*>(&expected),4)||expected!=retireRecoveredRemovedElementHandleCandidate(r,mode))throw std::runtime_error("oracle mismatch");}char extra;if(f.get(extra))throw std::runtime_error("extra oracle");std::cout<<"x64_removal_retirement_policy_cases=2048"<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
