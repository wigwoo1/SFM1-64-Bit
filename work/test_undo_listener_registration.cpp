#include "undo_listener_registration_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Listener {int id;};
struct Adapter {
    std::vector<Listener*> values;int grows=0;
    int listenerCount(){return static_cast<int>(values.size());}int listenerCapacity(){return static_cast<int>(values.capacity());}
    Listener* listenerAt(int i){return values.at(i);}
    void growListeners(int n){assert(n>0);++grows;values.reserve(values.capacity()+n);}
    void incrementListenerCountAndRefreshBase(){values.push_back(nullptr);}
    void moveListenerRangeRight(int,int){assert(false);}
    Listener** listenerSlot(int i){return &values.at(i);}
};
int main(){Adapter a;Listener x{1},y{1};
    assert(registerUndoListenerCandidate(&x,a));const int grows=a.grows;
    assert(!registerUndoListenerCandidate(&x,a) && a.grows==grows);
    assert(registerUndoListenerCandidate(&y,a) && a.values.size()==2);
    assert(registerUndoListenerCandidate(static_cast<Listener*>(nullptr),a));
    assert(!registerUndoListenerCandidate(static_cast<Listener*>(nullptr),a));
    assert(a.values[0]==&x && a.values[1]==&y && !a.values[2]);
    static_assert(sizeof(Listener*)==8,"x64 required");
    std::cout<<"x64 listener registration: pointer identity/duplicate no growth/native pointers/null preserved passed\n";
}
