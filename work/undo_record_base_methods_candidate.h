#pragma once
#include <cstdint>
template<class Record,class Adapter>
void disposeUndoRecordCandidate(Record* record,Adapter& a) {
    if(record)a.invokeDestructorSlot8(record,1);
}
inline std::uint8_t undoRecordMarkerCandidate(std::uint8_t marker){return marker;}
inline void setUndoRecordMarkerCandidate(std::uint8_t& marker,std::uint32_t input){marker=static_cast<std::uint8_t>(input);}
