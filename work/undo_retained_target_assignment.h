#pragma once
#include <cstdint>
// Native 1025BF80. Incoming unresolved targets survive class filtering.
template<class Release,class Resolve,class Accept,class Retain>
void assignRecoveredUndoRetainedTargetCandidate(uint32_t&storage,uint32_t incoming,Release release,Resolve resolve,Accept accept,Retain retain){
 if(incoming==storage)return;release(storage);storage=incoming;
 if(storage!=0xffffffffu){auto target=resolve(storage);if(target&&!accept(target))storage=0xffffffffu;}
 retain(storage);
}
