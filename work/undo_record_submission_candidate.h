#pragma once
#include <cstdint>
//10390ce0. Helper semantics remain explicit; native vector stores pointers.
template<class Record,class Adapter>
inline void submitUndoRecordCandidate(Record* record,Adapter& a) {
    if(!record)return;
    a.setField70(a.field70()+std::uint32_t{1});
    a.clearRedoRecords();
    a.logSubmittedRecord(a.recordLabel(record));
    if(a.count()>=a.capacity())a.growRecords(1);
    a.incrementCountAndRefreshBase();
    if(auto* slot=a.lastRecordSlot())*slot=record;
    if(a.flags74()&1) {
        a.setRecordMarker(record,1);
        a.setFlags74(static_cast<std::uint8_t>(a.flags74()&0xfe));
    }
}
