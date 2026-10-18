#include "reference_record_lookup.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
struct Record{uint32_t id;};
int main(int argc,char**argv){try{if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);unsigned cases=0;Record live{0xfffffffeu};for(unsigned q=0;q<128;++q){std::vector<std::vector<RecoveredPendingReferenceCandidate<Record>>>buckets(2);for(unsigned b=0;b<2;++b)for(unsigned i=0;i<((q>>(1+b*2))&3);++i)buckets[b].push_back({(i+b+(q>>5))%5,{b*3+i}});for(unsigned handle=0;handle<8;++handle)for(unsigned want=0;want<2;++want){RecoveredPendingReferenceCandidate<Record>dummy{};auto*pending=&dummy;auto*record=lookupRecoveredReferenceRecordCandidate(handle,[&](uint32_t h)->Record*{return(q&1)&&h==3?&live:nullptr;},buckets,want?&pending:nullptr);std::array<uint32_t,2>actual{record?record->id:0xffffffffu,!want?0xfffffffdu:pending?pending->record.id:0xfffffffeu},expected{};f.read(reinterpret_cast<char*>(expected.data()),8);if(!f||actual!=expected)throw std::runtime_error("native lookup mismatch");++cases;}}char extra;if(f.read(&extra,1))throw std::runtime_error("trailing oracle");std::cout<<"native_reference_record_lookup_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
