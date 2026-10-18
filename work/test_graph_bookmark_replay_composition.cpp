#include "graph_bookmark_replay_composition_fixture.h"
int main(){
 Replay a;a.old.resize(513);a.next.resize(1025);
 for(unsigned i=0;i<a.old.size();++i)a.old[i]=i*0x1234567u;
 for(unsigned i=0;i<a.next.size();++i)a.next[i]=~(i*0x9e3779b9u);
 a.old[3]=0x80000000u;a.next[4]=0xffffffffu;
 for(int i=0;i<100;++i){graphBookmarkUndoReplayCandidate(false,a);a.verify(a.old);graphBookmarkUndoReplayCandidate(true,a);a.verify(a.next);}
 assert(a.clearNotifications==200&&a.dirty==0x4000&&a.clears==200&&a.insertNotifications==153800&&a.insertions==153800&&a.storage.cap==2048);
 const auto growth=a.storage.growth;auto* base=a.storage.base0();
 a.next.clear();graphBookmarkUndoReplayCandidate(true,a);a.verify(a.next);
 graphBookmarkUndoReplayCandidate(false,a);a.verify(a.old);
 assert(a.insertNotifications==154313&&a.clearNotifications==202&&a.storage.cap==2048&&a.storage.growth==growth&&a.storage.base0()==base);
 std::cout<<"x64 composed bookmark replay: 100 undo/redo cycles, 153800 restored values and attribute clear/insert notifications and empty redo retained storage passed\n";
}
