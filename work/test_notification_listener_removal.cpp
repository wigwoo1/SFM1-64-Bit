#include "notification_listener_removal_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Listener{};
struct Adapter {
    std::vector<Listener*> values;int moves=0;
    int count(){return static_cast<int>(values.size());}Listener* at(int i){return values.at(i);}
    void moveLeft(int start,int n){++moves;for(int i=0;i<n;++i)values[start+i]=values[start+i+1];}
    void decrementCount(){values.pop_back();}
};
int main(){Listener a,b,c;Adapter x{{&a,&b,&a,nullptr,&c}};
    assert(removeNotificationListenerCandidate(&a,x));assert((x.values==std::vector<Listener*>{&b,&a,nullptr,&c}));
    assert(removeNotificationListenerCandidate(static_cast<Listener*>(nullptr),x));
    assert(removeNotificationListenerCandidate(&c,x));assert(x.moves==2);
    assert(!removeNotificationListenerCandidate(&c,x));assert(x.values.size()==2);
    Adapter empty;assert(!removeNotificationListenerCandidate(&a,empty));
    std::cout<<"x64 notification unsubscribe: stable order/first duplicate/null/tail/absent/empty passed\n";
}
