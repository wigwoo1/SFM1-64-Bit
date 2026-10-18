#include "reference_record_transfer.h"
#include <iostream>
#include <stdexcept>
int main(){try{unsigned checks=0;for(unsigned bits=0;bits<4;++bits)for(unsigned flags=0;flags<256;++flags){RecoveredTransferReferenceRecordCandidate r{7,(bits<<30)|0x3abcdeffu,uint8_t(flags),9,uintptr_t(0x123456789abcdef0ull)};resetRecoveredPendingReferenceRecordCandidate(r);if(r.handle!=0xffffffffu||r.counts!=(bits<<30)||r.flags!=(flags&0xfe)||r.headAttribute!=0xffffffffu||r.next)throw std::runtime_error("pending reset mismatch");++checks;}std::cout<<"pending_reference_reset_policy_checks="<<checks<<std::endl;return 0;}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
