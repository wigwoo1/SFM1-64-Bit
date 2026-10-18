#include "graph_bookmark_undo_lifecycle_fixture.h"
#include "undo_operation_candidate.h"
#include "redo_operation_candidate.h"
struct StackAdapter{
 LifecycleReplay replay;std::vector<GraphBookmarkUndoCandidate*> undo,redo;unsigned char flags=0x98;unsigned notifications=0;
 void beginUndoNotification(){++notifications;}void beginRedoNotification(){++notifications;}
 void logUndoBegin(){}void logUndoEnd(){}void logRedoBegin(){}void logRedoEnd(){}
 auto flags74(){return flags;}void setFlags74(unsigned char n){flags=n;}
 int undoCount(){return int(undo.size());}int redoCount(){return int(redo.size());}
 auto* popLiveUndoTail(){auto* p=undo.back();undo.pop_back();return p;}
 auto* popLiveRedoTail(){auto* p=redo.back();redo.pop_back();return p;}
 auto* liveRedoTail(){return redo.back();}
 const char* recordLabel(GraphBookmarkUndoCandidate* p){assert(!(flags&8));return p->identity.label;}
 void logRecord(const char*){}
 void replayUndo(GraphBookmarkUndoCandidate* p){assert(!(flags&8));replay.record=p;graphBookmarkUndoReplayCandidate(false,replay);}
 void replayRedo(GraphBookmarkUndoCandidate* p){assert(!(flags&8));replay.record=p;graphBookmarkUndoReplayCandidate(true,replay);}
 void appendUndo(GraphBookmarkUndoCandidate* p){undo.push_back(p);}void appendRedo(GraphBookmarkUndoCandidate* p){redo.push_back(p);}
 unsigned char recordMarker(GraphBookmarkUndoCandidate*){return 1;}
 int field54(){return 0;}void clearFields78And7c(){}
 void endUndoNotification(int n){assert(n==0);}void endRedoNotification(int n){assert(n==0);}
 GraphBookmarkUndoCandidate* capture(const std::vector<std::uint32_t>& values){
  auto* p=new GraphBookmarkUndoCandidate;ConstructorAdapter a{replay};
  graphBookmarkUndoConstructorCandidate(*p,&replay.attribute,values.data(),int(values.size()),0,a);return p;
 }
 void setStorage(const std::vector<std::uint32_t>& v){replay.storage.data=v;replay.storage.count=int(v.size());replay.storage.cap=unsigned(v.size());}
};
int main(){
 StackAdapter a;const std::vector<std::uint32_t> original{1,2},first{3,0x80000000u,5},second{0xffffffffu};
 a.setStorage(original);auto* r1=a.capture(first);a.setStorage(first);auto* r2=a.capture(second);a.setStorage(second);a.undo={r1,r2};
 for(int i=0;i<100;++i){
  undoOperationCandidate(a);a.replay.verify(first);assert(a.undo.size()==1&&a.redo.size()==1&&a.flags==0x98);
  undoOperationCandidate(a);a.replay.verify(original);assert(a.undo.empty()&&a.redo.size()==2);
  redoOperationCandidate(a);a.replay.verify(first);assert(a.undo.size()==1&&a.redo.size()==1);
  redoOperationCandidate(a);a.replay.verify(second);assert(a.undo.size()==2&&a.redo.empty()&&a.flags==0x98);
 }
 assert(a.notifications==400);ReleaseAdapter release;
 graphBookmarkUndoReleaseCandidate(r1,1,release);graphBookmarkUndoReleaseCandidate(r2,1,release);
 a.undo.clear();a.replay.record=nullptr;assert(release.buffers==4&&release.objects==2);
 std::cout<<"x64 bookmark undo stack: 100 two-edit round trips, boundaries, recording suppression and record cleanup passed\n";
}
