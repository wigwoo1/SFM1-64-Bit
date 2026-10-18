#pragma once
#include <cstdint>
struct RecoveredTransferReferenceRecordCandidate{uint32_t handle=0xffffffffu,counts=0;uint8_t flags=0;uint32_t headAttribute=0xffffffffu;uintptr_t next=0;};
//1035DC20: preserve destination count top2bits and all flag bits except bit0.
inline void transferRecoveredReferenceRecordCandidate(RecoveredTransferReferenceRecordCandidate&destination,const RecoveredTransferReferenceRecordCandidate&source){
 destination.handle=source.handle;destination.counts=(destination.counts&0xc0000000u)|(source.counts&0x3fffffffu);
 destination.flags=uint8_t((destination.flags&0xfe)|(source.flags&1));destination.headAttribute=source.headAttribute;destination.next=source.next;
}

//1035A4F7..1035A514 resets fields in a local pending-identity lookup key.
//This alone does not establish initialization of an allocated table entry.
inline void resetRecoveredPendingReferenceRecordCandidate(RecoveredTransferReferenceRecordCandidate&record){
 record.handle=0xffffffffu;record.counts&=0xc0000000u;record.flags&=0xfe;record.headAttribute=0xffffffffu;record.next=0;
}
