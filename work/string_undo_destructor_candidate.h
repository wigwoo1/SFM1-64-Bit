#pragma once
#include <cstdint>
// String record's complete deleting destructor. Pool lifetime is external.
template<class Record,class Adapter>
Record* destroyStringUndoRecordCandidate(Record* record,std::uint32_t flags,Adapter& a) {
    a.setBaseRecordVtable(record);
    if(flags&1)a.freeRecord(record);
    return record;
}
