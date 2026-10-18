#pragma once
template<class Listener,class Adapter>
bool registerUndoListenerCandidate(Listener* listener,Adapter& a) {
    const auto count=a.listenerCount();
    for(int i=0;i<count;++i)if(a.listenerAt(i)==listener)return false;
    if(count+1>a.listenerCapacity())a.growListeners(count-a.listenerCapacity()+1);
    a.incrementListenerCountAndRefreshBase();
    const auto moved=a.listenerCount()-count-1;
    if(moved>0)a.moveListenerRangeRight(count,moved);
    if(auto* slot=a.listenerSlot(count))*slot=listener;
    return true;
}
