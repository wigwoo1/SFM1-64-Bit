#include "controller_listener_cleanup_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Listener{};struct Window{};
struct Adapter {
    Listener listener;Window ownedWindow;bool subscribed=true,present=true;std::vector<int> events;
    Listener* nativeListenerInterface(){return &listener;}
    void removePostActionListener(Listener* p){assert(p==&listener);subscribed=false;events.push_back(1);}
    void closeAllWindows(){assert(!subscribed);events.push_back(2);}
    void flushApplication(){events.push_back(3);}
    Window* window(){return present?&ownedWindow:nullptr;}
    void invokeWindowSlot3(Window* p,int flag){assert(!subscribed && p==&ownedWindow && flag==1);events.push_back(4);}
    void clearWindow(){present=false;events.push_back(5);}
    void cleanup25e0(){assert(!present && !subscribed);events.push_back(6);}
    void cleanupTail2f1700(){events.push_back(7);}
};
int main(){Adapter a;cleanupControllerListenerCandidate(a);assert((a.events==std::vector<int>{1,2,3,4,5,6,7}));
    Adapter b;b.present=false;cleanupControllerListenerCandidate(b);assert((b.events==std::vector<int>{1,2,3,5,6,7}));
    std::cout<<"x64 controller cleanup: native interface unsubscribe before window release/live window/clear before helpers passed\n";
}
