#include "undo_listener_registration_candidate.h"
#include "notification_listener_removal_candidate.h"
#include "undo_post_action_listeners_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Listener{int id;};
struct Adapter {
    std::vector<Listener*> values;std::vector<int> calls;
    int count(){return static_cast<int>(values.size());}int listenerCount(){return count();}
    int listenerCapacity(){return static_cast<int>(values.capacity());}
    Listener* at(int i){return values.at(i);}Listener* listenerAt(int i){return at(i);}Listener* liveListener(int i){return at(i);}
    void growListeners(int n){values.reserve(values.capacity()+n);}
    void incrementListenerCountAndRefreshBase(){values.push_back(nullptr);}
    void moveListenerRangeRight(int,int){assert(false);}Listener** listenerSlot(int i){return &values.at(i);}
    void moveLeft(int i,int n){for(int j=0;j<n;++j)values[i+j]=values[i+j+1];}
    void decrementCount(){values.pop_back();}void invokeListenerSlot0(Listener* p){calls.push_back(p->id);}
};
int main(){Adapter a;Listener x{1},y{2},z{3};
    assert(registerUndoListenerCandidate(&x,a));assert(registerUndoListenerCandidate(&y,a));assert(registerUndoListenerCandidate(&z,a));
    assert(removeNotificationListenerCandidate(&y,a));notifyUndoPostActionListenersCandidate(a);
    assert((a.calls==std::vector<int>{1,3}));assert(!removeNotificationListenerCandidate(&y,a));
    assert(registerUndoListenerCandidate(&y,a));a.calls.clear();notifyUndoPostActionListenersCandidate(a);
    assert((a.calls==std::vector<int>{1,3,2}));
    std::cout<<"x64 post-action listeners: register/unsubscribe/dispatch/re-register stable order passed\n";
}
