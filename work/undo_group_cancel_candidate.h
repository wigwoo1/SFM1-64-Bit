#pragma once
#include <cstdint>
template<class Adapter>
bool cancelUndoGroupCandidate(Adapter& a) {
    if(!(a.flags74()&8))return false;
    const auto raw=a.submissionCounter();
    const bool changed=raw!=0 && raw<0x80000000u;
    a.logGroupCancel();
    a.endGroup();
    if(a.depth54()!=0 || !changed)return false;
    a.undoGroup();a.clearRedoRecords();return true;
}
