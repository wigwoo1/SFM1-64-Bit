#include "pending_reference_registry.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8);if(argc!=2)return 2;std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("missing oracle");unsigned cases=0;RecoveredTransferReferenceRecordCandidate live{};live.headAttribute=0xfffffffeu;
 for(unsigned q=0;q<128;++q){RecoveredPendingReferenceRegistryCandidate registry;for(unsigned b=0;b<2;++b)for(unsigned i=0;i<((q>>(1+b*2))&3);++i){RecoveredPendingReferenceEntryCandidate e{{b,i,q,0},{(i+b+(q>>5))%5,0,0,b*3+i,0}};registry.findOrAppend(e);}
 for(unsigned handle=0;handle<8;++handle)for(unsigned want=0;want<2;++want){RecoveredPendingReferenceEntryCandidate dummy{};auto*pending=&dummy;auto*record=registry.lookupRecord(handle,[&](uint32_t h)->RecoveredTransferReferenceRecordCandidate*{return(q&1)&&h==3?&live:nullptr;},want?&pending:nullptr);
 std::array<uint32_t,2>actual{record?record->headAttribute:0xffffffffu,!want?0xfffffffdu:pending?pending->record.headAttribute:0xfffffffeu},expected{};f.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!f||actual!=expected)throw std::runtime_error("native integrated lookup mismatch");if(record&&pending&&want&&record!=&pending->record)throw std::runtime_error("pending record address mismatch");++cases;}}
 if(f.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing oracle data");std::cout<<"native_pending_registry_lookup_matches="<<cases<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
