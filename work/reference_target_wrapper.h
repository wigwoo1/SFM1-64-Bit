#pragma once
#include <cstdint>
struct RecoveredReferenceTargetRecordCandidate{uint32_t counts=0;uint8_t flags=0;uint32_t headAttribute=0xffffffffu;};
//103597E0/10359940; mutation callbacks implement verified linked list operations.
template<class Lookup,class TargetNeedsCleanup,class Mutate>
void mutateRecoveredReferenceTargetCandidate(uint32_t target,bool attributePresent,bool removal,uint8_t&frameworkState,Lookup lookup,TargetNeedsCleanup needsCleanup,Mutate mutate){
 if(!attributePresent||target==0xffffffffu)return;auto*record=lookup(target);if(!record)return;
 auto mark=[&](){if(record->headAttribute==0xffffffffu&&!(record->counts&0xffc00u)&&needsCleanup(target))frameworkState|=16;};
 if(removal){mutate(*record,true);mark();}
 else{if(record->flags&1)mark();record->flags|=1;mutate(*record,false);}
}
