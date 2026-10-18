#include "model_undo_redo_actions_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter {
    bool available=false;int binding=0;std::vector<int> events;
    bool undoAvailable(){events.push_back(1);return available;}bool redoAvailable(){events.push_back(2);return available;}
    void performUndo(){events.push_back(3);binding=7;}void performRedo(){events.push_back(4);binding=9;}
    void invokeLiveModelSlot126(){events.push_back(binding);}
};
int main(){Adapter a;modelUndoActionCandidate(a);modelRedoActionCandidate(a);assert((a.events==std::vector<int>{1,2}));
    Adapter b;b.available=true;modelUndoActionCandidate(b);modelRedoActionCandidate(b);assert((b.events==std::vector<int>{1,3,7,2,4,9}));
    std::cout<<"x64 model undo/redo actions: availability gate/replay then live post-action binding passed\n";
}
