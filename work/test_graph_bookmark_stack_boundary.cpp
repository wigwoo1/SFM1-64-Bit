#include "graph_bookmark_undo_lifecycle_fixture.h"
#include "undo_operation_candidate.h"
#include "redo_operation_candidate.h"
#include "graph_undo_constructor_candidate.h"
#include "graph_undo_methods_candidate.h"
struct Owner{unsigned counter=0;};
struct Item{bool boundary;GraphBookmarkUndoCandidate* bookmark;GraphUndoObjectCandidate* graph;};
struct BoundaryStack{
 LifecycleReplay replay;Owner owner;unsigned char flags=0x98;std::vector<Item*> undo,redo;unsigned notifications=0;
 Item bookmarkItem{false,nullptr,nullptr},boundaryItem{true,nullptr,nullptr};
 void beginUndoNotification(){++notifications;}void beginRedoNotification(){++notifications;}
 void logUndoBegin(){}void logUndoEnd(){}void logRedoBegin(){}void logRedoEnd(){}
 auto flags74(){return flags;}void setFlags74(unsigned char n){flags=n;}
 int undoCount(){return int(undo.size());}int redoCount(){return int(redo.size());}
 auto* popLiveUndoTail(){auto* p=undo.back();undo.pop_back();return p;}
 auto* popLiveRedoTail(){auto* p=redo.back();redo.pop_back();return p;}
 auto* liveRedoTail(){return redo.back();}
 const char* recordLabel(Item* p){assert(!(flags&8));return p->boundary?p->graph->label:p->bookmark->identity.label;}
 void logRecord(const char*){}
 void replayUndo(Item* p){assert(!(flags&8));if(p->boundary)graphUndoSlot0Candidate(p->graph,*this);else{replay.record=p->bookmark;graphBookmarkUndoReplayCandidate(false,replay);}}
 void replayRedo(Item* p){assert(!(flags&8));if(p->boundary)graphUndoSlot1Candidate(p->graph,*this);else{replay.record=p->bookmark;graphBookmarkUndoReplayCandidate(true,replay);}}
 void appendUndo(Item* p){undo.push_back(p);}void appendRedo(Item* p){redo.push_back(p);}
 unsigned char recordMarker(Item* p){return p->boundary;}
 int field54(){return 0;}void clearFields78And7c(){}
 void endUndoNotification(int n){assert(!n);}void endRedoNotification(int n){assert(!n);}
 auto* owner14(GraphUndoObjectCandidate* p){return static_cast<Owner*>(p->owner14);}
 unsigned counter29c(Owner* p){return p->counter;}void setCounter29c(Owner* p,unsigned n){p->counter=n;}void serviceSlot10(){}
};
int main(){
 BoundaryStack a;const std::vector<std::uint32_t> old{1,2},next{3,4,0xffffffffu};
 a.replay.storage.data=old;a.replay.storage.count=2;a.replay.storage.cap=2;
 auto* copy=new GraphBookmarkUndoCandidate;ConstructorAdapter capture{a.replay};
 graphBookmarkUndoConstructorCandidate(*copy,&a.replay.attribute,next.data(),3,0,capture);
 a.replay.storage.data=next;a.replay.storage.count=3;a.replay.storage.cap=3;
 auto* marker=new GraphUndoObjectCandidate;struct ModelCapture{unsigned captureModelSlot80(const char*){return 0xffffffff;}unsigned captureModelSlot81(const char*){return 0xffffffff;}} model;
 constructGraphUndoCandidate(marker,&a.owner,"Stop Ignoring Log Changes",model);
 a.bookmarkItem.bookmark=copy;a.boundaryItem.graph=marker;a.undo={&a.bookmarkItem,&a.boundaryItem};
 undoOperationCandidate(a);assert(a.replay.storage.count==3&&a.owner.counter==1&&a.undo.size()==1&&a.redo.size()==1&&a.flags==0x98);
 undoOperationCandidate(a);a.replay.verify(old);assert(a.undo.empty()&&a.redo.size()==2&&a.owner.counter==1);
 redoOperationCandidate(a);a.replay.verify(next);assert(a.undo.size()==1&&a.redo.size()==1&&a.owner.counter==1);
 redoOperationCandidate(a);assert(a.undo.size()==2&&a.redo.empty()&&!a.owner.counter&&a.notifications==4);
 ReleaseAdapter release;graphBookmarkUndoReleaseCandidate(copy,1,release);delete marker;
 assert(release.buffers==2&&release.objects==1);
 std::cout<<"x64 bookmark stack boundary: separate editor marker, two-step undo/redo, edit grouping and record release passed\n";
}
