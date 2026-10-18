#pragma once
#include <cstdint>
template<class Adapter>
bool endUndoGroupCandidate(Adapter& a) {
    if(!(a.flags74()&8))return false;
    a.logGroupEnd();
    const auto depth=a.depth54()-std::uint32_t{1};a.setDepth54(depth);
    if(depth!=0)return false;
    const auto raw=a.submissionCounter();
    const bool changed=raw!=0 && raw<0x80000000u;
    if(changed){
        a.clearRedoRecords();
        if(a.currentContext()!=0 && a.currentContext()==a.previousContext()) {
            for(auto i=a.undoCount();i>0;--i){
                auto* record=a.undoRecord(i-1);
                if(record && a.recordMarker(record)){a.setRecordMarker(record,0);break;}
            }
        }
    }
    a.setSubmissionCounter(0);return changed;
}
