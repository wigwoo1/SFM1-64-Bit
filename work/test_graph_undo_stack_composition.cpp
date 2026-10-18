#define main standaloneUndoFixtureMain
#include "test_undo_operation.cpp"
#undef main
#include "redo_operation_candidate.h"
#include "graph_undo_methods_candidate.h"
struct GraphOwner{unsigned counter=0;};struct GraphRecord{GraphOwner* owner;};
struct GraphStackAdapter:Adapter{
 GraphOwner graphOwner;GraphRecord graphRecord{&graphOwner};unsigned services=0;
 GraphOwner* owner14(GraphRecord* r){return r->owner;}unsigned counter29c(GraphOwner* p){return p->counter;}void setCounter29c(GraphOwner* p,unsigned v){p->counter=v;}
 void serviceSlot10(){assert(!(flags&8));++services;}
 void replayUndo(Record*){assert(!(flags&8));graphUndoSlot0Candidate(&graphRecord,*this);}
 void replayRedo(Record*){assert(!(flags&8));graphUndoSlot1Candidate(&graphRecord,*this);}
 void beginRedoNotification(){}void logRedoBegin(){}void logRedoEnd(){}void endRedoNotification(int v){assert(v==0);}
 int redoCount(){return static_cast<int>(redo.size());}Record* popLiveRedoTail(){auto* p=redo.back();redo.pop_back();return p;}Record* liveRedoTail(){return redo.back();}
 void appendUndo(Record* p){undo.push_back(p);}unsigned char recordMarker(Record* p){return p->marker;}
};
int main(){GraphStackAdapter a;undoOperationCandidate(a);assert(a.undo.empty() && a.redo.size()==3 && a.graphOwner.counter==3 && !a.services);redoOperationCandidate(a);assert(a.redo.empty() && a.undo.size()==3 && !a.graphOwner.counter && a.services==3 && a.flags==0x98);for(unsigned i=0;i<100;++i){undoOperationCandidate(a);assert(a.graphOwner.counter==3);redoOperationCandidate(a);assert(!a.graphOwner.counter);}assert(a.services==303);std::cout<<"x64 graph undo/redo stack composition: slot0 undo, slot1 redo, recording gate and101 counter round trips passed\n";}
