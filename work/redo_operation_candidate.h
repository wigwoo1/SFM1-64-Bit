#pragma once
#include <cstdint>
template<class Adapter>
void redoOperationCandidate(Adapter& a) {
    a.beginRedoNotification();a.logRedoBegin();
    const auto saved=static_cast<std::uint8_t>(a.flags74()&8);
    a.setFlags74(static_cast<std::uint8_t>(a.flags74()&0xf7));
    while(a.redoCount()>0) {
        auto* record=a.popLiveRedoTail();
        a.logRecord(a.recordLabel(record));
        a.replayRedo(record);a.appendUndo(record);
        if(a.redoCount()<=0)break;
        if(a.recordMarker(a.liveRedoTail()))break;
    }
    a.logRedoEnd();
    a.setFlags74(static_cast<std::uint8_t>((a.flags74()&0xf7)|saved));
    if(a.field54()<=0)a.clearFields78And7c();
    a.endRedoNotification(0);
}
