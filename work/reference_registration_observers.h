#pragma once
#include <vector>
// 10360B20: snapshot element+50 observer list before invoking virtual0.
// The registration update caller later clears the observer list separately.
template<class Observer,class Element,class Enumerate,class Notify>
void notifyRecoveredReferenceRegistrationObserversCandidate(Element& e,bool blocked,Enumerate enumerate,Notify notify){
 if(blocked)return;
 std::vector<Observer> snapshot;enumerate([&](Observer observer){snapshot.push_back(observer);});
 for(auto observer:snapshot)notify(observer,e);
}
