#include "undo_record_base_methods_candidate.h"
#include <cassert>
#include <iostream>
struct Record{};
struct Adapter { Record* seen=nullptr; int calls=0; void invokeDestructorSlot8(Record* p,int flag){assert(flag==1);seen=p;++calls;} };
int main(){
    Adapter a;Record r;disposeUndoRecordCandidate(static_cast<Record*>(nullptr),a);assert(a.calls==0);
    disposeUndoRecordCandidate(&r,a);assert(a.calls==1 && a.seen==&r);
    for(std::uint32_t i=0;i<65536;++i){std::uint8_t m=0;setUndoRecordMarkerCandidate(m,i);assert(undoRecordMarkerCandidate(m)==(i&255));}
    std::cout<<"x64 undo record base methods: null disposal/slot8 flag/raw marker 65536 inputs passed\n";
}
