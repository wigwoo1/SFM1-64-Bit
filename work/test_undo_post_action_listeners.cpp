#include "undo_post_action_listeners_candidate.h"
#include "model_undo_redo_actions_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Listener {int id;};
struct Adapter {
    Listener first{1},second{2},replacement{3},extra{4};
    std::vector<Listener*> listeners{&first,&second};std::vector<int> seen;
    int count=2;bool available=true,replayed=false;
    int listenerCount(){return count;}Listener* liveListener(int i){return listeners.at(i);}
    void invokeListenerSlot0(Listener* p){assert(replayed);seen.push_back(p->id);if(p==&first){listeners={&first,&replacement,&extra};count=3;}}
    bool undoAvailable(){return available;}bool redoAvailable(){return available;}
    void performUndo(){replayed=true;}void performRedo(){replayed=true;}
    void invokeLiveModelSlot126(){notifyUndoPostActionListenersCandidate(*this);}
};
int main(){Adapter a;modelUndoActionCandidate(a);assert((a.seen==std::vector<int>{1,3}));
    Adapter b;b.count=-1;modelRedoActionCandidate(b);assert(b.seen.empty() && b.replayed);
    Adapter c;c.available=false;modelUndoActionCandidate(c);assert(!c.replayed && c.seen.empty());
    Adapter d;d.listeners={&d.second,&d.second};modelRedoActionCandidate(d);assert((d.seen==std::vector<int>{2,2}));
    std::cout<<"x64 model post-action listeners: after replay/snapshot bound/live vector reload/duplicates/negative count passed\n";
}
