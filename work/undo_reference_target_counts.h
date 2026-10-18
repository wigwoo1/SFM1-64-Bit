#pragma once
#include "undo_reference_count.h"
// Native kind-2 branches 10359850 / 103599A0. Record lookup includes pending.
template<class Lookup,class NeedsCleanup>
void mutateRecoveredUndoTargetCountCandidate(uint32_t handle,bool release,uint8_t&frameworkState,Lookup lookup,NeedsCleanup needsCleanup){
 if(handle==0xffffffffu)return;auto*record=lookup(handle);if(!record)return;
 if(release){record->counts=releaseUndoReferenceCount(record->counts);if(!(record->counts&0x3ff00000u)&&needsCleanup(handle))frameworkState|=0x10;}
 else{if(!(record->counts&0x3ff00000u)&&needsCleanup(handle))frameworkState|=0x10;record->flags|=1;record->counts=retainUndoReferenceCount(record->counts);}
}
