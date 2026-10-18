#pragma once
#include <cstdint>
template<class Adapter>
void undoOperationCandidate(Adapter& a) {
    a.beginUndoNotification();
    a.logUndoBegin();
    const auto saved=static_cast<std::uint8_t>(a.flags74()&8);
    a.setFlags74(static_cast<std::uint8_t>(a.flags74()&0xf7));
    auto remaining=a.undoCount();
    while(remaining>0) {
        auto* record=a.popLiveUndoTail();
        a.logRecord(a.recordLabel(record));
        a.replayUndo(record);
        a.appendRedo(record);
        if(a.recordMarker(record))break;
        --remaining;
    }
    a.logUndoEnd();
    a.setFlags74(static_cast<std::uint8_t>((a.flags74()&0xf7)|saved));
    if(a.field54()<=0)a.clearFields78And7c();
    a.endUndoNotification(0);
}
