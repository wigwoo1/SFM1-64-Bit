#pragma once
template<class Listener,class Adapter>
bool removeNotificationListenerCandidate(Listener* listener,Adapter& a) {
    const int count=a.count();
    for(int i=0;i<count;++i){
        if(a.at(i)!=listener)continue;
        const int following=count-i-1;
        if(following>0)a.moveLeft(i,following);
        a.decrementCount();return true;
    }
    return false;
}
