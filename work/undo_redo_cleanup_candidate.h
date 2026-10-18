#pragma once
#include <cstdint>
// 10390bd0: snapshot loop bound, live vector reload, explicit opaque context.
// Slot 5 behavior remains an adapter operation, not an assumed C++ delete.
template<class Adapter>
void clearRedoRecordsCandidate(Adapter& a) {
    const std::int32_t count=a.redoCount();
    if(count==0)return;
    auto context=a.makeZeroContext();
    a.collectContext(context);
    a.beginContext(context);
    const auto enabled=a.recordingEnabled();
    a.setRecordingEnabled(0);
    for(std::int32_t i=0;i<count;++i) {
        auto* record=a.redoRecord(i);
        a.logClearedRecord(a.recordLabel(record));
        a.invokeRecordSlot5(record);
    }
    a.setRedoCount(0);
    a.endContext(context);
    a.setRecordingEnabled(enabled);
    context.count=0;
    if(context.growSize>=0) {
        if(context.base){a.freeContext(context.base);context.base=nullptr;}
        context.capacity=0;
    }
    context.cachedBase=context.base;
    if(context.growSize>=0 && context.base)a.freeContext(context.base);
}
