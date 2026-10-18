#include "undo_reference_target_counts.h"
#include "reference_record_transfer.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <array>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle unavailable");
 for(unsigned seed=0;seed<24576;++seed){RecoveredTransferReferenceRecordCandidate r;r.counts=(seed%1024)<<20|0xc00abcdeu;r.flags=(seed/12288%2)?0xfeu:0xffu;uint8_t state=0xa5;unsigned queries=0;mutateRecoveredUndoTargetCountCandidate(7,(seed/1024%2)!=0,state,[&](uint32_t){return(seed/6144%2)?&r:nullptr;},[&](uint32_t){++queries;return seed/2048%3!=0;});std::array<uint32_t,4>out{r.counts,r.flags,state,queries},expected{};input.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!input||out!=expected)throw std::runtime_error("native mismatch");}
 if(input.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64_undo_target_count_cases=24576 native_counts_flags_cleanup_queries=matched\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
