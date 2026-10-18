#include "undo_operation_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Record{int id;unsigned char marker;};
struct Adapter {
    Record r1{1,1},r2{2,0},r3{3,0};
    std::vector<Record*> undo{&r1,&r2,&r3},redo;
    std::vector<int> events;unsigned char flags=0x98;int depth=0;
    void beginUndoNotification(){events.push_back(1);}void logUndoBegin(){events.push_back(2);}
    auto flags74(){return flags;}void setFlags74(unsigned char v){flags=v;}
    int undoCount(){return static_cast<int>(undo.size());}
    Record* popLiveUndoTail(){auto* p=undo.back();undo.pop_back();return p;}
    int recordLabel(Record* r){assert(!(flags&8));return r->id;}
    void logRecord(int id){events.push_back(10+id);}
    void replayUndo(Record* r){assert(!(flags&8));assert(undo.size()==static_cast<std::size_t>(r->id-1));flags|=0x40;events.push_back(20+r->id);}
    void appendRedo(Record* r){redo.push_back(r);}
    auto recordMarker(Record* r){assert(redo.back()==r);return r->marker;}
    void logUndoEnd(){events.push_back(3);}
    int field54(){return depth;}void clearFields78And7c(){events.push_back(4);}
    void endUndoNotification(int v){assert(v==0);events.push_back(5);}
};
int main(){Adapter a;undoOperationCandidate(a);assert(a.undo.empty() && a.redo.size()==3 && a.flags==0xd8);
    assert((a.events==std::vector<int>{1,2,13,23,12,22,11,21,3,4,5}));
    Adapter b;b.r3.marker=128;b.depth=1;b.flags=0x90;undoOperationCandidate(b);assert(b.undo.size()==2 && b.redo.size()==1 && b.flags==0xd0);
    Adapter e;e.undo.clear();undoOperationCandidate(e);assert((e.events==std::vector<int>{1,2,3,4,5}));
    std::cout<<"x64 undo operation: group boundary/pop before replay/append before marker/live flags/empty notifications passed\n";
    return 0;
}
