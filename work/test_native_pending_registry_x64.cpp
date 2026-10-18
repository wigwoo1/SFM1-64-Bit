#include "pending_reference_registry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned comparisons=0;
 auto word=[&](){uint32_t v=0;f.read(reinterpret_cast<char*>(&v),4);if(!f)throw std::runtime_error("short oracle");return v;};
 for(unsigned sequence=0;sequence<64;++sequence){RecoveredPendingReferenceRegistryCandidate registry;for(unsigned step=0;step<64;++step){unsigned id=(step*13+sequence)%23;RecoveredPendingReferenceEntryCandidate key{{id%4,id,sequence,0x12345678},{step+100,step*987654u,uint8_t(step&1),step+200,step+300}};
 if(registry.findOrAppend(key)!=word())throw std::runtime_error("insert result mismatch");for(const auto&bucket:registry.buckets()){if(bucket.size()!=word())throw std::runtime_error("bucket size mismatch");for(const auto&e:bucket){std::array<uint32_t,9>actual{e.identity[0],e.identity[1],e.identity[2],e.identity[3],e.record.handle,e.record.counts,e.record.flags,e.record.headAttribute,uint32_t(e.record.next)};for(auto v:actual)if(v!=word())throw std::runtime_error("registry state mismatch");}}++comparisons;}}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle data");std::cout<<"native_pending_registry_insert_matches="<<comparisons<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
