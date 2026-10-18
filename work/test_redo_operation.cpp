#include "redo_operation_candidate.h"
#include "undo_operation_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Record{int id;unsigned char marker;};
struct Adapter {
    Record a{1,1},b{2,0},c{3,128},d{4,0};
    std::vector<Record*> undo{&a,&b,&c,&d},redo;
    std::vector<int> replayed;unsigned char flags=8;int begins=0,ends=0,clears=0;
    auto flags74(){return flags;}void setFlags74(unsigned char f){flags=f;}
    int undoCount(){return static_cast<int>(undo.size());}int redoCount(){return static_cast<int>(redo.size());}
    Record* popLiveUndoTail(){auto* r=undo.back();undo.pop_back();return r;}
    Record* popLiveRedoTail(){auto* r=redo.back();redo.pop_back();return r;}
    Record* liveRedoTail(){return redo.back();}
    int recordLabel(Record* r){return r->id;}void logRecord(int){}
    void replayUndo(Record* r){assert(!(flags&8));replayed.push_back(-r->id);flags|=0x40;}
    void replayRedo(Record* r){assert(!(flags&8));replayed.push_back(r->id);flags|=0x20;}
    void appendRedo(Record* r){redo.push_back(r);}void appendUndo(Record* r){undo.push_back(r);}
    auto recordMarker(Record* r){return r->marker;}
    void beginUndoNotification(){++begins;}void beginRedoNotification(){++begins;}
    void logUndoBegin(){}void logUndoEnd(){}void logRedoBegin(){}void logRedoEnd(){}
    int field54(){return 0;}void clearFields78And7c(){++clears;}
    void endUndoNotification(int v){assert(v==0);++ends;}void endRedoNotification(int v){assert(v==0);++ends;}
};
int main(){Adapter a;
    undoOperationCandidate(a);undoOperationCandidate(a);assert(a.undo.empty() && a.redo.size()==4);
    redoOperationCandidate(a);assert(a.undo.size()==2 && a.redo.size()==2 && a.redo.back()==&a.c);
    redoOperationCandidate(a);assert(a.undo.size()==4 && a.redo.empty());
    assert((a.replayed==std::vector<int>{-4,-3,-2,-1,1,2,3,4}));
    assert(a.flags==0x68 && a.begins==4 && a.ends==4 && a.clears==4);
    redoOperationCandidate(a);assert(a.begins==5 && a.ends==5);
    std::cout<<"x64 grouped undo/redo roundtrip: two groups/next-tail marker/raw marker/flags/empty scope passed\n";
}
